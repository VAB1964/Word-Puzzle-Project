#!/usr/bin/env python3
"""Stream a Kaikki English JSONL dump and classify pending ESDB words.

The full Kaikki dump is intentionally never loaded into memory.  Only exact,
lowercase matches from the pending-word CSV are retained.  This tool prepares
an editorial queue; it does not modify the playable dictionary or approve any
word by itself.
"""

import argparse
import csv
import gzip
import hashlib
import json
import re
from collections import Counter, defaultdict
from pathlib import Path

from clean_dictionary import good_example, normalize


EXCLUDED_POS = {
    "character", "circumfix", "infix", "interfix", "name", "phrase",
    "prefix", "prep_phrase", "proper_noun", "punct", "suffix", "symbol",
}
EXCLUDED_TAGS = {
    "abbreviation", "acronym", "alt-of-misspelling", "error-misspelling",
    "error-unrecognized-form", "eye-dialect", "initialism", "misspelling",
    "nonstandard", "obsolete", "offensive", "slur", "vulgar",
}
ARCHAIC_TAGS = {"archaic", "obsolete", "dated-form"}
REVIEW_TAGS = {
    "dated", "dialectal", "historical", "rare", "regional", "uncommon",
}
POS_MAP = {
    "adj": "adj", "adjective": "adj", "adv": "adv", "adverb": "adv",
    "conj": "conj", "conjunction": "conj", "det": "det", "determiner": "det",
    "intj": "intj", "interjection": "intj", "noun": "noun", "num": "num",
    "number": "num", "numeral": "num", "particle": "particle", "prep": "prep",
    "preposition": "prep", "pron": "pron", "pronoun": "pron", "verb": "verb",
}
ABBREVIATION = re.compile(
    r"^(?:an?\s+)?(?:syllabic\s+)?(?:abbreviation|initialism|acronym|clipping)\s+of\b",
    re.I,
)
BAD_FORM = re.compile(
    r"^(?:an?\s+)?(?:archaic|obsolete|nonstandard|misspelling|misconstruction|eye dialect)\s+"
    r"(?:form|spelling)?\s*of\b",
    re.I,
)
CONTROL = re.compile(r"[\x00-\x08\x0b\x0c\x0e-\x1f\x7f]")
BAD_DEFINITION_MARKERS = (
    "{{", "}}", "<ref", "</", "http://", "https://", "please add",
    "definition needed", "unknown meaning", "used as a word",
)
FIXED_INFLECTIONS = {"ns", "vs", "vs3", "vd", "vn", "vg", "aj1", "aj2", "av1", "av2", "d1", "d2", "ds"}


def _strings(value):
    """Return normalized strings from Kaikki's list-or-string fields."""
    if isinstance(value, str):
        return [normalize(value)] if normalize(value) else []
    if isinstance(value, list):
        return [normalize(item) for item in value if isinstance(item, str) and normalize(item)]
    return []


def normalized_tags(*values):
    tags = set()
    for value in values:
        for tag in _strings(value):
            tags.add(tag.lower().replace("_", "-"))
    return sorted(tags)


def definition_for(sense):
    """Keep Kaikki's gloss hierarchy while producing readable plain text."""
    glosses = _strings(sense.get("glosses"))
    definition = " ".join(glosses)
    if definition and definition[-1] not in ".?!":
        definition += "."
    return definition


def normalized_pos(pos):
    key = str(pos or "").lower().replace("-", "_").replace(" ", "_")
    return POS_MAP.get(key, key)


def rejection_reason(pos, definition, tags):
    pos_key = (pos or "").lower().replace("-", "_").replace(" ", "_")
    if pos_key in EXCLUDED_POS:
        return "proper-name-or-word-fragment"
    if pos_key not in POS_MAP:
        return "unsupported-part-of-speech"
    excluded = sorted(set(tags) & EXCLUDED_TAGS)
    if excluded:
        return "excluded-usage:" + ",".join(excluded)
    archaic = sorted(set(tags) & ARCHAIC_TAGS)
    if archaic:
        return "archaic-or-obsolete-usage:" + ",".join(archaic)
    lowered = definition.casefold()
    if (not definition or len(definition) < 10 or len(definition) > 500 or
            "\ufffd" in definition or CONTROL.search(definition) or
            any(marker in lowered for marker in BAD_DEFINITION_MARKERS)):
        return "missing-or-damaged-definition"
    if ABBREVIATION.match(definition):
        return "abbreviation-only-sense"
    if BAD_FORM.match(definition):
        return "obsolete-or-invalid-form"
    return None


def choose_example(word, sense):
    examples = []
    for item in sense.get("examples", []) if isinstance(sense.get("examples"), list) else []:
        text = item.get("text", "") if isinstance(item, dict) else item
        if isinstance(text, str) and good_example(text, word):
            examples.append(normalize(text))
    return min(examples, key=len) if examples else ""


def senses_from_entry(entry, source_line):
    """Flatten one Kaikki entry into reviewable and rejected senses."""
    accepted, rejected = [], []
    entry_pos = entry.get("pos", "")
    entry_tags = entry.get("tags", [])
    record_id = hashlib.sha256(
        json.dumps(entry, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")
    ).hexdigest()
    raw_senses = entry.get("senses", [])
    if not isinstance(raw_senses, list):
        raw_senses = []
    for sense in raw_senses:
        if not isinstance(sense, dict):
            continue
        raw_pos = str(sense.get("pos") or entry_pos or "")
        pos = normalized_pos(raw_pos)
        definition = definition_for(sense)
        tags = normalized_tags(entry_tags, sense.get("tags", []))
        sense_id = hashlib.sha256(json.dumps(
            {"word": entry.get("word", ""), "pos": pos, "definition": definition, "labels": tags},
            ensure_ascii=False, sort_keys=True, separators=(",", ":"),
        ).encode("utf-8")).hexdigest()
        row = {
            "sense_id": sense_id,
            "pos": pos,
            "definition": definition,
            "example": choose_example(str(entry.get("word", "")), sense),
            "labels": tags,
            "provenance": {"source": "Kaikki English", "record_id": record_id},
            "source_line": source_line,
        }
        reason = rejection_reason(raw_pos, definition, tags)
        if reason:
            rejected.append({**row, "reason": reason})
        else:
            flags = set(tags) & REVIEW_TAGS
            if sense.get("form_of") or "form-of" in tags:
                flags.add("grammatical-form")
            if sense.get("alt_of") or "alt-of" in tags:
                flags.add("alternative-form")
            flags = sorted(flags)
            accepted.append({**row, "review_flags": flags})
    return accepted, rejected


def _dedupe(rows, rejected=False):
    result = []
    seen = set()
    for row in rows:
        key = (row["pos"], row["definition"], row.get("reason", "") if rejected else "")
        if key in seen:
            continue
        seen.add(key)
        result.append(row)
    return result


def classify_word(word, entries):
    accepted, rejected = [], []
    for source_line, entry in entries:
        good, bad = senses_from_entry(entry, source_line)
        accepted.extend(good)
        rejected.extend(bad)
    accepted = _dedupe(accepted)
    rejected = _dedupe(rejected, rejected=True)
    review_flags = sorted({flag for row in accepted for flag in row["review_flags"]})
    blocking_flags = set()
    # Rejected senses do not poison a valid homograph (for example, an ordinary
    # noun that also has an offensive historical sense).  Blocking flags become
    # candidate-level gates only when no approvable sense remains.
    if not accepted:
        for row in rejected:
            reason = row["reason"]
            if reason == "proper-name-or-word-fragment":
                blocking_flags.add("proper-name" if row["pos"] == "name" else "word-fragment")
            elif reason.startswith("excluded-usage:"):
                blocking_flags.update(reason.partition(":")[2].split(","))
            elif reason.startswith("archaic-or-obsolete") or reason == "obsolete-or-invalid-form":
                blocking_flags.add("archaic")
            elif reason == "abbreviation-only-sense":
                blocking_flags.add("abbreviation")
            elif reason == "missing-or-damaged-definition":
                blocking_flags.add("damaged-definition")
            elif reason == "unsupported-part-of-speech":
                blocking_flags.add("bad-sense")
    if len(accepted) == 1 and not review_flags:
        tier = "safe"
        reason = "one-ordinary-modern-sense"
    elif len(accepted) > 1:
        tier = "ambiguous"
        reason = "multiple-usable-senses"
        review_flags.append("multiple-senses")
    elif accepted:
        tier = "manual"
        reason = "usage-needs-review"
    elif entries:
        tier = "manual"
        reason = "no-usable-sense"
    else:
        tier = "manual"
        reason = "not-found-in-kaikki"
        blocking_flags.add("unsupported-definition")
    for row in accepted:
        row.pop("review_flags", None)
    return {
        "word": word,
        "tier": tier,
        "frequency_rank": None,
        "unlocks": [],
        "flags": sorted(set(review_flags) | blocking_flags),
        "classification_reason": reason,
        "senses": accepted,
        "rejected_senses": rejected,
        "matched_entry_count": len(entries),
    }


def read_pending(path):
    with Path(path).open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        if not reader.fieldnames or "word" not in reader.fieldnames:
            raise ValueError("Pending CSV must contain a 'word' column")
        words = {row["word"].strip() for row in reader if row.get("word", "").strip()}
    invalid = sorted(word for word in words if not re.fullmatch(r"[a-z]{3,7}", word))
    if invalid:
        raise ValueError(f"Pending CSV contains invalid candidate: {invalid[0]}")
    return words


def load_frequency_ranks(path):
    """Rank unique corpus spellings by descending recorded frequency."""
    frequencies = {}
    with Path(path).open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        if not reader.fieldnames or not {"ngram", "freq"} <= set(reader.fieldnames):
            raise ValueError("Frequency CSV must contain ngram and freq columns")
        for row in reader:
            word = row.get("ngram", "").strip()
            try:
                frequency = int(row.get("freq", ""))
            except ValueError as error:
                raise ValueError(f"Invalid frequency for {word!r}") from error
            frequencies[word] = max(frequency, frequencies.get(word, -1))
    ordered = sorted(frequencies, key=lambda word: (-frequencies[word], word))
    return {word: rank for rank, word in enumerate(ordered, 1)}


def load_inflection_relations(path):
    with Path(path).open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        required = {"word", "lemma", "esdb_pos"}
        if not reader.fieldnames or not required <= set(reader.fieldnames):
            raise ValueError("Inflection CSV must contain word, lemma, and esdb_pos columns")
        return list(reader)


def enrich_candidates(candidates, pending, frequency_ranks=None, relations=None):
    frequency_ranks = frequency_ranks or {}
    relations = relations or []
    by_lemma = defaultdict(list)
    for relation in relations:
        if relation["lemma"] in pending and relation["word"] in pending and relation["word"] != relation["lemma"]:
            by_lemma[relation["lemma"]].append(relation)
    for candidate in candidates:
        candidate["frequency_rank"] = frequency_ranks.get(candidate["word"])
        possible_pos = {sense["pos"] for sense in candidate["senses"]}
        unlocks = set()
        for relation in by_lemma.get(candidate["word"], []):
            code = relation["esdb_pos"]
            if (code in FIXED_INFLECTIONS or
                    code == "ms" and possible_pos & {"noun", "verb"} or
                    code in {"a1", "a2"} and possible_pos & {"adj", "adv"}):
                unlocks.add(relation["word"])
        candidate["unlocks"] = sorted(unlocks)
    return candidates


def _open_text(path):
    if Path(path).suffix.lower() == ".gz":
        return gzip.open(path, "rt", encoding="utf-8")
    return Path(path).open(encoding="utf-8")


def file_sha256(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def write_text_atomic(path, text):
    path = Path(path)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(text, encoding="utf-8", newline="\n")
    temporary.replace(path)


def extract(kaikki_path, pending_path, output_dir, source_snapshot="",
            frequency_path=None, inflections_path=None):
    pending = read_pending(pending_path)
    matches = defaultdict(list)
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    matched_path = output_dir / "matched-kaikki-entries.jsonl"
    matched_temporary = matched_path.with_name(matched_path.name + ".tmp")
    scanned = 0
    with _open_text(kaikki_path) as source, matched_temporary.open("w", encoding="utf-8", newline="\n") as raw_output:
        for line_number, line in enumerate(source, 1):
            if not line.strip():
                continue
            scanned += 1
            try:
                entry = json.loads(line)
            except json.JSONDecodeError as error:
                raise ValueError(f"Invalid Kaikki JSON on line {line_number}: {error.msg}") from error
            if not isinstance(entry, dict):
                raise ValueError(f"Kaikki line {line_number} is not a JSON object")
            word = entry.get("word")
            # Exact matching deliberately prevents a proper name from supplying a
            # definition for an ordinary lowercase candidate.
            if word in pending:
                matches[word].append((line_number, entry))
                raw_output.write(json.dumps(entry, ensure_ascii=False, separators=(",", ":")) + "\n")
    matched_temporary.replace(matched_path)

    candidates = [classify_word(word, matches.get(word, [])) for word in sorted(pending)]
    enrich_candidates(
        candidates, pending,
        load_frequency_ranks(frequency_path) if frequency_path else None,
        load_inflection_relations(inflections_path) if inflections_path else None,
    )
    review_path = output_dir / "candidate-review.jsonl"
    write_text_atomic(review_path, "".join(
        json.dumps(candidate, ensure_ascii=False, sort_keys=True) + "\n" for candidate in candidates
    ))
    source_metadata = {
        "name": "Kaikki English",
        "snapshot": source_snapshot or Path(kaikki_path).name,
        "sha256": file_sha256(kaikki_path),
        "license": "CC BY-SA 4.0",
        "url": "https://kaikki.org/dictionary/English/",
    }
    catalog = {"schema_version": 1, "source": source_metadata, "candidates": candidates}
    write_text_atomic(output_dir / "candidate-catalog.json",
        json.dumps(catalog, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
    )
    tiers = Counter(candidate["tier"] for candidate in candidates)
    summary = {
        "pending_candidates": len(pending),
        "kaikki_entries_scanned": scanned,
        "matched_entries": sum(len(rows) for rows in matches.values()),
        "matched_words": len(matches),
        "unmatched_words": len(pending - matches.keys()),
        "ranked_candidates": sum(candidate["frequency_rank"] is not None for candidate in candidates),
        "lemmas_with_unlocks": sum(bool(candidate["unlocks"]) for candidate in candidates),
        "forms_unlocked": sum(len(candidate["unlocks"]) for candidate in candidates),
        "tiers": {tier: tiers.get(tier, 0) for tier in ("safe", "ambiguous", "manual")},
        "outputs": {
            "matched_entries": matched_path.name,
            "candidate_review": review_path.name,
            "candidate_catalog": "candidate-catalog.json",
        },
    }
    write_text_atomic(output_dir / "summary.json", json.dumps(summary, indent=2, sort_keys=True) + "\n")
    return candidates, summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--kaikki-jsonl", type=Path, required=True, help="Full English Kaikki .jsonl or .jsonl.gz dump")
    parser.add_argument("--pending", type=Path, required=True, help="CSV containing a word column")
    parser.add_argument("--output-dir", type=Path, required=True, help="Directory for extracted records and review queue")
    parser.add_argument("--source-snapshot", default="", help="Kaikki snapshot date/version (defaults to the dump filename)")
    parser.add_argument("--frequency", type=Path, default=Path(__file__).resolve().parents[1] / "1grams_english.csv",
                        help="Frequency CSV used for candidate ranking")
    parser.add_argument("--inflections", type=Path, default=Path(__file__).resolve().parents[1] / "data/esdb/inflections-60.csv",
                        help="Pinned ESDB relationships used to count forms unlocked by a lemma")
    args = parser.parse_args()
    _, summary = extract(args.kaikki_jsonl, args.pending, args.output_dir, args.source_snapshot,
                         args.frequency, args.inflections)
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Clean the existing game vocabulary using the checked-in Kaikki senses.

No network, new vocabulary, frequency reassignment, or generated word forms.
Run from the repo root; see docs/dictionary-cleanup.md for review/rebuild steps.
"""

import argparse
import csv
import json
import re
from collections import Counter, defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
LETTERS = re.compile(r"^[a-z]{3,7}$")
MEANING_SEPARATOR = " Meaning: "
EXCLUDED_POS = {"name", "prefix", "suffix", "infix", "interfix", "symbol", "punct", "phrase", "prep_phrase"}
EXCLUDED_TAGS = {
    "abbreviation", "initialism", "acronym", "misspelling", "error-unrecognized-form",
    "nonstandard", "eye-dialect", "offensive", "slur", "vulgar",
}
ABBREVIATION = re.compile(r"^(?:syllabic )?(?:abbreviation|initialism|acronym|clipping) of\b", re.I)
BAD_DEFINITION = re.compile(r"^(?:misspelling|obsolete (?:form|spelling)|archaic (?:form|spelling)|nonstandard (?:form|spelling)) of\b", re.I)
TEMPLATE = re.compile(r"^(?:[\w-]+ (?:means\b|is a word\b)|To [\w-]+ is\b)", re.I)
ARCHAIC_EXAMPLE = re.compile(r"\b(?:thou|thee|thy|thine|hath|doth|shalt|art thou|ye|unto)\b|(?-i:[ſͭͤ])|\[|\]|…|\.\.\.", re.I)
RELATION = re.compile(
    r"^((?:(?:first|second|third)-person\s+)?(?:(?:singular|plural|simple|present|past|indicative|and|participle|gerund|tense|of|the|form|inflection|alternative|spelling|standard|comparative|superlative|synonym)\s+)+)of\s+([A-Za-z]+)(?=[.,;:(“]|$|\s+[（(“])",
    re.I,
)


def normalize(text):
    return re.sub(r"\s+", " ", text or "").strip()


def definition_text(text):
    """Keep the complete sense path, separating its hierarchy as ordinary prose."""
    return normalize(text).replace(" | ", " ")


def reason_for_sense(sense):
    if sense["pos"] in EXCLUDED_POS:
        return "proper-name-or-nonword-sense"
    excluded = sorted(set(sense.get("tags", "").split(" | ")) & EXCLUDED_TAGS)
    if excluded:
        return "excluded-usage:" + ",".join(excluded)
    gloss = normalize(sense.get("glosses", ""))
    if not gloss or "\ufffd" in gloss:
        return "missing-or-damaged-definition"
    if ABBREVIATION.match(gloss):
        return "abbreviation-only-sense"
    if BAD_DEFINITION.match(gloss):
        return "obsolete-or-invalid-form"
    return None


def good_example(example, word):
    example = normalize(example)
    if not 12 <= len(example) <= 180 or len(example.split()) < 3:
        return False
    if TEMPLATE.match(example) or ARCHAIC_EXAMPLE.search(example) or "\ufffd" in example:
        return False
    # A complete example must actually demonstrate this spelling, not a related word.
    if not re.search(r"\b" + re.escape(word) + r"\b", example, re.I):
        return False
    if not re.match(r"^[\"'“‘]?[A-Z]", example) or not re.search(r"[.!?][\"'”’]?$", example):
        return False
    return True


def choose_example(word, sense, previous="", same_meaning=False):
    candidates = [previous] if same_meaning else []
    if sense:
        candidates.extend(sense.get("examples", "").split(" | "))
    valid = [normalize(s) for s in candidates if good_example(s, word)]
    # Keep a suitable existing example; otherwise prefer a short complete source example.
    if same_meaning and valid and valid[0] == normalize(previous):
        return valid[0]
    return min(valid, key=len) if valid else ""


def choose_sense(senses, previous):
    usable = [s for s in senses if reason_for_sense(s) is None]
    if not usable:
        return None
    old = definition_text(previous.get("Definition", "")).split(MEANING_SEPARATOR, 1)[0].rstrip(".")
    def rank(s):
        tags = set(s.get("tags", "").split(" | "))
        # Preserve a valid existing meaning. Do not assume the shortest sense is the commonest.
        return (
            bool(tags & {"obsolete", "archaic"}),
            definition_text(s["glosses"]).rstrip(".") != old,
            bool(tags & {"dated", "dialectal", "rare", "uncommon"}),
            s["pos"] != previous.get("pos"),
        )
    return min(usable, key=rank)


def read_rows(path):
    with Path(path).open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        return list(reader), reader.fieldnames


def clean_rows(rows, source_rows, overrides, reference_meanings=None):
    reference_meanings = reference_meanings or {}
    groups = defaultdict(list)
    source = defaultdict(list)
    for sense in source_rows:
        source[sense["word"]].append(sense)
    removed = []
    for row in rows:
        original = row["word"].strip()
        key = original.lower()
        if not LETTERS.fullmatch(key):
            removed.append({"word": original, "rarity": row["rarity"], "reason": "outside-ascii-letter-wheel"})
        else:
            groups[key].append(row)

    result = {}
    decisions = {}
    for word, candidates in groups.items():
        # A capitalized name/brand cannot override a real lowercase word or its rarity.
        lowercase = [r for r in candidates if r["word"].strip() == word]
        previous = (lowercase or candidates)[0]
        override = overrides.get(word)
        if override and override.get("exclude"):
            decisions[word] = override["reason"]
            continue
        sense = choose_sense(source[word], previous)
        if override:
            cleaned = dict(previous)
            cleaned.update({k: override[k] for k in ("pos", "Definition", "Sentence") if k in override})
        elif sense:
            cleaned = dict(previous)
            cleaned["pos"] = sense["pos"]
            cleaned["Definition"] = definition_text(sense["glosses"])
            cleaned["Sentence"] = choose_example(
                word, sense, previous.get("Sentence", ""),
                definition_text(previous.get("Definition", "")).split(MEANING_SEPARATOR, 1)[0].rstrip(".") == cleaned["Definition"].rstrip("."),
            )
        elif source[word]:
            decisions[word] = ";".join(sorted({reason_for_sense(s) for s in source[word]}))
            continue
        elif not lowercase:
            decisions[word] = "capitalized-only-without-lowercase-source"
            continue
        else:
            # Preserve established inflections absent from this partial source export.
            cleaned = dict(previous)
            definition = definition_text(previous.get("Definition", ""))
            if not definition or "\ufffd" in definition or ABBREVIATION.match(definition) or BAD_DEFINITION.match(definition):
                decisions[word] = "missing-or-unsuitable-definition"
                continue
            cleaned["Definition"] = definition
            cleaned["Sentence"] = choose_example(word, None, previous.get("Sentence", ""), True)
        cleaned["word"] = word
        cleaned["Definition"] = definition_text(cleaned["Definition"])
        cleaned["Sentence"] = normalize(cleaned.get("Sentence", ""))
        result[word] = cleaned

    # Resolve grammatical/spelling references, including two-letter roots that are
    # explanatory only. Reject cycles and forms whose only root was excluded.
    resolving = set()
    resolved = {}
    def root_meaning(word, pos):
        key = (word, pos)
        if key in resolving:
            raise ValueError("cyclic-reference")
        if key in resolved:
            return resolved[key]
        if word in reference_meanings and pos in reference_meanings[word]:
            return reference_meanings[word][pos]
        if word in overrides and overrides[word].get("exclude"):
            raise ValueError("excluded-root")
        resolving.add(key)
        row = result.get(word)
        if row and row["pos"] == pos:
            definition = row["Definition"].split(MEANING_SEPARATOR, 1)[0]
        else:
            senses = [s for s in source[word] if s["pos"] == pos]
            sense = choose_sense(senses, {"pos": pos})
            if senses and not sense:
                resolving.remove(key)
                raise ValueError("excluded-root")
            definition = definition_text(sense["glosses"]) if sense else ""
        match = RELATION.match(definition)
        try:
            if match:
                target = match.group(2)
                # Preserve case: a plural of a proper name is still a name.
                if target != target.lower():
                    raise ValueError("capitalized-root")
                meaning = root_meaning(target, pos)
            else:
                usable_meanings = {
                    definition_text(s["glosses"]) for s in source[word]
                    if s["pos"] == pos and reason_for_sense(s) is None
                }
                # Do not attach an arbitrary homograph sense (e.g. "mean" the
                # average versus "mean" the intention) to an inflected word.
                meaning = definition if word in overrides or len(usable_meanings) <= 1 else None
                meaning = meaning or None
        finally:
            resolving.remove(key)
        resolved[key] = meaning
        return meaning

    for word, row in list(result.items()):
        base_definition = row["Definition"].split(MEANING_SEPARATOR, 1)[0]
        match = RELATION.match(base_definition)
        if not match or word in overrides:
            continue
        target = match.group(2)
        try:
            meaning = root_meaning(word, row["pos"])
        except ValueError as error:
            decisions[word] = str(error) + ":" + target
            continue
        if meaning:
            row["Definition"] = base_definition.rstrip(".") + "." + MEANING_SEPARATOR + meaning
        # Absence from a partial export is not evidence that a known form is invalid.
    for word in decisions:
        result.pop(word, None)

    for word, candidates in groups.items():
        if word in decisions:
            for row in candidates:
                removed.append({"word": row["word"], "rarity": row["rarity"], "reason": decisions[word]})
        else:
            chosen = next((r for r in candidates if r["word"].strip() == word), candidates[0])
            skipped_chosen = False
            for row in candidates:
                if row is chosen and not skipped_chosen:
                    skipped_chosen = True
                else:
                    removed.append({"word": row["word"], "rarity": row["rarity"], "reason": "duplicate-after-lowercasing"})

    cleaned_rows = sorted(result.values(), key=lambda r: (len(r["word"]), r["word"]))
    return cleaned_rows, removed


def validate(rows):
    seen = set()
    for row in rows:
        word = row["word"]
        if not LETTERS.fullmatch(word) or word in seen:
            raise ValueError(f"Invalid or duplicate word: {word}")
        seen.add(word)
        if row["rarity"] not in {"1", "2", "3", "4"}:
            raise ValueError(f"Invalid rarity for {word}")
        if not row["Definition"] or "\ufffd" in row["Definition"]:
            raise ValueError(f"Missing/damaged definition: {word}")
        if TEMPLATE.match(row.get("Sentence", "")):
            raise ValueError(f"Placeholder example: {word}")


def write_rows(path, rows, fieldnames):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, default=ROOT / "words_processed.csv")
    parser.add_argument("--source", type=Path, default=ROOT / "kaikki_english.csv")
    parser.add_argument("--overrides", type=Path, default=ROOT / "tools/dictionary_overrides.json")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--removed", type=Path)
    args = parser.parse_args()
    rows, fields = read_rows(args.input)
    source_rows, _ = read_rows(args.source)
    editorial = json.loads(args.overrides.read_text(encoding="utf-8"))
    cleaned, removed = clean_rows(rows, source_rows, editorial["words"], editorial.get("reference_meanings"))
    validate(cleaned)
    write_rows(args.output, cleaned, fields)
    summary = {
        "input_entries": len(rows), "output_words": len(cleaned),
        "removed_entries": len(removed),
        "removal_reasons": dict(Counter(r["reason"].split(":", 1)[0] for r in removed)),
        "rarity_counts": dict(sorted(Counter(r["rarity"] for r in cleaned).items())),
        "with_examples": sum(bool(r["Sentence"]) for r in cleaned),
        "without_examples": sum(not r["Sentence"] for r in cleaned),
    }
    if args.removed:
        write_rows(args.removed, removed, ["word", "rarity", "reason"])
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()

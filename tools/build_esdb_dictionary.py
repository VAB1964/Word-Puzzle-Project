#!/usr/bin/env python3
"""Build the playable dictionary from pinned ESDB vocabulary and reviewed meanings.

ESDB levels decide vocabulary eligibility, never game rarity. Undefined words stay
in a review queue. No network access or ESDB installation is needed for this build.
"""

import argparse
import csv
import io
import json
import re
import subprocess
from collections import Counter, defaultdict
from datetime import datetime
from pathlib import Path

from clean_dictionary import LETTERS, validate, write_rows
from export_esdb import text_hash
from esdb_review_batches import (ALLOWED_POS, BLOCKING_FLAGS, SCHEMA_VERSION,
                                 TIERS, _valid_definition, digest,
                                 relation_only_definition)

ROOT = Path(__file__).resolve().parents[1]
BASELINE = "88c958927f986cff82f19222d8d7482567b908d3"
FIELDS = ["word", "rarity", "pos", "Definition", "Sentence"]
PROVENANCE_FIELDS = ["word", "esdb_level", "definition_source", "rarity_basis", "lemma"]
BATCH_PROVENANCE_FIELDS = [
    "approval_batch", "approval_batch_sha256", "approval_artifact",
    "reviewed_by", "reviewed_at", "definition_source_name",
    "definition_record_id", "definition_sense_id",
]
GENERATED_SENTENCES_PATH = ROOT / "tools/generated_sentences.json"
FORM_TYPES = {
    "ns": ("noun", "Plural"),
    "vs": ("verb", "Third-person singular present tense"),
    "vs3": ("verb", "Third-person singular present tense"),
    "vd": ("verb", "Past tense"), "vn": ("verb", "Past participle"),
    "vg": ("verb", "Present participle"),
    "aj1": ("adj", "Comparative"), "aj2": ("adj", "Superlative"),
    "av1": ("adv", "Comparative"), "av2": ("adv", "Superlative"),
    "d1": ("det", "Comparative"), "d2": ("det", "Superlative"),
    "ds": ("det", "Plural"),
}


def inflection(word, relations, known):
    """Describe only attested forms of a known lemma spelling.

    Do not copy a lemma's lexical definition: its homograph sense may be different
    (lie/lied). A grammatical definition is explicit and does not guess the sense.
    """
    candidates = []
    for relation in relations:
        lemma, code = relation["lemma"], relation["esdb_pos"]
        root = known.get(lemma)
        if not root or lemma == word:
            continue
        if code == "ms":
            kind = (root["pos"], "Plural" if root["pos"] == "noun" else "Third-person singular present tense") if root["pos"] in {"noun", "verb"} else None
        elif code in {"a1", "a2"}:
            kind = (root["pos"], "Comparative" if code == "a1" else "Superlative") if root["pos"] in {"adj", "adv"} else None
        else:
            kind = FORM_TYPES.get(code)
        # ESDB supplies the form's POS. The game's single definition for the root
        # may cover another POS (act as a noun vs acted as a verb); that does not
        # invalidate the attested grammatical relationship.
        if kind:
            candidates.append((lemma, kind[0], kind[1], root["rarity"]))
    if not candidates:
        return None
    # A stable lexical choice, not a claim about the most frequent meaning.
    lemma, pos, label, rarity = sorted(set(candidates))[0]
    labels = {c[2] for c in candidates if c[0] == lemma and c[1] == pos}
    if {"Past tense", "Past participle"} <= labels:
        label = "Past tense and past participle"
    return {
        "word": word, "rarity": rarity, "pos": pos,
        "Definition": f"{label} of {lemma}.", "Sentence": "",
    }, lemma


def load_approved_batches(path):
    """Load immutable, validated review artifacts from *path*.

    Only approvals affect the playable dictionary. Rejections and deferrals stay
    in their signed artifact as the editorial record, without becoming implicit
    exclusions. Duplicate approvals are rejected rather than depending on file
    system ordering.
    """
    if not path.exists():
        return {}
    if not path.is_dir():
        raise ValueError(f"Approved batches path is not a directory: {path}")
    approvals = {}
    for artifact_path in sorted(path.glob("*.json")):
        try:
            artifact = json.loads(artifact_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            raise ValueError(f"Cannot read approved batch {artifact_path}: {exc}") from exc
        if not isinstance(artifact, dict) or artifact.get("schema_version") != SCHEMA_VERSION:
            raise ValueError(f"Invalid approved batch schema: {artifact_path}")
        claimed_hash = artifact.get("artifact_sha256")
        unsigned = {key: value for key, value in artifact.items() if key != "artifact_sha256"}
        if claimed_hash != digest(unsigned):
            raise ValueError(f"Approved batch checksum mismatch: {artifact_path}")
        batch_id, batch_hash = artifact.get("batch_id"), artifact.get("batch_sha256")
        reviewer, reviewed_at = artifact.get("reviewer"), artifact.get("reviewed_at")
        if (not isinstance(batch_id, str) or not batch_id or
                not isinstance(batch_hash, str) or not re.fullmatch(r"[0-9a-f]{64}", batch_hash) or
                not isinstance(reviewer, dict) or reviewer.get("kind") not in {"agent", "human"} or
                not isinstance(reviewer.get("id"), str) or not reviewer["id"].strip()):
            raise ValueError(f"Approved batch lacks review provenance: {artifact_path}")
        try:
            if (not isinstance(reviewed_at, str) or
                    datetime.fromisoformat(reviewed_at.replace("Z", "+00:00")).tzinfo is None):
                raise ValueError
        except ValueError as exc:
            raise ValueError(f"Invalid approved batch review timestamp: {artifact_path}") from exc
        decisions = artifact.get("decisions")
        if not isinstance(decisions, list):
            raise ValueError(f"Approved batch decisions must be a list: {artifact_path}")
        seen = set()
        for decision in decisions:
            if not isinstance(decision, dict) or not LETTERS.fullmatch(str(decision.get("word", ""))):
                raise ValueError(f"Invalid decision in approved batch: {artifact_path}")
            word = decision["word"]
            if word in seen or decision.get("decision") not in {"approve", "reject", "defer"}:
                raise ValueError(f"Duplicate or invalid decision for {word}: {artifact_path}")
            seen.add(word)
            if decision["decision"] != "approve":
                continue
            if word in approvals:
                raise ValueError(f"Word approved by more than one batch: {word}")
            sense = decision.get("sense")
            provenance = sense.get("provenance") if isinstance(sense, dict) else None
            raw_flags = decision.get("candidate_flags", [])
            raw_labels = sense.get("labels", []) if isinstance(sense, dict) else []
            flags = set(raw_flags) if (isinstance(raw_flags, list) and
                                       all(isinstance(item, str) for item in raw_flags)) else {"invalid"}
            labels = set(raw_labels) if (isinstance(raw_labels, list) and
                                         all(isinstance(item, str) for item in raw_labels)) else {"invalid"}
            rarity = decision.get("rarity")
            if (not isinstance(sense, dict) or sense.get("pos") not in ALLOWED_POS or
                    not isinstance(sense.get("sense_id"), str) or not sense["sense_id"] or
                    not _valid_definition(sense.get("definition")) or
                    not isinstance(provenance, dict) or not provenance.get("source") or
                    not provenance.get("record_id") or flags & BLOCKING_FLAGS or
                    labels & BLOCKING_FLAGS or decision.get("candidate_tier") not in TIERS or
                    relation_only_definition(sense.get("definition")) or
                    rarity not in (1, 2, 3, 4) or isinstance(rarity, bool) or
                    not isinstance(decision.get("reason"), str) or
                    len(decision["reason"].strip()) < 10):
                raise ValueError(f"Unsafe or incomplete approval for {word}: {artifact_path}")
            approvals[word] = {
                "word": word, "rarity": str(rarity), "pos": sense["pos"],
                "Definition": sense["definition"], "Sentence": "",
                "reason": decision.get("reason", ""),
                "approval_batch": batch_id, "approval_batch_sha256": batch_hash,
                "approval_artifact": claimed_hash,
                "reviewed_by": f'{reviewer["kind"]}:{reviewer["id"]}',
                "reviewed_at": reviewed_at,
                "definition_source_name": str(provenance["source"]),
                "definition_record_id": str(provenance["record_id"]),
                "definition_sense_id": sense["sense_id"],
            }
    return approvals


def load_generated_sentences(path=GENERATED_SENTENCES_PATH):
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    if data.get("schema_version") != 1 or not isinstance(data.get("entries"), dict):
        raise ValueError("Invalid generated-sentence catalog")
    return data["entries"]


def apply_generated_sentences(rows, generated):
    by_word = {row["word"]: row for row in rows}
    unknown = sorted(set(generated) - set(by_word))
    if unknown:
        raise ValueError(f"Generated sentences contain unknown words: {', '.join(unknown[:5])}")
    applied = 0
    for word, entry in generated.items():
        row = by_word[word]
        if entry.get("pos") != row["pos"] or entry.get("definition") != row["Definition"]:
            raise ValueError(f"Stale generated sentence metadata: {word}")
        sentence = re.sub(r"\s+", " ", str(entry.get("sentence", ""))).strip()
        tokens = re.findall(r"[A-Za-z]+(?:['-][A-Za-z]+)*", sentence)
        if (not 12 <= len(sentence) <= 180 or not 4 <= len(tokens) <= 20 or
                not re.search(rf"(?:^|[^A-Za-z]){re.escape(word)}(?:[^A-Za-z]|$)", sentence, re.I) or
                not re.match(r"^[A-Z]", sentence) or not re.search(r"[.!?]$", sentence) or
                len(re.findall(r"[.!?]", sentence)) != 1):
            raise ValueError(f"Invalid generated sentence: {word}")
        if not row["Sentence"]:
            row["Sentence"] = sentence
            applied += 1
    missing = [row["word"] for row in rows if not row["Sentence"]]
    if missing:
        raise ValueError(f"Playable words still lack sentences: {', '.join(missing[:5])}")
    return applied


def build(baseline, base_words, larger_levels, forms, editorial, excluded,
          batch_approvals=None, generated_sentences=None):
    validate(baseline)
    old = {row["word"]: row for row in baseline}
    approved = editorial["larger_level_approvals"]
    additions = editorial["new_definitions"]
    supplemental = editorial.get("supplemental_entries", {})
    batch_approvals = batch_approvals or {}
    for word, decision in approved.items():
        if word not in larger_levels or word in base_words or word not in old:
            raise ValueError(f"Larger-level approval is not an existing extension word: {word}")
        if larger_levels[word] != decision["level"] or not decision["reason"]:
            raise ValueError(f"Incorrect larger-level decision: {word}")
    for word, row in additions.items():
        if word not in base_words or word in old or not row["reason"]:
            raise ValueError(f"Editorial addition is not a new size-60 word: {word}")
    for word, row in supplemental.items():
        if (not LETTERS.fullmatch(word) or word in base_words or
                word in approved or word in additions or not row["reason"]):
            raise ValueError(f"Invalid supplemental editorial entry: {word}")
    for word, row in batch_approvals.items():
        if (word not in base_words or word in old or word in additions or
                row.get("word") != word or not row.get("reason") or
                any(field not in row for field in FIELDS + BATCH_PROVENANCE_FIELDS)):
            raise ValueError(f"Invalid approved-batch entry: {word}")
    if (set(approved) | set(additions) | set(supplemental) | set(batch_approvals)) & excluded:
        raise ValueError("An editorial approval conflicts with an explicit exclusion")
    eligible = (base_words | set(approved)) - excluded
    result = {word: dict(old[word]) for word in eligible & old.keys()}
    provenance = {
        word: {"word": word, "esdb_level": 60 if word in base_words else larger_levels[word],
               "definition_source": "cleaned-dictionary", "rarity_basis": "preserved", "lemma": ""}
        for word in result
    }
    for word, row in additions.items():
        result[word] = {"word": word, **{field: str(row[field]) for field in FIELDS[1:]}}
        provenance[word] = {"word": word, "esdb_level": 60, "definition_source": "editorial",
                            "rarity_basis": "provisional-editorial", "lemma": ""}
    for word, row in batch_approvals.items():
        result[word] = {field: str(row[field]) for field in FIELDS}
        provenance[word] = {
            "word": word, "esdb_level": 60, "definition_source": "reviewed-batch",
            "rarity_basis": "provisional-editorial", "lemma": "",
            **{field: row[field] for field in BATCH_PROVENANCE_FIELDS},
        }
    for word, row in supplemental.items():
        result[word] = {"word": word, **{field: str(row[field]) for field in FIELDS[1:]}}
        provenance[word] = {
            "word": word, "esdb_level": larger_levels.get(word, ""),
            "definition_source": "editorial-supplement",
            "rarity_basis": "preserved" if word in old and old[word]["rarity"] == str(row["rarity"])
                            else "provisional-editorial",
            "lemma": "",
        }
    # Resolve against lexical entries only. No recursive suffix guessing or cycles.
    known = {word: row for word, row in result.items() if word in base_words}
    by_word = defaultdict(list)
    for row in forms:
        by_word[row["word"]].append(row)
    for word in sorted((base_words - excluded) - result.keys()):
        selected = inflection(word, by_word[word], known)
        if selected:
            result[word], lemma = selected
            provenance[word] = {"word": word, "esdb_level": 60, "definition_source": "esdb-inflection",
                                "rarity_basis": "inherited-from-lemma", "lemma": lemma}
            if lemma in batch_approvals:
                provenance[word].update({field: batch_approvals[lemma][field]
                                         for field in BATCH_PROVENANCE_FIELDS})
    rows = sorted(result.values(), key=lambda row: (len(row["word"]), row["word"]))
    generated_example_count = apply_generated_sentences(rows, generated_sentences) if generated_sentences is not None else 0
    validate(rows)
    pending = [{"word": word, "reason": "explicit-editorial-exclusion" if word in excluded else "needs-definition-and-usage-review"}
               for word in sorted(base_words - result.keys())]
    removed = [{"word": word, "rarity": old[word]["rarity"],
                "reason": "explicit-editorial-exclusion" if word in excluded else
                          "larger-level-not-approved" if word in larger_levels else "outside-filtered-American-60-80"}
               for word in sorted(old.keys() - result.keys())]
    review = [{"word": word, "esdb_level": level,
               "decision": "approved" if word in approved or word in supplemental else "deferred",
               "existing_rarity": old.get(word, {}).get("rarity", ""),
               "existing_definition": old.get(word, {}).get("Definition", ""),
               "reason": (approved[word]["reason"] if word in approved else supplemental[word]["reason"])
                         if word in approved or word in supplemental
                         else "Not individually approved; not added to puzzles."}
              for word, level in sorted(larger_levels.items())]
    summary = {
        "baseline_words": len(old), "size60_candidates": len(base_words),
        "playable_words": len(rows), "size60_playable": sum(w in base_words for w in result),
        "larger_level_approved": sum(word in result for word in larger_levels),
        "supplemental_outside_esdb": sum(word not in base_words and word not in larger_levels for word in supplemental),
        "pending_size60": len(pending),
        "retained_words": len(old.keys() & result.keys()), "added_words": len(result.keys() - old.keys()),
        "removed_words": len(removed),
        "definition_sources": dict(sorted(Counter(p["definition_source"] for p in provenance.values()).items())),
        "generated_examples": generated_example_count,
        "words_with_examples": sum(bool(row["Sentence"]) for row in rows),
        "words_without_examples": sum(not row["Sentence"] for row in rows),
        "rarity_counts": dict(sorted(Counter(row["rarity"] for row in rows).items())),
        "removed_easiest_words": sorted(w for w in old.keys() - result.keys() if old[w]["rarity"] == "1"),
    }
    if batch_approvals:
        summary["approved_batch_headwords"] = len(batch_approvals)
        summary["approved_batch_forms"] = sum(
            row["definition_source"] == "esdb-inflection" and bool(row.get("approval_batch"))
            for row in provenance.values())
        summary["approved_batches"] = sorted({row["approval_batch"] for row in batch_approvals.values()})
    return rows, pending, removed, review, sorted(provenance.values(), key=lambda row: row["word"]), summary


def load_snapshot(path):
    manifest = json.loads((path / "manifest.json").read_text(encoding="utf-8"))
    for name, digest in manifest["sha256_normalized_utf8"].items():
        if text_hash((path / name).read_text(encoding="utf-8")) != digest:
            raise ValueError(f"ESDB snapshot checksum mismatch: {name}")
    base = set((path / "american-60.txt").read_text(encoding="utf-8").splitlines())
    larger = {}
    for size in (70, 80):
        for word in (path / f"american-{size}-additions.txt").read_text(encoding="utf-8").splitlines():
            if word in base or word in larger:
                raise ValueError(f"Duplicate ESDB level entry: {word}")
            larger[word] = size
    if not all(LETTERS.fullmatch(word) for word in base | larger.keys()):
        raise ValueError("Invalid word in ESDB snapshot")
    with (path / "inflections-60.csv").open(encoding="utf-8", newline="") as handle:
        forms = list(csv.DictReader(handle))
    return base, larger, forms


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path, help="Optional exported pre-ESDB CSV; otherwise read the pinned Git commit.")
    parser.add_argument("--snapshot", type=Path, default=ROOT / "data/esdb")
    parser.add_argument("--approved-batches", type=Path, default=ROOT / "tools/esdb_approved_batches",
                        help="Directory of validated review artifacts (default: tools/esdb_approved_batches).")
    parser.add_argument("--output-dir", type=Path, required=True, help="Candidate directory; does not overwrite the game's dictionary.")
    args = parser.parse_args()
    text = args.baseline.read_text(encoding="utf-8-sig") if args.baseline else subprocess.check_output(
        ["git", "show", f"{BASELINE}:words_processed.csv"], cwd=ROOT, text=True, encoding="utf-8")
    baseline = list(csv.DictReader(io.StringIO(text)))
    editorial = json.loads((ROOT / "tools/esdb_editorial.json").read_text(encoding="utf-8"))
    old_editorial = json.loads((ROOT / "tools/dictionary_overrides.json").read_text(encoding="utf-8"))
    excluded = {word for word, row in old_editorial["words"].items() if row.get("exclude")}
    batch_approvals = load_approved_batches(args.approved_batches)
    generated_sentences = load_generated_sentences()
    rows, pending, removed, review, provenance, summary = build(
        baseline, *load_snapshot(args.snapshot), editorial, excluded, batch_approvals,
        generated_sentences)
    output = args.output_dir
    write_rows(output / "words_processed.csv", rows, FIELDS)
    write_rows(output / "pending-definitions.csv", pending, ["word", "reason"])
    write_rows(output / "removed-words.csv", removed, ["word", "rarity", "reason"])
    write_rows(output / "larger-level-review.csv", review, ["word", "esdb_level", "decision", "existing_rarity", "existing_definition", "reason"])
    provenance_fields = PROVENANCE_FIELDS + (BATCH_PROVENANCE_FIELDS if batch_approvals else [])
    write_rows(output / "word-provenance.csv", provenance, provenance_fields)
    summary["baseline_commit"] = BASELINE if not args.baseline else "explicit --baseline file"
    (output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()

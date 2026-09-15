#!/usr/bin/env python3
"""Build the playable dictionary from pinned ESDB vocabulary and reviewed meanings.

ESDB levels decide vocabulary eligibility, never game rarity. Undefined words stay
in a review queue. No network access or ESDB installation is needed for this build.
"""

import argparse
import csv
import io
import json
import subprocess
from collections import Counter, defaultdict
from pathlib import Path

from clean_dictionary import LETTERS, validate, write_rows
from export_esdb import text_hash

ROOT = Path(__file__).resolve().parents[1]
BASELINE = "88c958927f986cff82f19222d8d7482567b908d3"
FIELDS = ["word", "rarity", "pos", "Definition", "Sentence"]
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


def build(baseline, base_words, larger_levels, forms, editorial, excluded):
    validate(baseline)
    old = {row["word"]: row for row in baseline}
    approved = editorial["larger_level_approvals"]
    additions = editorial["new_definitions"]
    for word, decision in approved.items():
        if word not in larger_levels or word in base_words or word not in old:
            raise ValueError(f"Larger-level approval is not an existing extension word: {word}")
        if larger_levels[word] != decision["level"] or not decision["reason"]:
            raise ValueError(f"Incorrect larger-level decision: {word}")
    for word, row in additions.items():
        if word not in base_words or word in old or not row["reason"]:
            raise ValueError(f"Editorial addition is not a new size-60 word: {word}")
    if (set(approved) | set(additions)) & excluded:
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
    rows = sorted(result.values(), key=lambda row: (len(row["word"]), row["word"]))
    validate(rows)
    pending = [{"word": word, "reason": "explicit-editorial-exclusion" if word in excluded else "needs-definition-and-usage-review"}
               for word in sorted(base_words - result.keys())]
    removed = [{"word": word, "rarity": old[word]["rarity"],
                "reason": "explicit-editorial-exclusion" if word in excluded else
                          "larger-level-not-approved" if word in larger_levels else "outside-filtered-American-60-80"}
               for word in sorted(old.keys() - result.keys())]
    review = [{"word": word, "esdb_level": level,
               "decision": "approved" if word in approved else "deferred",
               "existing_rarity": old.get(word, {}).get("rarity", ""),
               "existing_definition": old.get(word, {}).get("Definition", ""),
               "reason": approved[word]["reason"] if word in approved else "Not individually approved; not added to puzzles."}
              for word, level in sorted(larger_levels.items())]
    summary = {
        "baseline_words": len(old), "size60_candidates": len(base_words),
        "playable_words": len(rows), "size60_playable": sum(w in base_words for w in result),
        "larger_level_approved": len(approved), "pending_size60": len(pending),
        "retained_words": len(old.keys() & result.keys()), "added_words": len(result.keys() - old.keys()),
        "removed_words": len(removed),
        "definition_sources": dict(sorted(Counter(p["definition_source"] for p in provenance.values()).items())),
        "rarity_counts": dict(sorted(Counter(row["rarity"] for row in rows).items())),
        "removed_easiest_words": sorted(w for w in old.keys() - result.keys() if old[w]["rarity"] == "1"),
    }
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
    parser.add_argument("--output-dir", type=Path, required=True, help="Candidate directory; does not overwrite the game's dictionary.")
    args = parser.parse_args()
    text = args.baseline.read_text(encoding="utf-8-sig") if args.baseline else subprocess.check_output(
        ["git", "show", f"{BASELINE}:words_processed.csv"], cwd=ROOT, text=True, encoding="utf-8")
    baseline = list(csv.DictReader(io.StringIO(text)))
    editorial = json.loads((ROOT / "tools/esdb_editorial.json").read_text(encoding="utf-8"))
    old_editorial = json.loads((ROOT / "tools/dictionary_overrides.json").read_text(encoding="utf-8"))
    excluded = {word for word, row in old_editorial["words"].items() if row.get("exclude")}
    rows, pending, removed, review, provenance, summary = build(baseline, *load_snapshot(args.snapshot), editorial, excluded)
    output = args.output_dir
    write_rows(output / "words_processed.csv", rows, FIELDS)
    write_rows(output / "pending-definitions.csv", pending, ["word", "reason"])
    write_rows(output / "removed-words.csv", removed, ["word", "rarity", "reason"])
    write_rows(output / "larger-level-review.csv", review, ["word", "esdb_level", "decision", "existing_rarity", "existing_definition", "reason"])
    write_rows(output / "word-provenance.csv", provenance, ["word", "esdb_level", "definition_source", "rarity_basis", "lemma"])
    summary["baseline_commit"] = BASELINE if not args.baseline else "explicit --baseline file"
    (output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()

"""Classify reported multiplayer word issues against the playable dictionary."""

from __future__ import annotations

import argparse
import csv
import sys
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ISSUES = ROOT / "data" / "word_issues.csv"
DEFAULT_DICTIONARY = ROOT / "words_processed.csv"
VALID_ISSUES = {"missing_from_database", "missing_from_puzzle_or_bonus"}


def can_spell(word: str, puzzle_word: str) -> bool:
    available = Counter(puzzle_word)
    required = Counter(word)
    return all(required[letter] <= available[letter] for letter in required)


def load_dictionary(path: Path) -> set[str]:
    with path.open(encoding="utf-8-sig", newline="") as handle:
        return {row["word"].strip().lower() for row in csv.DictReader(handle)}


def classify(row: dict[str, str], dictionary: set[str]) -> tuple[str, str]:
    word = row.get("word", "").strip().lower()
    puzzle_word = row.get("puzzle_word", "").strip().lower()
    reported_issue = row.get("reported_issue", "").strip()

    if not word.isalpha() or not word.isascii():
        return "invalid_report", "Word must contain only ASCII letters."
    if reported_issue not in VALID_ISSUES:
        return "invalid_report", f"Unknown reported_issue: {reported_issue or '(blank)'}"
    if word not in dictionary:
        return "add_to_database", "Word is absent from words_processed.csv."
    if reported_issue == "missing_from_database":
        return "resolved", "Word is now present in words_processed.csv."
    if not puzzle_word:
        return "needs_puzzle_word", "Dictionary word is present, but the historical report omitted the puzzle word."
    if not puzzle_word.isalpha() or not puzzle_word.isascii():
        return "invalid_report", "Puzzle word must contain only ASCII letters."
    if not can_spell(word, puzzle_word):
        return "invalid_for_puzzle", f"{word} cannot be formed from {puzzle_word}."
    return "add_to_puzzle_or_bonus", "Valid spellable dictionary word was reported missing from both lists."


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--issues", type=Path, default=DEFAULT_ISSUES)
    parser.add_argument("--dictionary", type=Path, default=DEFAULT_DICTIONARY)
    parser.add_argument("--output", type=Path, help="Optionally write the classified rows as CSV.")
    args = parser.parse_args()

    dictionary = load_dictionary(args.dictionary)
    with args.issues.open(encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.DictReader(handle))

    results = []
    for row in rows:
        action, detail = classify(row, dictionary)
        results.append({**row, "action": action, "detail": detail})
        print(f"{row.get('word', '')}: {action} - {detail}")

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        fields = list(rows[0]) + ["action", "detail"] if rows else ["word", "puzzle_word", "reported_issue", "notes", "action", "detail"]
        with args.output.open("w", encoding="utf-8", newline="") as handle:
            writer = csv.DictWriter(handle, fieldnames=fields, lineterminator="\n")
            writer.writeheader()
            writer.writerows(results)

    return 1 if any(row["action"] == "invalid_report" for row in results) else 0


if __name__ == "__main__":
    sys.exit(main())

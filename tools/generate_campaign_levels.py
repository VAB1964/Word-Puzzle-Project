"""Generate prevalidated single-player mobile campaigns.

The generator is deterministic. Re-running it against the same dictionary and
seeds produces the same catalogs, which makes campaign saves reproducible.
"""

from __future__ import annotations

import csv
import json
import random
from collections import Counter
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DICTIONARY = ROOT / "words_processed.csv"
OUTPUT = ROOT / "web" / "src" / "data" / "campaign-levels.json"
SEED = 20261016
CAMPAIGN_COUNT = 8
PUZZLES_PER_LEVEL = 10
MAX_REQUIRED_WORDS = 9
COMMON_WORD_MAX_RARITY = 2
# Each tuple is (rack length, minimum answer length, maximum answer length).
# Keeping this progression explicit makes the six-level campaign structure easy
# to review and change without relying on difficulty formulas.
LEVEL_RULES = (
    (3, 3, 3),
    (4, 3, 4),
    (5, 3, 5),
    (6, 3, 6),
    (7, 3, 7),
    (7, 4, 7),
)
# A fixed schedule keeps both game modes represented in every group of ten.
CROSSWORD_PUZZLES = frozenset({5, 8, 10})
# These connected word sets do not place completely with the app's
# deterministic crossword-board generator, so they intentionally remain Casual.
UNSUITABLE_CROSSWORDS: set[tuple[int, int]] = set()


@dataclass(frozen=True)
class Word:
    text: str
    rarity: int


def can_build(word: str, rack: str) -> bool:
    rack_counts = Counter(rack)
    return all(count <= rack_counts[letter] for letter, count in Counter(word).items())


def canonical(word: str) -> str:
    return "".join(sorted(word))


def intersection_graph_is_connected(words: list[str]) -> bool:
    if len(words) < 2:
        return False
    remaining = set(range(1, len(words)))
    reached = {0}
    while remaining:
        added = {
            index
            for index in remaining
            if any(set(words[index]) & set(words[other]) for other in reached)
        }
        if not added:
            return False
        reached.update(added)
        remaining.difference_update(added)
    return True


def load_words() -> list[Word]:
    result: list[Word] = []
    with DICTIONARY.open(encoding="utf-8", newline="") as handle:
        for row in csv.DictReader(handle):
            text = row["word"].strip().lower()
            if not (3 <= len(text) <= 7 and text.isalpha()):
                continue
            try:
                rarity = int(row.get("rarity", "4"))
            except ValueError:
                rarity = 4
            if 1 <= rarity <= 4:
                result.append(Word(text, rarity))
    return result


def choose_required_words(
    rack: Word,
    pool: list[Word],
    minimum_length: int,
    maximum_length: int,
) -> list[Word]:
    candidates = [
        word
        for word in pool
        if minimum_length <= len(word.text) <= maximum_length
        and word.rarity <= COMMON_WORD_MAX_RARITY
        and can_build(word.text, rack.text)
    ]
    if rack not in candidates:
        candidates.append(rack)

    # Prefer common and longer answers. The lexical tie-breaker keeps generation
    # stable, while reserving a slot guarantees that the rack word is playable.
    candidates.sort(key=lambda word: (word.rarity, -len(word.text), word.text))
    selected = candidates[:MAX_REQUIRED_WORDS]
    if rack not in selected:
        selected[-1] = rack
        selected.sort(key=lambda word: (word.rarity, -len(word.text), word.text))
    return selected


def generate_campaign(
    words: list[Word], seed: int, campaign_number: int
) -> list[dict]:
    roots_by_length = {
        length: [word for word in words if len(word.text) == length]
        for length in range(3, 8)
    }
    rng = random.Random(seed)
    used_letter_sets: set[str] = set()
    levels: list[dict] = []

    for group_index, (
        rack_length,
        minimum_length,
        maximum_length,
    ) in enumerate(LEVEL_RULES):
        for puzzle_index in range(1, PUZZLES_PER_LEVEL + 1):
            level_number = group_index * PUZZLES_PER_LEVEL + puzzle_index
            wants_crossword = (
                puzzle_index in CROSSWORD_PUZZLES
                and (campaign_number, level_number) not in UNSUITABLE_CROSSWORDS
            )

            candidates = [
                root
                for root in roots_by_length[rack_length]
                if root.rarity <= COMMON_WORD_MAX_RARITY
                and canonical(root.text) not in used_letter_sets
            ]
            rng.shuffle(candidates)

            chosen_rack: Word | None = None
            chosen_words: list[Word] = []
            for candidate in candidates:
                required = choose_required_words(
                    candidate,
                    words,
                    minimum_length,
                    maximum_length,
                )
                if not required:
                    continue
                if wants_crossword and not intersection_graph_is_connected(
                    [word.text for word in required]
                ):
                    continue
                chosen_rack = candidate
                chosen_words = required
                break

            if chosen_rack is None:
                raise RuntimeError(
                    f"No campaign candidate for level {level_number} "
                    f"(rack {rack_length}, max word {maximum_length})"
                )

            used_letter_sets.add(canonical(chosen_rack.text))
            levels.append(
                {
                    "level": level_number,
                    "mode": "Crossword" if wants_crossword else "Casual",
                    "rackWord": chosen_rack.text,
                    "requiredWords": [word.text for word in chosen_words],
                    "rackLength": rack_length,
                    "minimumRequiredLength": minimum_length,
                    "maximumRequiredLength": maximum_length,
                    "maximumRarity": max(
                        word.rarity for word in chosen_words
                    ),
                }
            )

    return levels


def generate() -> dict:
    words = load_words()
    campaigns = [
        {
            "id": index,
            "seed": SEED + index * 7919,
            "levels": generate_campaign(
                words, SEED + index * 7919, index + 1
            ),
        }
        for index in range(CAMPAIGN_COUNT)
    ]
    return {
        "version": 2,
        "structure": {
            "levels": len(LEVEL_RULES),
            "puzzlesPerLevel": PUZZLES_PER_LEVEL,
            "maximumRequiredWords": MAX_REQUIRED_WORDS,
        },
        # Keep the original field for compatibility and as Campaign 1.
        "levels": campaigns[0]["levels"],
        "campaigns": campaigns,
    }


def main() -> None:
    catalog = generate()
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(catalog, indent=2) + "\n", encoding="utf-8")
    print(
        f"Wrote {len(catalog['campaigns'])} campaigns "
        f"({len(catalog['levels'])} levels each) to {OUTPUT}"
    )


if __name__ == "__main__":
    main()

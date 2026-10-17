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
# These connected word sets do not place completely with the app's
# deterministic crossword-board generator, so they intentionally remain Casual.
UNSUITABLE_CROSSWORDS = {(6, 58), (7, 45)}


@dataclass(frozen=True)
class Word:
    text: str
    rarity: int


def stage_size(rack_length: int) -> int:
    return 5 * (rack_length - 2) if rack_length < 7 else 20


def stage_start(rack_length: int) -> int:
    if rack_length == 3:
        return 1
    if rack_length == 7:
        return 51
    return 1 + sum(stage_size(length) for length in range(3, rack_length))


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
    maximum_length: int,
    maximum_rarity: int,
    target_count: int,
    rng: random.Random,
) -> list[Word]:
    candidates = [
        word
        for word in pool
        if len(word.text) <= maximum_length
        and word.rarity <= maximum_rarity
        and can_build(word.text, rack.text)
    ]
    if maximum_length == len(rack.text) and rack not in candidates:
        candidates.append(rack)

    # Common words come first, with a deterministic shuffle inside each tier.
    by_tier: dict[tuple[int, int], list[Word]] = {}
    for word in candidates:
        by_tier.setdefault((word.rarity, len(word.text)), []).append(word)
    for values in by_tier.values():
        rng.shuffle(values)

    selected: list[Word] = []
    for rarity in range(1, maximum_rarity + 1):
        for length in range(maximum_length, 2, -1):
            for word in by_tier.get((rarity, length), []):
                if word not in selected:
                    selected.append(word)
                    if len(selected) == target_count:
                        return selected
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

    for rack_length in range(3, 8):
        count = stage_size(rack_length)
        start = stage_start(rack_length)
        for stage_index in range(count):
            level_number = start + stage_index
            progress = stage_index / max(1, count - 1)
            maximum_length = min(
                rack_length,
                3 + int((rack_length - 2) * progress),
            )
            target_count = 1 + (rack_length - 3) + int(4 * progress + 0.999999)
            maximum_rarity = min(4, 1 + int(3 * progress))
            wants_crossword = (
                level_number > 3
                and level_number % 5 in (0, 3)
                and (campaign_number, level_number) not in UNSUITABLE_CROSSWORDS
            )

            candidates = [
                root
                for root in roots_by_length[rack_length]
                if root.rarity <= maximum_rarity
                and canonical(root.text) not in used_letter_sets
            ]
            rng.shuffle(candidates)

            chosen_rack: Word | None = None
            chosen_words: list[Word] = []
            for candidate in candidates:
                required = choose_required_words(
                    candidate,
                    words,
                    maximum_length,
                    maximum_rarity,
                    target_count,
                    rng,
                )
                minimum_count = 1 if level_number == 1 else min(target_count, 2)
                if len(required) < minimum_count:
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
                    "maximumRequiredLength": maximum_length,
                    "maximumRarity": maximum_rarity,
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
        "version": 1,
        "formula": {
            "stageSize": "5 * (rackLength - 2), with repeating 20-level chapters at 7",
            "maximumRequiredLength": "3 + floor((rackLength - 2) * stageProgress)",
            "targetWords": "1 + (rackLength - 3) + ceil(4 * stageProgress)",
            "maximumRarity": "1 + floor(3 * stageProgress)",
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

import argparse
import csv
from pathlib import Path


def is_acronym(word: str, lengths: set[int]) -> bool:
    return len(word) in lengths and word.isalpha() and word.isupper()


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Remove acronym entries by word length and case."
    )
    parser.add_argument(
        "--input",
        required=True,
        help="Input CSV path (must include a 'word' column).",
    )
    parser.add_argument(
        "--output",
        required=True,
        help="Output CSV path.",
    )
    parser.add_argument(
        "--lengths",
        default="3,4",
        help="Comma-separated acronym lengths to remove (default: 3,4).",
    )
    args = parser.parse_args()

    input_path = Path(args.input)
    output_path = Path(args.output)
    lengths = {int(item.strip()) for item in args.lengths.split(",") if item.strip()}

    with input_path.open("r", newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        fieldnames = reader.fieldnames
        if not fieldnames:
            raise SystemExit("No header found in CSV.")

        rows: list[dict[str, str]] = []
        removed = 0
        for row in reader:
            word = row.get("word", "")
            if is_acronym(word, lengths):
                removed += 1
                continue
            rows.append(row)

    with output_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    print(f"Removed acronyms: {removed}")
    print(f"Wrote: {output_path}")


if __name__ == "__main__":
    main()

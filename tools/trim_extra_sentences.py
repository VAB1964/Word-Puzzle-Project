import argparse
import csv
import os


def trim_sentence(text: str) -> str:
    if not text:
        return text
    # Keep only the first sentence segment before the pipe separator.
    return text.split("|", 1)[0].strip()


def build_default_output_path(input_path: str) -> str:
    base, ext = os.path.splitext(input_path)
    return f"{base}_single_sentence{ext}"


def process_file(input_path: str, output_path: str) -> None:
    with open(input_path, "r", newline="", encoding="utf-8", errors="replace") as infile:
        reader = csv.DictReader(infile)
        if not reader.fieldnames:
            raise ValueError("Input CSV has no header row.")

        if "Sentence" not in reader.fieldnames:
            raise ValueError("Input CSV does not contain a 'Sentence' column.")

        with open(output_path, "w", newline="", encoding="utf-8") as outfile:
            writer = csv.DictWriter(outfile, fieldnames=reader.fieldnames)
            writer.writeheader()
            for row in reader:
                row["Sentence"] = trim_sentence(row.get("Sentence", ""))
                writer.writerow(row)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Trim extra sentences separated by '|' in the Sentence column."
    )
    parser.add_argument(
        "--input",
        default="words_processed.csv",
        help="Path to input CSV (default: words_processed.csv)",
    )
    parser.add_argument(
        "--output",
        help="Path to output CSV (default: <input>_single_sentence.csv)",
    )
    parser.add_argument(
        "--in-place",
        action="store_true",
        help="Overwrite the input file in-place.",
    )

    args = parser.parse_args()

    input_path = args.input
    if args.in_place:
        output_path = input_path
    else:
        output_path = args.output or build_default_output_path(input_path)

    process_file(input_path, output_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

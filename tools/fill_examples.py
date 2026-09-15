import argparse
import csv
import re


def normalize_sentence(text):
    text = re.sub(r"\s+", " ", text).strip()
    if not text:
        return text
    if text[-1] not in ".!?":
        text += "."
    return text


def main():
    parser = argparse.ArgumentParser(
        description="Normalize existing examples; leave missing examples empty. Use clean_dictionary.py to select suitable source examples."
    )
    parser.add_argument("--input", required=True, help="Input CSV file.")
    parser.add_argument("--output", required=True, help="Output CSV file.")
    args = parser.parse_args()

    with open(args.input, "r", encoding="utf-8", newline="") as f_in, open(
        args.output, "w", encoding="utf-8", newline=""
    ) as f_out:
        reader = csv.DictReader(f_in)
        writer = csv.DictWriter(f_out, fieldnames=reader.fieldnames)
        writer.writeheader()
        for row in reader:
            examples = (row.get("examples") or "").strip()
            # A restatement of a definition is not an example of usage.
            row["examples"] = normalize_sentence(examples) if examples else ""
            writer.writerow(row)


if __name__ == "__main__":
    main()

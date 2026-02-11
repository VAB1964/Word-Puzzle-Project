import argparse
import csv
import json
import os
import sys


def flatten_forms(forms):
    if not forms:
        return ""
    parts = []
    for item in forms:
        if not isinstance(item, dict):
            continue
        form = item.get("form", "")
        tags = item.get("tags") or []
        if form and tags:
            parts.append(f"{form} ({', '.join(tags)})")
        elif form:
            parts.append(form)
    return " | ".join(parts)


def flatten_examples(examples):
    if not examples:
        return ""
    parts = []
    for item in examples:
        if isinstance(item, dict):
            text = item.get("text", "")
            if text:
                parts.append(text)
        elif isinstance(item, str):
            parts.append(item)
    return " | ".join(parts)


def sense_rows(entry, include_forms, include_etymology):
    word = entry.get("word", "")
    pos = entry.get("pos", "")
    forms = flatten_forms(entry.get("forms", [])) if include_forms else ""
    etymology_text = entry.get("etymology_text", "") if include_etymology else ""
    senses = entry.get("senses") or []

    if not senses:
        yield [
            word,
            pos,
            "",
            "",
            "",
            "",
            forms,
            etymology_text,
        ]
        return

    for index, sense in enumerate(senses, start=1):
        if not isinstance(sense, dict):
            continue
        glosses = sense.get("glosses") or []
        tags = sense.get("tags") or []
        examples = flatten_examples(sense.get("examples") or [])
        yield [
            word,
            pos,
            index,
            " | ".join(glosses),
            " | ".join(tags),
            examples,
            forms,
            etymology_text,
        ]


def iterate_rows(input_path, include_forms, include_etymology, max_rows, progress_every):
    row_count = 0
    with open(input_path, "r", encoding="utf-8") as handle:
        for line_number, line in enumerate(handle, start=1):
            line = line.strip()
            if not line:
                continue
            try:
                entry = json.loads(line)
            except json.JSONDecodeError:
                print(f"Skipping invalid JSON on line {line_number}", file=sys.stderr)
                continue
            for row in sense_rows(entry, include_forms, include_etymology):
                yield row
                row_count += 1
                if max_rows and row_count >= max_rows:
                    return
                if progress_every and row_count % progress_every == 0:
                    print(f"Wrote {row_count:,} rows...", file=sys.stderr)


def write_csv(output_path, rows, headers):
    with open(output_path, "w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(headers)
        for row in rows:
            writer.writerow(row)


def write_xlsx(output_path, rows, headers, sheet_name):
    try:
        import openpyxl
    except ImportError as exc:
        raise RuntimeError(
            "openpyxl is required for .xlsx output. Install it with: pip install openpyxl"
        ) from exc

    workbook = openpyxl.Workbook(write_only=True)
    sheet = workbook.create_sheet(title=sheet_name)
    sheet.append(headers)
    for row in rows:
        sheet.append(row)
    workbook.save(output_path)


def main():
    parser = argparse.ArgumentParser(
        description="Extract Kaikki.org English JSONL into an Excel-friendly sheet."
    )
    parser.add_argument(
        "--input",
        default=r"c:\Users\vince\Downloads\kaikki.org-dictionary-English.jsonl",
        help="Path to the Kaikki.org JSONL file.",
    )
    parser.add_argument(
        "--output",
        default="kaikki_english.xlsx",
        help="Output path (.xlsx or .csv).",
    )
    parser.add_argument(
        "--max-rows",
        type=int,
        default=200000,
        help="Maximum number of rows to write (0 for all).",
    )
    parser.add_argument(
        "--progress-every",
        type=int,
        default=50000,
        help="Print progress every N rows (0 to disable).",
    )
    parser.add_argument(
        "--include-forms",
        action="store_true",
        help="Include inflected/alternative forms in the output.",
    )
    parser.add_argument(
        "--include-etymology",
        action="store_true",
        help="Include etymology text in the output (can be large).",
    )
    parser.add_argument(
        "--sheet-name",
        default="Kaikki",
        help="Sheet name for .xlsx output.",
    )
    args = parser.parse_args()

    if not os.path.isfile(args.input):
        raise FileNotFoundError(f"Input file not found: {args.input}")

    headers = [
        "word",
        "pos",
        "sense_index",
        "glosses",
        "tags",
        "examples",
        "forms",
        "etymology_text",
    ]

    rows = iterate_rows(
        args.input,
        include_forms=args.include_forms,
        include_etymology=args.include_etymology,
        max_rows=None if args.max_rows == 0 else args.max_rows,
        progress_every=None if args.progress_every == 0 else args.progress_every,
    )

    output_ext = os.path.splitext(args.output)[1].lower()
    if output_ext == ".csv":
        write_csv(args.output, rows, headers)
    elif output_ext == ".xlsx":
        write_xlsx(args.output, rows, headers, args.sheet_name)
    else:
        raise ValueError("Output must end with .xlsx or .csv")


if __name__ == "__main__":
    main()

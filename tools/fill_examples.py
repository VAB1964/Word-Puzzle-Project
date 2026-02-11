import argparse
import csv
import re


def first_gloss(glosses):
    if not glosses:
        return ""
    return glosses.split(" | ")[0].strip()


def normalize_sentence(text):
    text = re.sub(r"\s+", " ", text).strip()
    if not text:
        return text
    if text[-1] not in ".!?":
        text += "."
    return text


def make_sentence(word, pos, gloss):
    if not gloss:
        return normalize_sentence(f"{word} is a word")
    gloss_lower = gloss.lower()
    if pos == "verb":
        if gloss_lower.startswith("to "):
            return normalize_sentence(f"To {word} is {gloss}")
        return normalize_sentence(f"To {word} is to {gloss}")
    if pos in {"noun", "adjective", "adverb"}:
        return normalize_sentence(f"{word} means {gloss}")
    return normalize_sentence(f"{word} means {gloss}")


def main():
    parser = argparse.ArgumentParser(
        description="Fill missing example sentences using the gloss."
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
            if not examples:
                word = (row.get("word") or "").strip()
                pos = (row.get("pos") or "").strip().lower()
                gloss = first_gloss((row.get("glosses") or "").strip())
                if word:
                    row["examples"] = make_sentence(word, pos, gloss)
            writer.writerow(row)


if __name__ == "__main__":
    main()

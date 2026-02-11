#!/usr/bin/env python3
"""
Expand words_processed.csv with plural, past tense, and -ing forms.
Supports checkpointing for resumable runs across multiple sessions.
"""

import argparse
import csv
import json
import os

# Optional: try lemminflect first, fall back to rules-based
try:
    from lemminflect import getInflection
    HAS_LEMMINFLECT = True
except ImportError:
    HAS_LEMMINFLECT = False

import inflect


# Common irregular verb forms for fallback when lemminflect unavailable
IRREGULAR_VERBS = {
    "be": ("was", "were", "been", "being"),
    "bear": ("bore", "borne", "bearing"),
    "beat": ("beat", "beaten", "beating"),
    "become": ("became", "become", "becoming"),
    "begin": ("began", "begun", "beginning"),
    "bet": ("bet", "bet", "betting"),
    "bite": ("bit", "bitten", "biting"),
    "blow": ("blew", "blown", "blowing"),
    "break": ("broke", "broken", "breaking"),
    "bring": ("brought", "brought", "bringing"),
    "build": ("built", "built", "building"),
    "buy": ("bought", "bought", "buying"),
    "catch": ("caught", "caught", "catching"),
    "choose": ("chose", "chosen", "choosing"),
    "come": ("came", "come", "coming"),
    "cut": ("cut", "cut", "cutting"),
    "dig": ("dug", "dug", "digging"),
    "do": ("did", "done", "doing"),
    "draw": ("drew", "drawn", "drawing"),
    "drink": ("drank", "drunk", "drinking"),
    "drive": ("drove", "driven", "driving"),
    "eat": ("ate", "eaten", "eating"),
    "fall": ("fell", "fallen", "falling"),
    "feed": ("fed", "fed", "feeding"),
    "feel": ("felt", "felt", "feeling"),
    "fight": ("fought", "fought", "fighting"),
    "find": ("found", "found", "finding"),
    "fly": ("flew", "flown", "flying"),
    "forget": ("forgot", "forgotten", "forgetting"),
    "get": ("got", "gotten", "getting"),
    "give": ("gave", "given", "giving"),
    "go": ("went", "gone", "going"),
    "grow": ("grew", "grown", "growing"),
    "hang": ("hung", "hung", "hanging"),
    "have": ("had", "had", "having"),
    "hear": ("heard", "heard", "hearing"),
    "hide": ("hid", "hidden", "hiding"),
    "hit": ("hit", "hit", "hitting"),
    "hold": ("held", "held", "holding"),
    "hurt": ("hurt", "hurt", "hurting"),
    "keep": ("kept", "kept", "keeping"),
    "know": ("knew", "known", "knowing"),
    "lay": ("laid", "laid", "laying"),
    "lead": ("led", "led", "leading"),
    "leave": ("left", "left", "leaving"),
    "lend": ("lent", "lent", "lending"),
    "let": ("let", "let", "letting"),
    "lie": ("lay", "lain", "lying"),
    "lose": ("lost", "lost", "losing"),
    "make": ("made", "made", "making"),
    "mean": ("meant", "meant", "meaning"),
    "meet": ("met", "met", "meeting"),
    "pay": ("paid", "paid", "paying"),
    "put": ("put", "put", "putting"),
    "read": ("read", "read", "reading"),
    "ride": ("rode", "ridden", "riding"),
    "ring": ("rang", "rung", "ringing"),
    "rise": ("rose", "risen", "rising"),
    "run": ("ran", "run", "running"),
    "say": ("said", "said", "saying"),
    "see": ("saw", "seen", "seeing"),
    "sell": ("sold", "sold", "selling"),
    "send": ("sent", "sent", "sending"),
    "set": ("set", "set", "setting"),
    "shake": ("shook", "shaken", "shaking"),
    "shine": ("shone", "shone", "shining"),
    "shoot": ("shot", "shot", "shooting"),
    "show": ("showed", "shown", "showing"),
    "shut": ("shut", "shut", "shutting"),
    "sing": ("sang", "sung", "singing"),
    "sit": ("sat", "sat", "sitting"),
    "sleep": ("slept", "slept", "sleeping"),
    "speak": ("spoke", "spoken", "speaking"),
    "spend": ("spent", "spent", "spending"),
    "stand": ("stood", "stood", "standing"),
    "steal": ("stole", "stolen", "stealing"),
    "stick": ("stuck", "stuck", "sticking"),
    "swim": ("swam", "swum", "swimming"),
    "take": ("took", "taken", "taking"),
    "teach": ("taught", "taught", "teaching"),
    "tear": ("tore", "torn", "tearing"),
    "tell": ("told", "told", "telling"),
    "think": ("thought", "thought", "thinking"),
    "throw": ("threw", "thrown", "throwing"),
    "understand": ("understood", "understood", "understanding"),
    "wake": ("woke", "woken", "waking"),
    "wear": ("wore", "worn", "wearing"),
    "win": ("won", "won", "winning"),
    "write": ("wrote", "written", "writing"),
}


def _past_tense_rules(word: str) -> str | None:
    """Rule-based past tense (fallback when lemminflect unavailable)."""
    word = word.lower()
    if not word or len(word) < 2:
        return None
    if word in IRREGULAR_VERBS:
        forms = IRREGULAR_VERBS[word]
        return forms[0] if forms else None
    # Regular: -ed
    if word.endswith("e"):
        return word + "d"
    if len(word) >= 2 and word[-1] in "bdgmnprt" and word[-2] in "aeiou" and word[-1] == word[-1].lower():
        # double final consonant (simplified)
        if word[-2] != word[-1]:
            return word + word[-1] + "ed"
    if word.endswith("y") and len(word) >= 2 and word[-2] not in "aeiou":
        return word[:-1] + "ied"
    return word + "ed"


def _ing_form_rules(word: str) -> str | None:
    """Rule-based -ing form (fallback when lemminflect unavailable)."""
    word = word.lower()
    if not word or len(word) < 2:
        return None
    if word in IRREGULAR_VERBS:
        forms = IRREGULAR_VERBS[word]
        # Usually last element is -ing form
        for f in forms:
            if f.endswith("ing"):
                return f
        # fallback
        return word + "ing"
    # Drop final e
    if word.endswith("e") and len(word) >= 3 and word[-2] != "e":
        return word[:-1] + "ing"
    # Double consonant: CVC
    if len(word) >= 3 and word[-1] in "bdgmnprt" and word[-2] in "aeiou" and word[-3] not in "aeiou":
        return word + word[-1] + "ing"
    return word + "ing"


def get_past_tense(word: str) -> str | None:
    """Return past tense form, or None if not applicable."""
    w = word.lower()
    if HAS_LEMMINFLECT:
        result = getInflection(w, tag="VBD")
        return result[0] if result else _past_tense_rules(word)
    return _past_tense_rules(word)


def get_ing_form(word: str) -> str | None:
    """Return -ing form, or None if not applicable."""
    w = word.lower()
    if HAS_LEMMINFLECT:
        result = getInflection(w, tag="VBG")
        return result[0] if result else _ing_form_rules(word)
    return _ing_form_rules(word)


def is_already_plural(row: dict, p: inflect.engine) -> bool:
    """Return True if the word is already a plural form."""
    word = (row.get("word") or "").strip()
    definition = (row.get("Definition") or "").lower()
    if not word:
        return False
    if "plural of" in definition:
        return True
    if p.plural(word.lower()) == word.lower():
        return True
    return False


def load_input_rows(path: str) -> list[dict]:
    """Load all CSV rows into memory."""
    rows = []
    with open(path, "r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        fieldnames = reader.fieldnames or []
        for row in reader:
            rows.append(dict(row))
    return rows, fieldnames


def build_valid_words(rows: list[dict], extra_path: str | None = None) -> set[str]:
    """Build set of all words from input (lowercase) for verification. Optionally add words from extra file."""
    words = set()
    for row in rows:
        w = (row.get("word") or "").strip().lower()
        if w:
            words.add(w)
    if extra_path and os.path.exists(extra_path):
        with open(extra_path, "r", encoding="utf-8", newline="") as f:
            reader = csv.DictReader(f)
            for row in reader:
                w = (row.get("ngram") or row.get("word") or "").strip().lower()
                if w and w.isalpha():
                    words.add(w)
        print(f"Loaded extra dictionary from {extra_path}, {len(words)} total valid words")
    return words


def load_checkpoint(path: str) -> int | None:
    """Load last processed index from checkpoint. Returns None if no checkpoint."""
    if not os.path.exists(path):
        return None
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        return data.get("last_processed_index")
    except (json.JSONDecodeError, OSError):
        return None


def save_checkpoint(path: str, last_index: int, total_rows: int) -> None:
    """Save checkpoint."""
    with open(path, "w", encoding="utf-8") as f:
        json.dump({"last_processed_index": last_index, "total_input_rows": total_rows}, f, indent=2)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Expand words_processed.csv with plural, past tense, and -ing forms. Resumable via checkpoint."
    )
    parser.add_argument("--input", "-i", default="words_processed.csv", help="Input CSV file.")
    parser.add_argument("--output", "-o", default="words_processed_expanded.csv", help="Output CSV file.")
    parser.add_argument("--checkpoint-file", "-c", default="word_variants_checkpoint.json", help="Checkpoint file path.")
    parser.add_argument("--checkpoint-interval", "-n", type=int, default=500, help="Save checkpoint every N rows.")
    parser.add_argument("--dictionary", "-d", default=None, help="Optional extra word list (CSV with 'ngram' or 'word' column) for verification.")
    parser.add_argument("--limit", "-l", type=int, default=None, help="Process only first N rows (for testing).")
    args = parser.parse_args()

    input_path = args.input
    output_path = args.output
    checkpoint_path = args.checkpoint_file
    interval = args.checkpoint_interval

    if not os.path.exists(input_path):
        print(f"Error: Input file not found: {input_path}")
        return

    all_rows, fieldnames = load_input_rows(input_path)
    valid_words = build_valid_words(all_rows, args.dictionary)
    rows = all_rows[: args.limit] if args.limit is not None else all_rows
    p = inflect.engine()
    start_index = 0
    write_header = True

    last_check = load_checkpoint(checkpoint_path)
    written_words: set[str] = set()
    if last_check is not None:
        start_index = last_check + 1
        write_header = False
        # Load already-written words from output file to avoid duplicate variants
        if os.path.exists(output_path):
            with open(output_path, "r", encoding="utf-8", newline="") as f_existing:
                reader = csv.DictReader(f_existing)
                for r in reader:
                    w = (r.get("word") or "").strip().lower()
                    if w:
                        written_words.add(w)
        print(f"Resuming from row {start_index} (checkpoint: last_processed_index={last_check}, {len(written_words)} words already written)")

    if not fieldnames:
        fieldnames = ["word", "rarity", "pos", "Definition", "Sentence"]

    mode = "a" if start_index > 0 else "w"

    with open(output_path, mode, encoding="utf-8", newline="") as f_out:
        writer = csv.DictWriter(f_out, fieldnames=fieldnames)
        if write_header:
            writer.writeheader()

        for i, row in enumerate(rows):
            if i < start_index:
                continue

            word = (row.get("word") or "").strip()
            pos = (row.get("pos") or "").strip().lower()
            rarity = row.get("rarity", "")
            definition = row.get("Definition", "")
            sentence = row.get("Sentence", "")

            # Write original row (on resume we only process new rows, so we always write)
            writer.writerow(row)
            written_words.add(word.lower())

            variants_to_add: list[tuple[str, str]] = []  # (word, variant_type)

            def can_add_variant(var: str) -> bool:
                """Only add variant if valid, not yet written, and not already an input base row."""
                if not var or var in written_words:
                    return False
                if var not in valid_words:
                    return False
                # Don't add if variant is an input base word - we'll write it when we process that row
                if any((r.get("word") or "").strip().lower() == var for r in all_rows):
                    return False
                return True

            # Nouns: plural (if not already plural)
            if pos == "noun" and word and not is_already_plural(row, p):
                plural = p.plural(word.lower())
                if plural and plural != word.lower() and can_add_variant(plural):
                    variants_to_add.append((plural, "plural"))

            # Verbs: past tense and -ing
            if pos == "verb" and word:
                past = get_past_tense(word)
                if past and can_add_variant(past):
                    variants_to_add.append((past, "past"))
                ing = get_ing_form(word)
                if ing and can_add_variant(ing):
                    variants_to_add.append((ing, "ing"))

            for var_word, var_type in variants_to_add:
                var_def = f"{var_type.capitalize()} of {word}."
                var_sent = f"{var_word} means {var_def}"
                writer.writerow({
                    "word": var_word,
                    "rarity": rarity,
                    "pos": pos,
                    "Definition": var_def,
                    "Sentence": var_sent,
                })
                written_words.add(var_word.lower())

            if (i + 1) % interval == 0:
                save_checkpoint(checkpoint_path, i, len(rows))
                print(f"Processed {i + 1}/{len(rows)} rows, checkpoint saved.")

        save_checkpoint(checkpoint_path, len(rows) - 1, len(rows))
        print(f"Done. Processed {len(rows)} rows. Output: {output_path}")


if __name__ == "__main__":
    main()

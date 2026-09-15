#!/usr/bin/env python3
"""Refresh the pinned ESDB vocabulary snapshot (requires an ESDB checkout/database)."""

import argparse
import csv
import hashlib
import io
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UPSTREAM_COMMIT = "1e5b7d3a72f47a71da5d28686c1dd4b397178485"
OPTIONS = [
    "--deaccent", "--wo-poses=abbr",
    "--wo-pos-classes=person,surname,place,name,demonym,trademark,abbr,upper,name?,upper?,abbr?",
    "--wo-pos-categories=nonword,wordpart", "--categories=",
    "--wo-usage-notes=offensive-1,offensive-2,vulgar-1,vulgar-3,nonstandard",
]


def text_hash(text):
    return hashlib.sha256(text.replace("\r\n", "\n").encode("utf-8")).hexdigest()


def export(checkout, output):
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=checkout, text=True).strip()
    if revision != UPSTREAM_COMMIT:
        raise ValueError(f"Expected ESDB commit {UPSTREAM_COMMIT}; got {revision}")
    if subprocess.check_output(["git", "status", "--porcelain", "--untracked-files=no"], cwd=checkout):
        raise ValueError("ESDB checkout has modified tracked files")
    # Rebuild from the checked-out source, so an old database cannot masquerade as
    # the pinned revision. The official build works directly with Python on Windows.
    subprocess.run([sys.executable, "combine.py", "create-db", "scowl.db"], cwd=checkout, check=True)
    output.mkdir(parents=True, exist_ok=True)
    files, counts, previous = {}, {}, set()
    for size in (60, 70, 80):
        raw = subprocess.check_output(
            [sys.executable, "scowl", "word-list", str(size), "A", "1", *OPTIONS],
            cwd=checkout, text=True, encoding="utf-8",
        )
        # Never lowercase a capitalized name into a playable word.
        words = {word for word in raw.splitlines() if re.fullmatch(r"[a-z]{3,7}", word)}
        if not previous <= words:
            raise ValueError("ESDB levels are not cumulative")
        filename = "american-60.txt" if size == 60 else f"american-{size}-additions.txt"
        files[filename] = "\n".join(sorted(words - previous)) + "\n"
        counts[str(size)] = len(words)
        if size == 60:
            base_words = words
        previous = words

    # Use upstream's own query builder, with exactly the same metadata filters.
    sys.path.insert(0, str(checkout))
    import libscowl
    from libscowl._search import queryString
    query = queryString(
        size=60, spellings=["A"], variantLevel=1, poses=libscowl.Exclude("abbr"),
        posClasses=libscowl.Exclude(*OPTIONS[2].split("=", 1)[1].split(",")),
        posCategories=libscowl.Exclude("nonword", "wordpart"), categories=[""],
        usageNotes=libscowl.Exclude(*OPTIONS[-1].split("=", 1)[1].split(",")),
    )
    conn = libscowl.openDB(str(checkout / "scowl.db"))
    sql = ("with eligible as (select word, lemma_id, pos from scowl_ " + query.where + ") "
           "select distinct e.word, l.word, e.pos from eligible e join words l on e.lemma_id=l.word_id")
    forms = set()
    for word, lemma, pos in conn.execute(sql):
        word, lemma = libscowl.deaccent(word), libscowl.deaccent(lemma)
        if word in base_words and word != lemma and re.fullmatch(r"[a-z]+", lemma):
            forms.add((word, lemma, pos))
    conn.close()
    buffer = io.StringIO(newline="")
    writer = csv.writer(buffer, lineterminator="\n")
    writer.writerow(["word", "lemma", "esdb_pos"])
    writer.writerows(sorted(forms))
    files["inflections-60.csv"] = buffer.getvalue()
    files["Copyright"] = (checkout / "Copyright").read_text(encoding="utf-8")
    for name, content in files.items():
        (output / name).write_text(content, encoding="utf-8", newline="\n")
    manifest = {
        "repository": "https://github.com/en-wl/wordlist", "commit": revision,
        "dialect": "American English", "spelling": "A", "variant_level": 1,
        "base_size": 60, "review_sizes": [70, 80], "options": OPTIONS,
        "post_filter": "full lowercase ASCII spelling, 3-7 letters; no lowercasing",
        "cumulative_counts": counts,
        "sha256_normalized_utf8": {name: text_hash(content) for name, content in files.items()},
    }
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(counts))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkout", type=Path)
    parser.add_argument("--output", type=Path, default=ROOT / "data/esdb")
    args = parser.parse_args()
    export(args.checkout.resolve(), args.output)

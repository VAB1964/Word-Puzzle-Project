import csv
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from extract_kaikki_candidates import (
    classify_word, enrich_candidates, extract, load_frequency_ranks,
    load_inflection_relations, rejection_reason,
)


def entry(word, senses, pos="noun", tags=None):
    return {"word": word, "pos": pos, "tags": tags or [], "senses": senses}


def sense(gloss, tags=None, example=""):
    result = {"glosses": [gloss], "tags": tags or []}
    if example:
        result["examples"] = [{"text": example}]
    return result


class KaikkiClassificationTests(unittest.TestCase):
    def test_single_modern_sense_is_safe_and_keeps_a_good_example(self):
        candidate = classify_word(
            "loader",
            [(12, entry("loader", [sense("A person or machine that loads something.", example="The loader filled the truck.")]))],
        )
        self.assertEqual(candidate["tier"], "safe")
        self.assertEqual(candidate["senses"][0]["example"], "The loader filled the truck.")
        self.assertEqual(candidate["senses"][0]["source_line"], 12)
        self.assertRegex(candidate["senses"][0]["sense_id"], r"^[0-9a-f]{64}$")
        self.assertEqual(candidate["frequency_rank"], None)
        self.assertEqual(candidate["unlocks"], [])

    def test_duplicate_senses_are_deduplicated_but_distinct_senses_are_ambiguous(self):
        duplicate = entry("bank", [sense("A financial institution.")])
        river = entry("bank", [sense("The land beside a river.")])
        candidate = classify_word("bank", [(1, duplicate), (2, duplicate), (3, river)])
        self.assertEqual(candidate["tier"], "ambiguous")
        self.assertEqual(len(candidate["senses"]), 2)

    def test_filters_names_fragments_abbreviations_bad_spellings_and_offensive_senses(self):
        fixtures = [
            ("name", "A personal name.", [], "proper-name-or-word-fragment"),
            ("suffix", "A word ending.", [], "proper-name-or-word-fragment"),
            ("noun", "Abbreviation of loader.", [], "abbreviation-only-sense"),
            ("noun", "Archaic spelling of load.", [], "obsolete-or-invalid-form"),
            ("noun", "A damaged \ufffd definition.", [], "missing-or-damaged-definition"),
            ("noun", "A fish.", [], "missing-or-damaged-definition"),
            ("noun", "Definition needed for this entry.", [], "missing-or-damaged-definition"),
            ("noun", "An insulting expression.", ["offensive"], "excluded-usage:offensive"),
        ]
        for pos, definition, tags, expected in fixtures:
            with self.subTest(definition=definition):
                self.assertEqual(rejection_reason(pos, definition, tags), expected)

    def test_single_rare_sense_stays_manual(self):
        candidate = classify_word("rara", [(4, entry("rara", [sense("A regional object.", ["rare"])]))])
        self.assertEqual(candidate["tier"], "manual")
        self.assertEqual(candidate["classification_reason"], "usage-needs-review")

    def test_rejected_secondary_sense_does_not_block_an_ordinary_homograph(self):
        candidate = classify_word("spade", [(5, entry("spade", [
            sense("A tool for digging soil."),
            sense("An insulting term.", ["offensive"]),
        ]))])
        self.assertEqual(candidate["tier"], "safe")
        self.assertEqual(candidate["flags"], [])
        self.assertEqual(candidate["rejected_senses"][0]["reason"], "excluded-usage:offensive")

    def test_grammatical_form_is_not_auto_safe(self):
        form = sense("Plural of loader.", ["form-of"])
        form["form_of"] = [{"word": "loader"}]
        candidate = classify_word("loaders", [(8, entry("loaders", [form]))])
        self.assertEqual(candidate["tier"], "manual")
        self.assertIn("grammatical-form", candidate["flags"])


class KaikkiStreamingTests(unittest.TestCase):
    def test_enriches_frequency_rank_and_actual_pending_inflection_unlocks(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            frequencies = root / "frequency.csv"
            frequencies.write_text("ngram,freq\nother,500\nloader,100\nload,200\n", encoding="utf-8")
            relations = root / "inflections.csv"
            relations.write_text(
                "word,lemma,esdb_pos\nloaders,loader,ns\nloaded,load,vd\nloading,load,vg\nignored,loader,unknown\n",
                encoding="utf-8",
            )
            candidates = [
                classify_word("load", [(1, entry("load", [sense("To put material onto something.")], "verb"))]),
                classify_word("loader", [(2, entry("loader", [sense("A device that loads material.")]))]),
            ]
            enrich_candidates(
                candidates, {"load", "loader", "loaders", "loaded", "loading", "ignored"},
                load_frequency_ranks(frequencies), load_inflection_relations(relations),
            )
            by_word = {item["word"]: item for item in candidates}
            self.assertEqual(by_word["load"]["frequency_rank"], 2)
            self.assertEqual(by_word["loader"]["frequency_rank"], 3)
            self.assertEqual(by_word["load"]["unlocks"], ["loaded", "loading"])
            self.assertEqual(by_word["loader"]["unlocks"], ["loaders"])

    def test_extract_keeps_only_exact_pending_records_and_reports_unmatched_words(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            pending = root / "pending.csv"
            with pending.open("w", encoding="utf-8", newline="") as handle:
                writer = csv.DictWriter(handle, fieldnames=["word", "reason"])
                writer.writeheader()
                writer.writerows([
                    {"word": "loader", "reason": "review"},
                    {"word": "bank", "reason": "review"},
                    {"word": "absent", "reason": "review"},
                ])
            source = root / "english.jsonl"
            records = [
                entry("unlisted", [sense("Not requested.")]),
                entry("Loader", [sense("A proper name.")], pos="name"),
                entry("loader", [sense("A person or machine that loads something.")]),
                entry("bank", [sense("A financial institution."), sense("The land beside a river.")]),
            ]
            source.write_text("".join(json.dumps(row) + "\n" for row in records), encoding="utf-8")
            candidates, summary = extract(source, pending, root / "output")

            self.assertEqual(summary["kaikki_entries_scanned"], 4)
            self.assertEqual(summary["matched_entries"], 2)
            self.assertEqual(summary["unmatched_words"], 1)
            by_word = {row["word"]: row for row in candidates}
            self.assertEqual(by_word["loader"]["tier"], "safe")
            self.assertEqual(by_word["bank"]["tier"], "ambiguous")
            self.assertEqual(by_word["absent"]["classification_reason"], "not-found-in-kaikki")
            retained = (root / "output/matched-kaikki-entries.jsonl").read_text(encoding="utf-8")
            self.assertIn('"word":"loader"', retained)
            self.assertNotIn('"word":"Loader"', retained)
            written = [json.loads(line) for line in (root / "output/candidate-review.jsonl").read_text(encoding="utf-8").splitlines()]
            self.assertEqual([row["word"] for row in written], ["absent", "bank", "loader"])
            catalog = json.loads((root / "output/candidate-catalog.json").read_text(encoding="utf-8"))
            self.assertEqual(catalog["schema_version"], 1)
            self.assertRegex(catalog["source"]["sha256"], r"^[0-9a-f]{64}$")
            self.assertEqual(catalog["source"]["license"], "CC BY-SA 4.0")

    def test_malformed_json_reports_the_source_line(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "pending.csv").write_text("word\nloader\n", encoding="utf-8")
            (root / "bad.jsonl").write_text("{}\nnot-json\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "line 2"):
                extract(root / "bad.jsonl", root / "pending.csv", root / "output")


if __name__ == "__main__":
    unittest.main()

import csv
import io
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from build_esdb_dictionary import BASELINE, ROOT, build, inflection, load_snapshot


def entry(word, pos="noun", rarity="2", definition="A useful meaning."):
    return dict(word=word, pos=pos, rarity=rarity, Definition=definition, Sentence="")


EMPTY = {"larger_level_approvals": {}, "new_definitions": {}}


class EsdbPolicyTests(unittest.TestCase):
    def test_no_undefined_words_or_unapproved_larger_levels_enter_puzzles(self):
        rows, pending, removed, review, _, _ = build(
            [entry("cat"), entry("xeric", "adj", "4")], {"cat", "aisle"},
            {"xeric": 70}, [], EMPTY, set())
        self.assertEqual([row["word"] for row in rows], ["cat"])
        self.assertEqual(pending[0]["word"], "aisle")
        self.assertEqual(removed[0]["reason"], "larger-level-not-approved")
        self.assertEqual(review[0]["decision"], "deferred")

    def test_levels_do_not_set_rarity_and_retained_records_are_unchanged(self):
        old = [entry("cat", rarity="3"), entry("xeric", "adj", "4")]
        editorial = {**EMPTY, "larger_level_approvals": {"xeric": {"level": 70, "reason": "Reviewed ecological term."}}}
        rows, _, _, _, provenance, _ = build(old, {"cat"}, {"xeric": 70}, [], editorial, set())
        self.assertEqual(rows, old)
        self.assertEqual([p["rarity_basis"] for p in provenance], ["preserved", "preserved"])

    def test_uses_attested_irregular_forms_without_inventing_suffixes(self):
        old = [entry("mouse", rarity="3")]
        forms = [{"word": "mice", "lemma": "mouse", "esdb_pos": "ns"}]
        rows, pending, _, _, provenance, _ = build(old, {"mouse", "mice", "mouses"}, {}, forms, EMPTY, set())
        mice = next(row for row in rows if row["word"] == "mice")
        self.assertEqual(mice["Definition"], "Plural of mouse.")
        self.assertEqual(mice["rarity"], "3")
        self.assertEqual(mice["Sentence"], "")
        self.assertEqual(pending[0]["word"], "mouses")
        self.assertEqual(provenance[0]["lemma"], "mouse")

    def test_does_not_attach_an_unrelated_homograph_meaning(self):
        root = entry("lie", "verb", "2", "To recline.")
        forms = [{"word": "lied", "lemma": "lie", "esdb_pos": pos} for pos in ("vd", "vn")]
        row, lemma = inflection("lied", forms, {"lie": root})
        self.assertEqual(row["Definition"], "Past tense and past participle of lie.")
        self.assertNotIn("recline", row["Definition"])
        self.assertEqual(lemma, "lie")

    def test_esdb_can_attest_a_different_pos_from_the_roots_displayed_sense(self):
        row, _ = inflection("acted", [{"word": "acted", "lemma": "act", "esdb_pos": "vd"}], {"act": entry("act")})
        self.assertEqual(row["pos"], "verb")
        self.assertEqual(row["Definition"], "Past tense of act.")

    def test_rejects_explicit_exclusions_even_when_esdb_lists_them(self):
        rows, pending, _, _, _, _ = build([entry("cat")], {"cat"}, {}, [], EMPTY, {"cat"})
        self.assertEqual(rows, [])
        self.assertEqual(pending[0]["reason"], "explicit-editorial-exclusion")

    def test_adds_reviewed_supplements_without_weakening_explicit_exclusions(self):
        supplemental = {
            "zen": {**entry("zen", definition="A state of calm attentiveness."), "reason": "Reviewed."}
        }
        editorial = {**EMPTY, "supplemental_entries": supplemental}
        rows, _, _, _, provenance, summary = build([], set(), {}, [], editorial, set())
        self.assertEqual(rows, [entry("zen", definition="A state of calm attentiveness.")])
        self.assertEqual(provenance[0]["definition_source"], "editorial-supplement")
        self.assertEqual(summary["supplemental_outside_esdb"], 1)
        with self.assertRaisesRegex(ValueError, "conflicts with an explicit exclusion"):
            build([], set(), {}, [], editorial, {"zen"})

    def test_refuses_stale_editorial_approvals(self):
        editorial = {**EMPTY, "larger_level_approvals": {"xeric": {"level": 80, "reason": "Review"}}}
        with self.assertRaisesRegex(ValueError, "Incorrect larger-level"):
            build([entry("xeric")], set(), {"xeric": 70}, [], editorial, set())

    def test_detects_modified_snapshot_before_building(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            (path / "manifest.json").write_text(json.dumps({"sha256_normalized_utf8": {"american-60.txt": "incorrect"}}))
            (path / "american-60.txt").write_text("cat\n")
            with self.assertRaisesRegex(ValueError, "checksum mismatch"):
                load_snapshot(path)


class PublishedEsdbTests(unittest.TestCase):
    def test_published_dictionary_and_audits_match_the_reproducible_build(self):
        raw = subprocess.check_output(["git", "show", f"{BASELINE}:words_processed.csv"], cwd=ROOT, text=True, encoding="utf-8")
        baseline = list(csv.DictReader(io.StringIO(raw)))
        editorial = json.loads((ROOT / "tools/esdb_editorial.json").read_text())
        overrides = json.loads((ROOT / "tools/dictionary_overrides.json").read_text())
        excluded = {word for word, row in overrides["words"].items() if row.get("exclude")}
        base, larger, forms = load_snapshot(ROOT / "data/esdb")
        rows, pending, removed, review, provenance, summary = build(baseline, base, larger, forms, editorial, excluded)
        summary["baseline_commit"] = BASELINE
        for filename, expected in [("words_processed.csv", rows),
                                   ("docs/esdb-audit/pending-definitions.csv", pending),
                                   ("docs/esdb-audit/removed-words.csv", removed),
                                   ("docs/esdb-audit/larger-level-review.csv", review),
                                   ("docs/esdb-audit/word-provenance.csv", provenance)]:
            with (ROOT / filename).open(encoding="utf-8", newline="") as handle:
                actual = list(csv.DictReader(handle))
            self.assertEqual(actual, [{k: str(v) for k, v in row.items()} for row in expected], filename)
        self.assertEqual(summary, json.loads((ROOT / "docs/esdb-audit/summary.json").read_text()))
        self.assertEqual((ROOT / "Standalone/words_processed.csv").read_bytes(), (ROOT / "words_processed.csv").read_bytes())
        old = {row["word"]: row for row in baseline}
        supplemental = set(editorial.get("supplemental_entries", {}))
        for row in rows:
            if row["word"] in old and row["word"] not in supplemental:
                self.assertEqual(row, old[row["word"]])
        self.assertEqual(len(base), 28324)
        self.assertEqual(sum(level == 70 for level in larger.values()), 11559)
        self.assertEqual(sum(level == 80 for level in larger.values()), 11135)


if __name__ == "__main__":
    unittest.main()

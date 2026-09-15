import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from clean_dictionary import clean_rows, good_example, validate


def word(text, definition, rarity="2", pos="noun", sentence=""):
    return {"word": text, "rarity": rarity, "pos": pos, "Definition": definition, "Sentence": sentence}


def sense(text, definition, pos="noun", tags="", examples=""):
    return {"word": text, "pos": pos, "glosses": definition, "tags": tags, "examples": examples}


class DictionaryCleanupTests(unittest.TestCase):
    def test_brand_cannot_override_lowercase_word_or_rarity(self):
        rows = [word("Apple", "A computer.", "4"), word("apple", "A fruit.", "2")]
        cleaned, removed = clean_rows(rows, [sense("apple", "A fruit.")], {})
        self.assertEqual(cleaned, [word("apple", "A fruit.", "2")])
        self.assertEqual(removed[0]["reason"], "duplicate-after-lowercasing")

    def test_replaces_abbreviation_sense_without_rejecting_real_word(self):
        rows = [word("par", "Abbreviation of paragraph."), word("abt", "Initialism of all but thesis.")]
        source = [sense("par", "Abbreviation of paragraph.", tags="abbreviation"),
                  sense("par", "An expected standard."),
                  sense("abt", "Initialism of all but thesis.", tags="initialism")]
        cleaned, _ = clean_rows(rows, source, {})
        self.assertEqual(cleaned, [word("par", "An expected standard.")])

    def test_editorial_exception_repairs_true_and_preserves_sub(self):
        rows = [word("TRUE", "Correct.", "1", "adj"), word("sub", "Abbreviation of submarine.")]
        overrides = {"true": {"pos": "adj", "Definition": "Correct.", "Sentence": "The story is true."},
                     "sub": {"Definition": "A submarine.", "Sentence": ""}}
        cleaned, _ = clean_rows(rows, [sense("sub", "Abbreviation of submarine.", tags="abbreviation")], overrides)
        by_word = {r["word"]: r for r in cleaned}
        self.assertEqual(by_word["true"]["rarity"], "1")
        self.assertIn("sub", by_word)

    def test_uses_modern_sense_and_rejects_template_and_archaic_examples(self):
        rows = [word("train", "An old meaning.", sentence="train means An old meaning.")]
        source = [sense("train", "An old meaning.", tags="obsolete"),
                  sense("train", "A series of railway cars.", examples="Thou shalt catch the train. | She took the train home.")]
        cleaned, _ = clean_rows(rows, source, {})
        self.assertEqual(cleaned[0]["Sentence"], "She took the train home.")
        self.assertTrue(good_example("She can speak English.", "can"))
        self.assertFalse(good_example("The ſun is bright.", "sun"))

    def test_missing_source_is_not_evidence_of_invalid_inflection(self):
        rows = [word("meant", "simple past of mean", pos="verb")]
        cleaned, _ = clean_rows(rows, [], {})
        self.assertEqual(cleaned, rows)

    def test_does_not_guess_a_homographs_meaning(self):
        rows = [word("lied", "simple past of lie", pos="verb")]
        source = [sense("lie", "To recline.", "verb"), sense("lie", "To say something untrue.", "verb")]
        cleaned, _ = clean_rows(rows, source, {})
        self.assertEqual(cleaned[0]["Definition"], "simple past of lie")

    def test_does_not_truncate_multiword_reference(self):
        rows = [word("cos", "Synonym of romaine lettuce, a long-leaved lettuce.")]
        cleaned, _ = clean_rows(rows, [], {})
        self.assertEqual(cleaned, rows)

    def test_rejects_case_only_names_bad_spellings_and_reference_cycles(self):
        rows = [word("Vince", "A name."), word("i18n", "Internationalization."),
                word("decieve", "Misspelling of deceive."), word("corp", "Synonym of corp.")]
        source = [sense("decieve", "Misspelling of deceive.", tags="misspelling"),
                  sense("corp", "Synonym of corp.")]
        cleaned, removed = clean_rows(rows, source, {})
        self.assertEqual(cleaned, [])
        self.assertEqual(len(removed), 4)

    def test_cleanup_is_idempotent_with_expanded_meanings(self):
        rows = [word("did", "simple past of do", pos="verb", sentence="I did the work myself.")]
        source = [sense("did", "simple past of do", "verb")]
        reference = {"do": {"verb": "To perform an action."}}
        once, _ = clean_rows(rows, source, {}, reference)
        twice, removed = clean_rows(once, source, {}, reference)
        self.assertEqual(once, twice)
        self.assertEqual(removed, [])
        self.assertIn("Meaning:", once[0]["Definition"])
        self.assertEqual(once[0]["Sentence"], "I did the work myself.")
        validate(once)


if __name__ == "__main__":
    unittest.main()

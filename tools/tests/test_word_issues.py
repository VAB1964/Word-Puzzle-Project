import csv
import sys
import tempfile
import unittest
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from check_word_issues import classify, load_dictionary


class WordIssueTests(unittest.TestCase):
    def test_classifies_database_and_puzzle_actions(self):
        dictionary = {"rinse", "tinge"}
        self.assertEqual(
            classify({"word": "sours", "puzzle_word": "", "reported_issue": "missing_from_database"}, dictionary)[0],
            "add_to_database",
        )
        self.assertEqual(
            classify({"word": "rinse", "puzzle_word": "", "reported_issue": "missing_from_puzzle_or_bonus"}, dictionary)[0],
            "needs_puzzle_word",
        )
        self.assertEqual(
            classify({"word": "tinge", "puzzle_word": "resting", "reported_issue": "missing_from_puzzle_or_bonus"}, dictionary)[0],
            "add_to_puzzle_or_bonus",
        )
        self.assertEqual(
            classify({"word": "tinge", "puzzle_word": "soured", "reported_issue": "missing_from_puzzle_or_bonus"}, dictionary)[0],
            "invalid_for_puzzle",
        )

    def test_marks_a_completed_database_addition_resolved(self):
        action, _ = classify(
            {"word": "sours", "puzzle_word": "", "reported_issue": "missing_from_database"},
            {"sours"},
        )
        self.assertEqual(action, "resolved")

    def test_loads_the_word_column_from_a_dictionary_csv(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "words.csv"
            with path.open("w", encoding="utf-8", newline="") as handle:
                writer = csv.DictWriter(handle, fieldnames=["word", "rarity"])
                writer.writeheader()
                writer.writerow({"word": "rinse", "rarity": "3"})
            self.assertEqual(load_dictionary(path), {"rinse"})


if __name__ == "__main__":
    unittest.main()

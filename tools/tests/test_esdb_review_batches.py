import copy
import hashlib
import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from esdb_review_batches import ReviewError, digest, make_batches, validate_catalog, validate_decisions


def sense(identifier="k-1", definition="A device that loads material.", labels=None):
    return {
        "sense_id": identifier,
        "pos": "noun",
        "definition": definition,
        "labels": labels or [],
        "provenance": {"source": "Kaikki English", "record_id": identifier},
    }


def candidate(word, tier="safe", unlocks=None, rank=100, flags=None, senses=None):
    return {
        "word": word,
        "tier": tier,
        "frequency_rank": rank,
        "unlocks": unlocks or [],
        "flags": flags or [],
        "senses": senses if senses is not None else [sense(f"k-{word}")],
    }


def catalog(candidates):
    return {
        "schema_version": 1,
        "source": {
            "name": "Kaikki English", "snapshot": "2026-09-01",
            "sha256": hashlib.sha256(b"fixture").hexdigest(),
            "license": "CC BY-SA 4.0", "url": "https://kaikki.org/",
        },
        "candidates": candidates,
    }


def review(batch, rows):
    return {
        "schema_version": 1,
        "batch_id": batch["batch_id"],
        "batch_sha256": batch["batch_sha256"],
        "reviewer": {"kind": "agent", "id": "editor-agent-1"},
        "reviewed_at": "2026-09-17T18:00:00-06:00",
        "decisions": rows,
    }


class BatchCreationTests(unittest.TestCase):
    def test_orders_by_tier_unlocks_frequency_length_and_word(self):
        values = [
            candidate("manual", "manual", rank=1),
            candidate("simple", rank=5),
            candidate("loader", unlocks=["loaders"], rank=500),
            candidate("ambit", "ambiguous", rank=2),
        ]
        batches, manifest = make_batches(catalog(values), 2)
        self.assertEqual([[x["word"] for x in b["candidates"]] for b in batches],
                         [["loader", "simple"], ["ambit", "manual"]])
        self.assertEqual(manifest["candidate_count"], 4)
        self.assertEqual(manifest["batches"][0]["batch_sha256"], batches[0]["batch_sha256"])
        self.assertEqual(manifest["batch_prefix"], "esdb")

    def test_custom_batch_prefix_produces_unique_hashed_ids(self):
        source = catalog([candidate("loader"), candidate("simple")])
        default_batches, _ = make_batches(source, 1)
        residual_batches, manifest = make_batches(source, 1, "esdb-residual")
        self.assertEqual([batch["batch_id"] for batch in residual_batches],
                         ["esdb-residual-0001", "esdb-residual-0002"])
        self.assertEqual(manifest["batch_prefix"], "esdb-residual")
        self.assertNotEqual(default_batches[0]["batch_sha256"],
                            residual_batches[0]["batch_sha256"])
        # The validator remains schema-compatible because batch_id and its hash
        # were already treated as opaque values.
        rows = [{"word": "loader", "decision": "defer",
                 "reason": "Deferred to exercise a custom residual batch identifier."}]
        result = validate_decisions(residual_batches[0], review(residual_batches[0], rows))
        self.assertEqual(result[0]["word"], "loader")

    def test_rejects_unsafe_batch_prefixes(self):
        source = catalog([candidate("loader")])
        for prefix in ("", "ESDB", "../escape", "esdb_residual", "esdb--residual"):
            with self.subTest(prefix=prefix), self.assertRaisesRegex(ReviewError, "batch_prefix"):
                make_batches(source, 1, prefix)

    def test_rejects_duplicate_words_and_unprovenanced_senses(self):
        with self.assertRaisesRegex(ReviewError, "duplicate candidate word"):
            validate_catalog(catalog([candidate("loader"), candidate("loader")]))
        broken = candidate("loader")
        del broken["senses"][0]["provenance"]
        with self.assertRaisesRegex(ReviewError, "lacks source provenance"):
            validate_catalog(catalog([broken]))

    def test_requires_bad_definitions_to_be_explicitly_flagged(self):
        bad = candidate("loader", senses=[sense(definition="{{rfdef|en}}")])
        with self.assertRaisesRegex(ReviewError, "must be flagged damaged-definition"):
            validate_catalog(catalog([bad]))
        bad["flags"] = ["damaged-definition"]
        self.assertEqual(validate_catalog(catalog([bad]))[0]["word"], "loader")


class DecisionValidationTests(unittest.TestCase):
    def setUp(self):
        self.batch = make_batches(catalog([candidate("loader", unlocks=["loaders"])]), 250)[0][0]

    def test_accepts_exact_sourced_sense_and_preserves_provenance(self):
        rows = [{"word": "loader", "decision": "approve", "sense_id": "k-loader",
                 "rarity": 2, "reason": "Ordinary modern noun with a clear meaning."}]
        result = validate_decisions(self.batch, review(self.batch, rows))
        self.assertEqual(result[0]["sense"]["provenance"]["record_id"], "k-loader")
        self.assertEqual(result[0]["unlocks"], ["loaders"])

    def test_rejects_stale_batch_or_incomplete_review(self):
        stale = review(self.batch, [])
        stale["batch_sha256"] = "0" * 64
        with self.assertRaisesRegex(ReviewError, "stale batch"):
            validate_decisions(self.batch, stale)
        with self.assertRaisesRegex(ReviewError, "incomplete"):
            validate_decisions(self.batch, review(self.batch, []))

    def test_cannot_approve_names_abbreviations_or_bad_senses(self):
        for blocked in ("proper-name", "abbreviation", "derogatory", "unsupported-definition"):
            blocked_batch = make_batches(catalog([candidate("loader", flags=[blocked])]), 10)[0][0]
            rows = [{"word": "loader", "decision": "approve", "sense_id": "k-loader",
                     "rarity": 2, "reason": "The reviewer attempted to waive the policy."}]
            with self.subTest(blocked=blocked), self.assertRaisesRegex(ReviewError, "blocking flags"):
                validate_decisions(blocked_batch, review(blocked_batch, rows))

        labeled = candidate("loader", senses=[sense(labels=["archaic"])])
        labeled_batch = make_batches(catalog([labeled]), 10)[0][0]
        rows = [{"word": "loader", "decision": "approve", "sense_id": "k-1",
                 "rarity": 2, "reason": "This unsafe sense should not be approved."}]
        with self.assertRaisesRegex(ReviewError, "unsafe or damaged"):
            validate_decisions(labeled_batch, review(labeled_batch, rows))

        relation = candidate("loader", senses=[sense(definition="Alternative spelling of loadre.")])
        relation_batch = make_batches(catalog([relation]), 10)[0][0]
        rows = [{"word": "loader", "decision": "approve", "sense_id": "k-1",
                 "rarity": 2, "reason": "A circular relation is not a useful game definition."}]
        with self.assertRaisesRegex(ReviewError, "unsafe or damaged"):
            validate_decisions(relation_batch, review(relation_batch, rows))

    def test_forbids_agent_authored_definition_or_unlisted_sense(self):
        altered = [{"word": "loader", "decision": "approve", "sense_id": "k-loader",
                    "rarity": 2, "definition": "A newly invented meaning.",
                    "reason": "This tries to bypass sourced definition review."}]
        with self.assertRaisesRegex(ReviewError, "unsupported fields"):
            validate_decisions(self.batch, review(self.batch, altered))
        unknown = copy.deepcopy(altered)
        unknown[0].pop("definition")
        unknown[0]["sense_id"] = "made-up"
        with self.assertRaisesRegex(ReviewError, "existing sense_id"):
            validate_decisions(self.batch, review(self.batch, unknown))

    def test_reject_and_defer_require_reason_but_no_lexical_claims(self):
        rows = [{"word": "loader", "decision": "defer",
                 "reason": "Multiple senses require a human lexical review."}]
        self.assertEqual(validate_decisions(self.batch, review(self.batch, rows))[0]["decision"], "defer")
        rows[0]["sense_id"] = "k-loader"
        with self.assertRaisesRegex(ReviewError, "unsupported fields"):
            validate_decisions(self.batch, review(self.batch, rows))

    def test_detects_tampered_batch_contents(self):
        tampered = copy.deepcopy(self.batch)
        tampered["candidates"][0]["word"] = "loaders"
        rows = [{"word": "loaders", "decision": "reject",
                 "reason": "Rejected only to exercise batch integrity validation."}]
        with self.assertRaisesRegex(ReviewError, "do not match"):
            validate_decisions(tampered, review(tampered, rows))


if __name__ == "__main__":
    unittest.main()

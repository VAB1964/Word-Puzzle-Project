#!/usr/bin/env python3
"""Create reproducible ESDB editorial batches and validate review decisions.

The tool deliberately stops short of editing ``esdb_editorial.json``.  It turns a
classified candidate catalog into immutable review packets, then verifies that a
reviewer selected an actual sourced sense and did not approve policy-blocked data.

Catalog and decision files are JSON so nested senses and provenance are retained.
Run ``python tools/esdb_review_batches.py --help`` for the two commands.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from datetime import datetime
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
SCHEMA_VERSION = 1
WORD_RE = re.compile(r"[a-z]{3,7}\Z")
TIERS = ("safe", "ambiguous", "manual")
DECISIONS = ("approve", "reject", "defer")
ALLOWED_POS = {
    "adj", "adv", "conj", "det", "intj", "noun", "num", "particle",
    "prep", "pron", "verb",
}

# A classifier may add more informational flags.  These flags are hard gates: an
# agent cannot waive them by writing a persuasive reason in a decision file.
BLOCKING_FLAGS = {
    "abbreviation", "acronym", "archaic", "bad-sense", "damaged-definition",
    "derogatory", "initialism", "misspelling", "name", "nonstandard", "obsolete", "offensive",
    "proper-name", "proper-noun", "slur", "unsupported-definition", "vulgar",
    "word-fragment", "word-part",
}
BAD_DEFINITION_MARKERS = (
    "{{", "}}", "<ref", "</", "http://", "https://", "please add",
    "definition needed", "unknown meaning", "used as a word",
)
RELATION_ONLY = re.compile(
    r"^(?:plural|singular|comparative|superlative|simple past|past tense|past participle|"
    r"present participle|third-person|alternative (?:form|spelling)|archaic (?:form|spelling)|"
    r"misspelling|inflection)\s+of\b",
    re.I,
)


class ReviewError(ValueError):
    """Raised for malformed or unsafe review artifacts."""


def canonical_bytes(value: Any) -> bytes:
    return (json.dumps(value, ensure_ascii=False, sort_keys=True,
                       separators=(",", ":")) + "\n").encode("utf-8")


def digest(value: Any) -> str:
    return hashlib.sha256(canonical_bytes(value)).hexdigest()


def read_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ReviewError(f"Cannot read JSON {path}: {exc}") from exc


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(json.dumps(value, ensure_ascii=False, indent=2,
                                sort_keys=True).encode("utf-8") + b"\n")


def _strings(value: Any, field: str) -> list[str]:
    if not isinstance(value, list) or not all(isinstance(item, str) for item in value):
        raise ReviewError(f"{field} must be a list of strings")
    return value


def _valid_definition(definition: Any) -> bool:
    if not isinstance(definition, str):
        return False
    text = " ".join(definition.split())
    lowered = text.casefold()
    return (10 <= len(text) <= 500 and text[-1] in ".?!" and
            not any(marker in lowered for marker in BAD_DEFINITION_MARKERS))


def relation_only_definition(definition: Any) -> bool:
    return isinstance(definition, str) and bool(RELATION_ONLY.match(" ".join(definition.split())))


def _blocked(values: list[str]) -> set[str]:
    """Match common tag spellings without trusting classifier punctuation."""
    normalized = {re.sub(r"[\s_]+", "-", value.strip().casefold()) for value in values}
    return normalized & BLOCKING_FLAGS


def validate_catalog(catalog: Any) -> list[dict[str, Any]]:
    """Validate classifier output and return its candidates.

    Provenance is mandatory at both catalog and sense level.  A checksum or
    version-like snapshot identifier makes later reviews reproducible.
    """
    if not isinstance(catalog, dict) or catalog.get("schema_version") != SCHEMA_VERSION:
        raise ReviewError(f"catalog schema_version must be {SCHEMA_VERSION}")
    source = catalog.get("source")
    required_source = ("name", "snapshot", "sha256", "license", "url")
    if not isinstance(source, dict) or any(not source.get(key) for key in required_source):
        raise ReviewError("catalog source must include name, snapshot, sha256, license, and url")
    if not re.fullmatch(r"[0-9a-f]{64}", str(source["sha256"])):
        raise ReviewError("catalog source sha256 must be 64 lowercase hexadecimal characters")
    candidates = catalog.get("candidates")
    if not isinstance(candidates, list):
        raise ReviewError("catalog candidates must be a list")
    seen_words: set[str] = set()
    for candidate in candidates:
        if not isinstance(candidate, dict):
            raise ReviewError("each candidate must be an object")
        word = candidate.get("word")
        if not isinstance(word, str) or not WORD_RE.fullmatch(word):
            raise ReviewError(f"invalid candidate word: {word!r}")
        if word in seen_words:
            raise ReviewError(f"duplicate candidate word: {word}")
        seen_words.add(word)
        if candidate.get("tier") not in TIERS:
            raise ReviewError(f"invalid review tier for {word}")
        flags = _strings(candidate.get("flags", []), f"{word}.flags")
        if len(flags) != len(set(flags)):
            raise ReviewError(f"duplicate candidate flag for {word}")
        unlocks = candidate.get("unlocks", [])
        _strings(unlocks, f"{word}.unlocks")
        if any(not WORD_RE.fullmatch(item) for item in unlocks):
            raise ReviewError(f"invalid unlocked spelling for {word}")
        rank = candidate.get("frequency_rank")
        if rank is not None and (not isinstance(rank, int) or isinstance(rank, bool) or rank < 1):
            raise ReviewError(f"frequency_rank for {word} must be a positive integer or null")
        senses = candidate.get("senses")
        if not isinstance(senses, list):
            raise ReviewError(f"senses for {word} must be a list")
        seen_senses: set[str] = set()
        for sense in senses:
            if not isinstance(sense, dict):
                raise ReviewError(f"each sense for {word} must be an object")
            sense_id = sense.get("sense_id")
            if not isinstance(sense_id, str) or not sense_id or sense_id in seen_senses:
                raise ReviewError(f"missing or duplicate sense_id for {word}")
            seen_senses.add(sense_id)
            if sense.get("pos") not in ALLOWED_POS:
                raise ReviewError(f"unsupported part of speech in {word}/{sense_id}")
            _strings(sense.get("labels", []), f"{word}/{sense_id}.labels")
            provenance = sense.get("provenance")
            if not isinstance(provenance, dict) or any(
                    not provenance.get(key) for key in ("source", "record_id")):
                raise ReviewError(f"sense {word}/{sense_id} lacks source provenance")
            # Bad definitions remain visible for rejection, but must be flagged so
            # that the approval validator has a deterministic hard stop.
            if not _valid_definition(sense.get("definition")) and "damaged-definition" not in flags:
                raise ReviewError(f"invalid definition for {word}/{sense_id} must be flagged damaged-definition")
    return candidates


def priority(candidate: dict[str, Any]) -> tuple[Any, ...]:
    """Stable order: editorial confidence, forms unlocked, frequency, brevity."""
    rank = candidate.get("frequency_rank")
    return (TIERS.index(candidate["tier"]), -len(set(candidate.get("unlocks", []))),
            rank is None, rank or 0, len(candidate["word"]), candidate["word"])


def make_batches(catalog: dict[str, Any], batch_size: int,
                 batch_prefix: str = "esdb") -> tuple[list[dict[str, Any]], dict[str, Any]]:
    candidates = validate_catalog(catalog)
    if batch_size < 1 or batch_size > 500:
        raise ReviewError("batch_size must be between 1 and 500")
    if (not isinstance(batch_prefix, str) or
            not re.fullmatch(r"[a-z][a-z0-9]*(?:-[a-z0-9]+)*", batch_prefix) or
            len(batch_prefix) > 48):
        raise ReviewError("batch_prefix must be a lowercase, hyphen-separated identifier of at most 48 characters")
    catalog_sha = digest(catalog)
    ordered = sorted(candidates, key=priority)
    batches = []
    for offset in range(0, len(ordered), batch_size):
        number = len(batches) + 1
        batch_id = f"{batch_prefix}-{number:04d}"
        batch = {
            "schema_version": SCHEMA_VERSION,
            "batch_id": batch_id,
            "catalog_sha256": catalog_sha,
            "policy": {
                "approval_requires": ["chosen sourced sense", "rarity 1-4", "editorial reason"],
                "blocked_flags": sorted(BLOCKING_FLAGS),
                "definition_edits_allowed": False,
            },
            "candidates": ordered[offset:offset + batch_size],
        }
        batch["batch_sha256"] = digest(batch)
        batches.append(batch)
    manifest = {
        "schema_version": SCHEMA_VERSION,
        "catalog_sha256": catalog_sha,
        "batch_prefix": batch_prefix,
        "batch_size": batch_size,
        "candidate_count": len(ordered),
        "batches": [{"batch_id": item["batch_id"],
                     "batch_sha256": item["batch_sha256"],
                     "candidate_count": len(item["candidates"])} for item in batches],
    }
    return batches, manifest


def _validate_reviewer(reviewer: Any) -> None:
    if not isinstance(reviewer, dict) or reviewer.get("kind") not in {"agent", "human"}:
        raise ReviewError("reviewer.kind must be agent or human")
    if not isinstance(reviewer.get("id"), str) or not reviewer["id"].strip():
        raise ReviewError("reviewer.id is required")


def validate_decisions(batch: dict[str, Any], decisions: Any) -> list[dict[str, Any]]:
    """Return normalized decisions after validating a complete batch review."""
    if not isinstance(batch, dict) or batch.get("schema_version") != SCHEMA_VERSION:
        raise ReviewError("invalid batch schema")
    embedded_hash = batch.get("batch_sha256")
    unsigned = {key: value for key, value in batch.items() if key != "batch_sha256"}
    if embedded_hash != digest(unsigned):
        raise ReviewError("batch contents do not match batch_sha256")
    if not isinstance(decisions, dict) or decisions.get("schema_version") != SCHEMA_VERSION:
        raise ReviewError("invalid decisions schema")
    if decisions.get("batch_id") != batch.get("batch_id") or decisions.get("batch_sha256") != embedded_hash:
        raise ReviewError("decision file targets a different or stale batch")
    _validate_reviewer(decisions.get("reviewer"))
    reviewed_at = decisions.get("reviewed_at")
    try:
        if not isinstance(reviewed_at, str) or datetime.fromisoformat(reviewed_at.replace("Z", "+00:00")).tzinfo is None:
            raise ValueError
    except ValueError as exc:
        raise ReviewError("reviewed_at must be an ISO-8601 timestamp with timezone") from exc
    rows = decisions.get("decisions")
    if not isinstance(rows, list):
        raise ReviewError("decisions must be a list")
    candidates = {item["word"]: item for item in batch.get("candidates", [])}
    by_word: dict[str, dict[str, Any]] = {}
    normalized = []
    for row in rows:
        if not isinstance(row, dict) or row.get("word") not in candidates:
            raise ReviewError(f"decision contains an unknown word: {row!r}")
        word = row["word"]
        if word in by_word:
            raise ReviewError(f"duplicate decision for {word}")
        by_word[word] = row
        decision = row.get("decision")
        reason = row.get("reason")
        if decision not in DECISIONS:
            raise ReviewError(f"invalid decision for {word}")
        if not isinstance(reason, str) or len(reason.strip()) < 10:
            raise ReviewError(f"decision for {word} needs a substantive reason")
        item = {"word": word, "decision": decision, "reason": reason.strip()}
        if decision == "approve":
            candidate = candidates[word]
            blocked = _blocked(candidate.get("flags", []))
            sense_id = row.get("sense_id")
            sense = next((sense for sense in candidate["senses"]
                          if sense["sense_id"] == sense_id), None)
            if blocked:
                raise ReviewError(f"cannot approve {word}; blocking flags: {', '.join(sorted(blocked))}")
            if sense is None:
                raise ReviewError(f"approval for {word} must select an existing sense_id")
            sense_blocked = _blocked(sense.get("labels", []))
            if (sense_blocked or not _valid_definition(sense.get("definition")) or
                    relation_only_definition(sense.get("definition"))):
                raise ReviewError(f"cannot approve unsafe or damaged sense {word}/{sense_id}")
            rarity = row.get("rarity")
            if rarity not in (1, 2, 3, 4):
                raise ReviewError(f"approval for {word} requires integer rarity 1-4")
            unexpected = set(row) - {"word", "decision", "reason", "sense_id", "rarity"}
            if unexpected:
                raise ReviewError(f"approval for {word} has unsupported fields: {sorted(unexpected)}")
            item.update({
                "rarity": rarity,
                "sense": sense,
                "candidate_tier": candidate["tier"],
                "candidate_flags": candidate.get("flags", []),
                "unlocks": candidate.get("unlocks", []),
            })
        else:
            if set(row) - {"word", "decision", "reason"}:
                raise ReviewError(f"{decision} decision for {word} has unsupported fields")
        normalized.append(item)
    missing = sorted(set(candidates) - set(by_word))
    if missing:
        raise ReviewError(f"decision file is incomplete; missing: {', '.join(missing)}")
    return sorted(normalized, key=lambda row: row["word"])


def command_batch(args: argparse.Namespace) -> None:
    catalog = read_json(args.catalog)
    batches, manifest = make_batches(catalog, args.batch_size, args.batch_prefix)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for batch in batches:
        write_json(args.output_dir / f"{batch['batch_id']}.json", batch)
    write_json(args.output_dir / "manifest.json", manifest)


def command_validate(args: argparse.Namespace) -> None:
    batch = read_json(args.batch)
    decisions = read_json(args.decisions)
    normalized = validate_decisions(batch, decisions)
    artifact = {
        "schema_version": SCHEMA_VERSION,
        "batch_id": batch["batch_id"],
        "batch_sha256": batch["batch_sha256"],
        "reviewer": decisions["reviewer"],
        "reviewed_at": decisions["reviewed_at"],
        "decisions": normalized,
    }
    artifact["artifact_sha256"] = digest(artifact)
    output = args.output or ROOT / "tools/esdb_approved_batches" / f"{batch['batch_id']}.json"
    write_json(output, artifact)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    batch_parser = subparsers.add_parser("batch", help="create immutable review packets")
    batch_parser.add_argument("--catalog", type=Path, required=True)
    batch_parser.add_argument("--output-dir", type=Path, required=True)
    batch_parser.add_argument("--batch-size", type=int, default=250)
    batch_parser.add_argument(
        "--batch-prefix", default="esdb",
        help="batch ID prefix (default: esdb; for example esdb-residual)")
    batch_parser.set_defaults(handler=command_batch)
    validate_parser = subparsers.add_parser("validate", help="validate one completed review packet")
    validate_parser.add_argument("--batch", type=Path, required=True)
    validate_parser.add_argument("--decisions", type=Path, required=True)
    validate_parser.add_argument(
        "--output", type=Path,
        help="validated artifact path (default: tools/esdb_approved_batches/<batch-id>.json)")
    validate_parser.set_defaults(handler=command_validate)
    args = parser.parse_args()
    try:
        args.handler(args)
    except ReviewError as exc:
        parser.error(str(exc))


if __name__ == "__main__":
    main()

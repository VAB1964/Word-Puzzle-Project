# Current dictionary cleanup

This document records the cleanup completed before the ESDB migration. For the
current vocabulary policy, totals, and rebuild command, use
[ESDB starting vocabulary](esdb-vocabulary.md). Do not use this historical cleanup
command to rebuild the new ESDB dictionary.

This change cleans the existing vocabulary. It does not switch to SCOWL, add an API,
generate new word forms, or reassign the existing rarity/gem tiers.

## Published result

The starting point is `words_processed.csv` at commit
`f009ae37ef604cf06a38b9216adde12c4a167945`.

| Measure | Before | After |
| --- | ---: | ---: |
| CSV entries | 19,885 | 18,242 |
| Duplicate entries after lowercasing | 491 | 0 |
| Easiest-tier entries (rarity 1) | 955 | 955 |
| Entries with missing definitions | 10 | 0 |
| Entries with a selected usage example | 19,885 (including placeholders) | 6,044 |
| Dictionary file size | 3,680,873 bytes | 2,012,262 bytes |

There are 1,643 fewer rows. Some removed rows were duplicates; others were names,
initialisms, misspellings, non-letter spellings, unsuitable senses, or invalid
references. All retained words keep the rarity of their original lowercase entry
(or their original capitalized entry when a reviewed normalization is needed).
The raw row count is not the count of previously unique playable words.

The exact row exclusions and their reasons are in
[`dictionary-audit/removed-entries.csv`](dictionary-audit/removed-entries.csv).
[`dictionary-audit/summary.json`](dictionary-audit/summary.json) records the build totals.
The duplicate category in that report is smaller than 491 because duplicates in
entirely excluded groups are counted under that group's exclusion reason.

## What changed

- Preserve the lowercase dictionary meaning and rarity when it collides with a
  capitalized brand/name, such as `apple`/`Apple`, `orange`/`Orange`, or `rose`/`Rose`.
- Normalize the spreadsheet-style `TRUE` entry to the ordinary adjective `true`.
- Keep only three-to-seven-letter ASCII spellings usable by the letter wheel.
  Existing plain spellings such as `cafe` and `naive` remain accepted.
- Select an ordinary lexical sense instead of an abbreviation where available.
  Reviewed exceptions preserve familiar shortened words such as `abs`, `app`,
  `bio`, `sub`, `specs`, `teddy`, `tux`, `limo`, and `condo`.
- Repair familiar meanings including `train` (railway), `orange` (fruit), `cat`
  (pet), `par` (golf), `tally` (count), and `golly` (surprise).
- Restore definitions for existing inflections with empty definitions and repair
  damaged text. Explain selected inflections using reviewed or unambiguous roots.
- Remove definition-restatement examples, malformed examples, long/excerpted
  quotations, and examples with obvious archaic spelling. Prefer a short complete
  example from the same sense. Reviewed examples in the override file are original.
- Leave the example empty when none passes those checks. The web and native popup
  omit that row rather than displaying a placeholder or `Sentence: N/A`.
- Stop the legacy example/variant tools from generating new placeholder examples.

Rare words remain part of the existing game. Modern senses take priority over
archaic senses when available, but an archaic label alone is not enough to delete
an entire word. Explicit obsolete-spelling references and misspellings are filtered.
This is a targeted data cleanup, not a claim that every remaining definition has
been manually edited or that every possible English word is included.

## Sources and editorial decisions

`tools/clean_dictionary.py` uses the already checked-in `kaikki_english.csv` for
alternate senses and usage labels. That file contains exactly 200,000 exported
rows and is **a partial source**, not a complete English dictionary. Missing entries
must not be treated as proof that an existing word or inflection is invalid.

`tools/dictionary_overrides.json` stores reviewed meanings, example sentences, and
reasons. It also contains explanatory root meanings for forms whose roots are
absent from this export. These reference meanings do not add playable words.

The script selects one record per existing normalized spelling and retains the
five-column contract: `word,rarity,pos,Definition,Sentence`. Inflection expansion
does not guess a sense for an ambiguous root. Multiword references are not silently
truncated, and cyclic or explicitly excluded roots are rejected.

The current playable file is `words_processed.csv`. The identical
`Standalone/words_processed.csv` is the native distribution copy. Older CSVs and
`words_processed.xlsx` remain historical source material, not regenerated outputs;
do not copy those over the cleaned runtime dictionary.

## Reproduce and validate

Python's standard library is sufficient (Python 3.10+); no dictionary download,
API key, or additional Python package is required.

From the repo root, inspect a candidate without overwriting the live file:

```bash
python tools/clean_dictionary.py --output words_cleaned.csv --report cleanup-summary.json --removed removed-entries.csv
python -m unittest discover -s tools/tests -v
npm test
npm run typecheck
npm run build
npm run worker:dry-run
```

Running the cleanup on its own output must produce byte-identical CSV content.
The committed dictionary was verified this way. Re-running against the already
cleaned file produces a zero-removal report; it does not reproduce the historical
before/after audit. To reproduce that audit, export the starting commit's CSV:

```bash
git show f009ae37ef604cf06a38b9216adde12c4a167945:words_processed.csv > words_before_cleanup.csv
python tools/clean_dictionary.py --input words_before_cleanup.csv --output words_cleaned.csv --report cleanup-summary.json --removed removed-entries.csv
```

After reviewing a future candidate, replace both current CSV copies together and
rerun the tests. The dictionary regression test checks browser/Worker agreement,
definitions, important retained/excluded words, and complete five-puzzle sessions
for both modes at all three difficulties, including crossword intersections.

The existing scoring test fixtures were updated with explicit `gems` arrays
required by the current game schema; scoring code and expected scores are unchanged.
The native C++ popup change needs a Windows/SFML build for native validation; no
replacement executable is included.

## Attribution

Dictionary text derives from [English Wiktionary](https://en.wiktionary.org/),
extracted through [Kaikki](https://kaikki.org/dictionary/English/index.html), and
from the project's existing dictionary edits. This cleanup filters and adapts
that material. Credit the Wiktionary contributors and retain the source links and
license notice when distributing the dictionary.

Wiktionary-derived dictionary text and the new editorial dictionary contributions
are distributed under [Creative Commons Attribution-ShareAlike 4.0](https://creativecommons.org/licenses/by-sa/4.0/).
The entry histories are available through Wiktionary. Source example quotations
may have separate underlying rights; this cleanup does not claim ownership of
them or relicense independent third-party material. This data notice is not a
change to the game's code license.

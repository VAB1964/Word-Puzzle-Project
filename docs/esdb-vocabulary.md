# ESDB starting vocabulary

The game now starts with **American English, ESDB size 60, variant level 1,
three to seven lowercase ASCII letters**. ESDB is the successor to SCOWL.
The maintainer documents size 60 as the default vetted spellchecking set; size
levels are vocabulary inclusion thresholds, **not calibrated game difficulty**.

Source: [English Speller Database](https://github.com/en-wl/wordlist), pinned to
[`1e5b7d3a72f47a71da5d28686c1dd4b397178485`](https://github.com/en-wl/wordlist/tree/1e5b7d3a72f47a71da5d28686c1dd4b397178485).
The exact export arguments and text checksums are in `data/esdb/manifest.json`.

## What is playable

| Measure | Words |
| --- | ---: |
| Previous cleaned dictionary | 18,242 |
| Filtered size-60 starting vocabulary | 28,324 |
| Size-60 words ready for puzzles | 22,005 |
| Individually approved size-70 words | 14 |
| Editorial supplements outside filtered ESDB | 1 |
| Total playable words | 22,020 |
| Retained from the previous dictionary | 13,409 |
| Newly playable | 8,611 |
| Previous words no longer included | 4,833 |
| Size-60 candidates awaiting definition/usage review | 6,319 |

The complete starting vocabulary is checked in, but membership alone is not
enough to enter a puzzle. ESDB supplies spellings, parts of speech, and inflection
relationships; it is not a source of full lexical definitions. The existing
200,000-row Kaikki CSV is only a partial export. Consequently, 6,319 candidates
remain outside the playable file until they have a usable meaning and usage
review. This is a definition-gated adoption, not full playable size-60 coverage.

Every retained entry keeps its prior definition, example, and rarity unchanged,
except when a reviewed supplemental entry explicitly replaces its display definition.
The earlier cleanup is not run again. There are 26 individually reviewed new
headword definitions, including `aisle`, `awesome`, `every`, and `your`.
Another 8,583 new words have grammatical definitions from ESDB's attested
inflection relationships, such as `accepts` and `acted`. These definitions say
which word and grammatical form they represent; they do not claim to provide
a newly researched lexical meaning. Examples remain blank for these forms.

The reviewed supplemental entries `zen`, `zee`, and `zees` cover ordinary lowercase
meanings needed by the game. `zee` and `zees` are individually approved size-70
spellings; lowercase `zen` is an editorial supplement because the filtered source
snapshot contains only the excluded proper-name form. This exception does not admit
other proper names or relax the explicit exclusions in `tools/dictionary_overrides.json`.

No word is created by guessing a suffix. The lemma must already be a defined
size-60 word. Unsupported inflection types, missing roots, and missing meanings
stay in the review queue. A root's lexical definition is not copied into an
inflection: a spelling such as `lie` has multiple meanings, and copying its
selected definition could give `lied` the wrong explanation.

## Vocabulary and difficulty policy

- Use the upstream American `A` spelling selection and maximum variant level 1.
- Exclude abbreviation POS, name/brand classes, word parts, nonwords, special
  categories, and the listed offensive/vulgar/nonstandard usage notes before
  exporting. These source labels are incomplete, so export membership is not
  blanket editorial approval for new headwords.
- Deaccent using the upstream exporter, then require `[a-z]{3,7}`. Do not lowercase
  names. This keeps ordinary `apple` while excluding capitalized-only `Apple`.
- Preserve explicit exclusions from `tools/dictionary_overrides.json`.
- Permit only explicitly reviewed lowercase supplements recorded with definitions,
  ratings, and reasons in `tools/esdb_editorial.json`.
- Preserve all retained rarity values. An attested new inflection inherits its
  lemma's current rating. This is a provisional familiarity heuristic, not a
  frequency measurement. The 26 new lexical entries have explicit provisional
  editorial ratings in `tools/esdb_editorial.json`.
- Never convert size 60/70/80 directly into rarity 2/3/4.

The American spelling policy removes `colour`, `centre`, `honour`, and `theatre`;
their American counterparts remain. The six removed prior rarity-1 entries are
`bam`, `english`, `french`, `india`, `june`, and `york`. They do not pass the chosen
filtered size-60 policy; removal is not a claim that each is an invalid English
word. Unlike the previous cleanup, this migration deliberately changes the
vocabulary boundary. Scoring code is unchanged, although new inherited ratings
change the available puzzle pool and should be playtested.

## Larger-level review

The snapshot includes 11,559 extra spellings at level 70 and another 11,135 at
level 80. They are review candidates, not automatic additions.

Fourteen words were individually approved from size 70. Twelve existing rarity-4
words retain their advanced rating:
`abiotic`, `albedo`, `anomie`, `aril`, `bireme`, `gnosis`, `tmesis`, `xeric`,
`zeugma`, `xebec`, `agaric`, and `allium`. Each has an existing explanatory
definition and a recorded reason in `tools/esdb_editorial.json`. Their rarity
remains 4, which keeps them out of Easy and Medium puzzle answers.
The familiar American letter name `zee` retains tier 3, and its plural `zees`
uses the same provisional tier.
No size-80 additions were approved. All other larger-level candidates are
explicitly deferred; they have not all received individual lexical review.

## Build and audit

Ordinary builds require only Python 3.10+ and the Git history containing the
previous cleanup commit. No network, paid API, Python package installation,
or ESDB database is required:

```powershell
python tools/build_esdb_dictionary.py --output-dir esdb-candidate
python -m unittest discover -s tools/tests -v
npm test
npm run typecheck
npm run build
npm run worker:dry-run
```

The builder reads `words_processed.csv` from pinned pre-migration commit
`88c958927f986cff82f19222d8d7482567b908d3`, so repeated runs do not mistake generated
forms for original lexical definitions or gradually change the audit.
For a shallow checkout, fetch the repository history or supply
`--baseline path/to/the-pre-esdb-words_processed.csv` explicitly.
The builder always writes a candidate directory; it never silently overwrites
either runtime dictionary. After reviewing a future candidate, copy its CSV to
both `words_processed.csv` and `Standalone/words_processed.csv`, and its five
audit files to `docs/esdb-audit/`.

Committed audit files:

- `summary.json`: exact before/after totals.
- `word-provenance.csv`: source and rarity basis for every playable spelling.
- `removed-words.csv`: each removed previous entry and the policy reason.
- `pending-definitions.csv`: every size-60 candidate not yet playable.
- `larger-level-review.csv`: level, existing meaning/rating if available, and
  approval/defer decision for every larger-level candidate.

To regenerate the source snapshot, clone the upstream repository, check out the
pinned commit above, then run:

```powershell
python tools/export_esdb.py path/to/wordlist
```

This command verifies the checkout, rebuilds the upstream database from source,
and regenerates the word lists, inflection relationships, and checksums. It uses
upstream's own export and query logic. Updating to a different upstream revision
requires intentionally changing the pin and reviewing the new audit.

## Local playtest

From the project root:

```powershell
npm --prefix web install
npm --prefix web run dev
```

Open **http://localhost:5173/wordpuzzle/** and choose Single Player. For multiplayer,
also start `npm run worker:dev` in a second terminal. Try Casual and Crossword at
each difficulty, particularly new inflections and their definition popups.

The submitted version passes 19 Python tests and 28 JavaScript/TypeScript tests,
including 30 generated puzzles covering both modes and all three difficulties.
Typecheck, the web build, and Worker dry-run packaging pass. The published CSV
matches the reproducible build, and both runtime dictionary copies are identical.
The native executable was not rebuilt; this change updates its bundled CSV and
license notices without changing native source code.

## Attribution

The ESDB notices are preserved in `data/esdb/Copyright`, including the additional
notices relevant to database-derived metadata. The generated word lists and
inflection relationships come from ESDB. Lexical definitions retain the existing
Wiktionary/Kaikki attribution and CC BY-SA 4.0 terms described in
[the cleanup documentation](dictionary-cleanup.md#attribution). The new editorial
definitions and examples are original contributions under those same terms.

Combined notices ship in `Standalone/Dictionary-Licenses.txt` and
`web/public/wordpuzzle/dictionary-licenses.txt` (served at
`/wordpuzzle/dictionary-licenses.txt`). These data notices do not change the
game's code license.

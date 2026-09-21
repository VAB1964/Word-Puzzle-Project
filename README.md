# Word Puzzle

## Checking reported word issues

Run the checker from the project root whenever new multiplayer word reports have
been added to `data/word_issues.csv`:

```powershell
npm run word-issues
```

If PowerShell prevents `npm.ps1` from running because of its execution policy,
use the Windows command wrapper instead:

```powershell
npm.cmd run word-issues
```

You can also run the Python script directly:

```powershell
python tools/check_word_issues.py
```

The input CSV has these columns:

- `word`: the reported word.
- `puzzle_word`: the original base word, when the reporter supplied it. Leave it
  blank when it is unknown.
- `reported_issue`: either `missing_from_database` or
  `missing_from_puzzle_or_bonus`.
- `notes`: optional context about the report.

The checker compares every report with `words_processed.csv` and prints the
recommended action. Possible results include:

- `add_to_database`: the word is missing from the playable dictionary.
- `add_to_puzzle_or_bonus`: the word is in the dictionary, can be formed from
  the supplied puzzle word, and was reported missing from both puzzle lists.
- `needs_puzzle_word`: the report cannot be fully checked because its puzzle
  word is unknown.
- `resolved`: a word reported missing from the database is now present.
- `invalid_for_puzzle` or `invalid_report`: the supplied data needs correction.

To save the classified results to another CSV:

```powershell
python tools/check_word_issues.py --output data/word_issue_results.csv
```

Custom input and dictionary files can also be supplied:

```powershell
python tools/check_word_issues.py `
  --issues data/word_issues.csv `
  --dictionary words_processed.csv `
  --output data/word_issue_results.csv
```

The checker only reports the needed action. It does not modify the dictionary
or issue file automatically.

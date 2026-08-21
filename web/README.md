# Word Puzzle (Web)

## Prereqs
- Node.js 18+ and npm on PATH.

## Install
```
npm install
```

## Run (dev)
```
npm run dev
```

## Build
```
npm run build
npm run preview
```

## Prerecorded Cribbage Table Talk

Cribbage uses scripted dialogue and prerecorded WebM clips from
`cribbage-transfer/cribbage/public/table-talk-voice/`. Runtime text and speech
generation are not used. The manifest maps each exact character and line pair
to its clip.

To generate or refresh the pack, set `GEMINI_API_KEY` in the environment or in
`web/.dev.vars`, then run:

```
npm run generate:cribbage-voices:gemini
```

The optional `TABLE_TALK_TTS_MODEL` variable selects the Gemini TTS model used
by that batch script. This key and model are only needed while generating the
static pack; they are not shipped to or used by the game at runtime.

## Assets + data
This web port loads assets and `words_processed.csv` from the repo root via Vite's file access:
- `assets/`, `fonts/`, `words_processed.csv`

If you want a fully self-contained web folder, copy those files into `web/public/` and update `web/src/assets.ts` accordingly.

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

## Assets + data
This web port loads assets and `words_processed.csv` from the repo root via Vite's file access:
- `assets/`, `fonts/`, `words_processed.csv`

If you want a fully self-contained web folder, copy those files into `web/public/` and update `web/src/assets.ts` accordingly.

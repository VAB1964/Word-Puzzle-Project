# Word Puzzle (Web)

## Prereqs
- Node.js 20.19+ and npm on PATH.

## Install
```
npm install
npm --prefix web install
```

## Run (dev)
From the repository root, start both processes:
```
npm run worker:dev
npm --prefix web run dev
```

The Vite development server proxies `/api/wordpuzzle` WebSockets and HTTP requests
to the local Worker on port `8790`.

## Build
```
npm run build
npm --prefix web run preview
```

The production build is published at `https://vabgames.com/wordpuzzle/`.
The VABGames landing page and Cribbage are maintained in their own repositories.

## Assets + data
This web port loads assets and `words_processed.csv` from the repo root via Vite's file access:
- `assets/`, `fonts/`, `words_processed.csv`

If you want a fully self-contained web folder, copy those files into `web/public/` and update `web/src/assets.ts` accordingly.

## Multiplayer

Multiplayer uses an authoritative Cloudflare Worker and one Durable Object per
room. Shared rules live in `shared/multiplayer`, Worker code in `worker/src`, and
the Canvas-independent entry/lobby/play interface in `web/src/multiplayer`.

Useful root commands:

```
npm test
npm run typecheck
npm run worker:types
npm run worker:dry-run
npx wrangler deploy --env staging
npx wrangler deploy --env production
```

Set `ALLOWED_ORIGIN` for staging and production in `wrangler.jsonc` before
deploying from a different web origin. Same-origin requests are always accepted.

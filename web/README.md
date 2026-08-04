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

## Cloud Table Talk voice (best-practice mode)

Cribbage supports two voice engines:
- `browser` (Web Speech API, local voices)
- `cloud` (secure server proxy, no API key in browser)

Cloud mode uses Cloudflare Pages Functions endpoints:
- `POST /api/table-talk-session` (issues short-lived signed session token)
- `POST /api/table-talk-generate` (generates notable in-character reactions from public game state)
- `POST /api/table-talk-tts` (server-side OpenAI TTS call)

Set these Pages environment variables:
- `TABLE_TALK_SESSION_SECRET` (required, long random string)
- `TABLE_TALK_TTS_PROVIDER` (`openai` or `gemini`; default: `openai`)
- `OPENAI_API_KEY` (required when provider is `openai`)
- `GEMINI_API_KEY` (required when provider is `gemini`)
- `TABLE_TALK_DIALOGUE_MODEL` (optional Gemini text model; defaults to `gemini-2.5-flash`)
- `TABLE_TALK_TTS_MODEL` (optional; defaults to `gpt-4o-mini-tts` for OpenAI or `gemini-3.1-flash-tts-preview` for Gemini)
- `TABLE_TALK_TTS_USD_PER_1K_CHARS` (optional, default: `0.015`, used for menu cost estimate)

Security notes:
- Browser never stores provider key.
- Session token is short-lived and generated server-side.
- Disable cloud voice by switching voice engine back to `browser` in-game.

## Assets + data
This web port loads assets and `words_processed.csv` from the repo root via Vite's file access:
- `assets/`, `fonts/`, `words_processed.csv`

If you want a fully self-contained web folder, copy those files into `web/public/` and update `web/src/assets.ts` accordingly.

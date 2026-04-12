/**
 * One-time script to generate pre-baked voice clips via OpenAI TTS API.
 * Run: node web/scripts/generateVoiceClips.mjs
 * Requires OPENAI_API_KEY env var or .env file in project root.
 *
 * Handles rate limits (3 RPM on free tier) with automatic retry + backoff.
 * Skips files that already exist on disk.
 */

import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const OUTPUT_DIR = path.resolve(__dirname, "../../assets/voice");

const API_KEY =
  process.env.OPENAI_API_KEY ||
  process.env.VITE_OPENAI_API_KEY ||
  loadDotEnv();

function loadDotEnv() {
  try {
    const envPath = path.resolve(__dirname, "../../.env");
    const content = fs.readFileSync(envPath, "utf-8");
    for (const line of content.split("\n")) {
      const match = line.match(/^VITE_OPENAI_API_KEY=(.+)$/);
      if (match) return match[1].trim();
    }
  } catch { /* no .env */ }
  return null;
}

if (!API_KEY) {
  console.error("No API key found. Set OPENAI_API_KEY or add it to .env");
  process.exit(1);
}

const VOICE = "nova";
const MODEL = "tts-1";
const SPEED = 1.05;
const MAX_RETRIES = 5;
const BASE_DELAY_MS = 22_000; // ~22s between requests to stay within 3 RPM

const CLIPS = {
  word_found: [
    "Nice one!",
    "Great find!",
    "Impressive!",
    "Well spotted!",
    "Great catch!",
    "That's a good one!",
    "Look at that!",
    "Nicely done!",
    "Bravo!",
    "You're on fire!",
    "Excellent!",
    "Keep it up!",
    "What a word!",
    "Amazing find!",
    "Wonderful!",
  ],
  rare_word: [
    "Wow, that's a rare one!",
    "Impressive, that's uncommon!",
    "What a find! Very rare!",
    "Not many would get that!",
    "Outstanding! That's a tough one!",
    "You really know your words!",
    "Incredible! That's a gem!",
    "Brilliant! A rare discovery!",
  ],
  puzzle_solved: [
    "Puzzle complete! You nailed it!",
    "All words found, well done!",
    "Puzzle solved! Fantastic work!",
    "You cleared the whole puzzle!",
    "Every word found, amazing!",
    "Puzzle finished! Brilliant!",
    "That's a wrap! Great job!",
    "All done! You're a natural!",
    "Perfect puzzle clearance!",
    "Solved it! Nothing gets past you!",
  ],
  session_complete: [
    "Session finished! You're a word master!",
    "What a performance, every puzzle solved!",
    "Session complete! Absolutely phenomenal!",
    "You conquered every puzzle! Incredible!",
    "All puzzles done! You're unstoppable!",
    "Session cleared! Take a bow!",
    "That was brilliant, session complete!",
    "Every puzzle beaten! What a champion!",
    "Full session victory! Outstanding!",
    "You did it! All puzzles mastered!",
  ],
};

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

async function generateClip(text, outputPath, attempt = 1) {
  const res = await fetch("https://api.openai.com/v1/audio/speech", {
    method: "POST",
    headers: {
      Authorization: `Bearer ${API_KEY}`,
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      model: MODEL,
      input: text,
      voice: VOICE,
      response_format: "wav",
      speed: SPEED,
    }),
  });

  if (res.status === 429 && attempt <= MAX_RETRIES) {
    const retryAfter = parseInt(res.headers.get("retry-after") || "25", 10);
    const waitMs = retryAfter * 1000 + 2000;
    process.stdout.write(`RATE LIMITED, waiting ${(waitMs / 1000).toFixed(0)}s (attempt ${attempt}/${MAX_RETRIES})... `);
    await sleep(waitMs);
    return generateClip(text, outputPath, attempt + 1);
  }

  if (!res.ok) {
    const err = await res.text();
    throw new Error(`API error ${res.status}: ${err.slice(0, 120)}`);
  }

  const buffer = Buffer.from(await res.arrayBuffer());
  fs.writeFileSync(outputPath, buffer);
  return buffer.length;
}

async function main() {
  fs.mkdirSync(OUTPUT_DIR, { recursive: true });

  const allClips = [];
  for (const [category, lines] of Object.entries(CLIPS)) {
    for (let i = 0; i < lines.length; i++) {
      const filename = `${category}_${String(i + 1).padStart(2, "0")}.wav`;
      allClips.push({ filename, text: lines[i], category });
    }
  }

  const toGenerate = allClips.filter(
    (c) => !fs.existsSync(path.join(OUTPUT_DIR, c.filename))
  );

  console.log(`Total clips: ${allClips.length}, already exist: ${allClips.length - toGenerate.length}, to generate: ${toGenerate.length}`);
  if (toGenerate.length === 0) {
    console.log("All clips already exist. Nothing to do.");
    return;
  }

  console.log(`Generating ${toGenerate.length} clips (rate limit: ~3/min, expect ~${Math.ceil(toGenerate.length / 3)} min)...\n`);
  let done = 0;

  for (const clip of toGenerate) {
    const outputPath = path.join(OUTPUT_DIR, clip.filename);
    process.stdout.write(`[${++done}/${toGenerate.length}] ${clip.filename} — "${clip.text}" ... `);
    try {
      const bytes = await generateClip(clip.text, outputPath);
      console.log(`OK (${(bytes / 1024).toFixed(1)} KB)`);
    } catch (err) {
      console.log(`FAILED: ${err.message}`);
    }

    if (done < toGenerate.length) await sleep(BASE_DELAY_MS);
  }

  const existing = fs.readdirSync(OUTPUT_DIR).filter((f) => f.endsWith(".wav")).length;
  console.log(`\nDone! ${existing}/${allClips.length} clips now on disk.`);
}

main().catch((err) => {
  console.error("Fatal error:", err);
  process.exit(1);
});

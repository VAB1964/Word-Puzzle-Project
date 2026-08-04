/**
 * Generates a small, versioned Cribbage voice pack for zero-wait common lines.
 * Review cribbageVoicePack.json before running.
 *
 * Run from /web: npm run generate:cribbage-voices
 * Uses OPENAI_API_KEY from the environment or web/.dev.vars.
 * Existing clips are skipped, so reruns only generate new phrases.
 */
import { createHash } from "node:crypto";
import fs from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const scriptsDir = path.dirname(fileURLToPath(import.meta.url));
const webRoot = path.resolve(scriptsDir, "..");
const outputDir = path.resolve(webRoot, "../cribbage-transfer/cribbage/public/table-talk-voice");
const phrases = JSON.parse(await fs.readFile(path.join(scriptsDir, "cribbageVoicePack.json"), "utf8"));
const voices = { mabel: "nova", arthur: "onyx", clara: "shimmer" };
const instructions = {
  mabel: "Warm and lightly mischievous. Speak briskly and fluidly with minimal pauses.",
  arthur: "Dry, friendly, and understated. Speak briskly and fluidly with minimal pauses.",
  clara: "Cheerful, relaxed, and grounded. Speak briskly and fluidly with minimal pauses.",
};

async function loadDevVars() {
  try {
    const text = await fs.readFile(path.join(webRoot, ".dev.vars"), "utf8");
    return Object.fromEntries(text.split(/\r?\n/).map(line => line.match(/^([^#=\s]+)=(.*)$/)).filter(Boolean).map(match => [match[1], match[2].replace(/^["']|["']$/g, "")]));
  } catch {
    return {};
  }
}

const devVars = await loadDevVars();
const apiKey = process.env.OPENAI_API_KEY || devVars.OPENAI_API_KEY;
if (!apiKey) throw new Error("OPENAI_API_KEY is required in the environment or web/.dev.vars.");

function clipId(characterId, text) {
  return createHash("sha256").update(`${characterId}|${text}`).digest("hex").slice(0, 20);
}

async function generate(characterId, text) {
  const response = await fetch("https://api.openai.com/v1/audio/speech", {
    method: "POST",
    headers: { Authorization: `Bearer ${apiKey}`, "Content-Type": "application/json" },
    body: JSON.stringify({
      model: process.env.TABLE_TALK_TTS_MODEL || devVars.TABLE_TALK_TTS_MODEL || "gpt-4o-mini-tts",
      voice: voices[characterId],
      input: text,
      format: "mp3",
      instructions: instructions[characterId],
    }),
  });
  if (!response.ok) throw new Error(`TTS failed (${response.status}): ${(await response.text()).slice(0, 160)}`);
  return Buffer.from(await response.arrayBuffer());
}

await fs.mkdir(outputDir, { recursive: true });
const manifest = {};
let generated = 0;
let reused = 0;

for (const [characterId, lines] of Object.entries(phrases)) {
  for (const text of lines) {
    const filename = `${characterId}-${clipId(characterId, text)}.mp3`;
    const outputPath = path.join(outputDir, filename);
    try {
      await fs.access(outputPath);
      reused++;
    } catch {
      const audio = await generate(characterId, text);
      await fs.writeFile(outputPath, audio);
      generated++;
      console.log(`Generated ${characterId}: "${text}"`);
    }
    manifest[`${characterId}|${text}`] = `table-talk-voice/${filename}`;
  }
}

await fs.writeFile(path.join(outputDir, "manifest.json"), `${JSON.stringify(manifest, null, 2)}\n`);
console.log(`Voice pack ready: ${generated} generated, ${reused} reused, ${Object.keys(manifest).length} total.`);

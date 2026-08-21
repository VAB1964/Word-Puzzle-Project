/**
 * Generates the complete Cribbage table-talk voice pack with Gemini TTS.
 *
 * Run from /web:
 *   npm run generate:cribbage-voices:gemini
 *   npm run generate:cribbage-voices:gemini -- --dry-run --list
 *
 * GEMINI_API_KEY is read from the environment or web/.dev.vars. Existing
 * valid clips are reused, so an interrupted run can be resumed safely.
 */
import { createHash } from "node:crypto";
import fs from "node:fs/promises";
import path from "node:path";
import { spawn } from "node:child_process";
import { fileURLToPath } from "node:url";
import ffmpegPath from "ffmpeg-static";
import ts from "typescript";

const scriptsDir = path.dirname(fileURLToPath(import.meta.url));
const webRoot = path.resolve(scriptsDir, "..");
const projectRoot = path.resolve(webRoot, "..");
const dialoguePath = path.join(
  projectRoot,
  "cribbage-transfer/cribbage/src/tableTalk/dialogueLibrary.ts",
);
const outputDir = path.join(
  projectRoot,
  "cribbage-transfer/cribbage/public/table-talk-voice",
);

const args = new Set(process.argv.slice(2));
const dryRun = args.has("--dry-run");
const listLines = args.has("--list");
const showHelp = args.has("--help") || args.has("-h");
const unknownArgs = [...args].filter(
  arg => !["--dry-run", "--list", "--help", "-h"].includes(arg),
);

if (showHelp) {
  console.log(`Generate the complete Gemini Cribbage voice pack.

Usage:
  node ./scripts/generateCribbageGeminiVoicePack.mjs [options]

Options:
  --dry-run  Extract and validate lines without calling Gemini or writing files
  --list     Print every extracted character, dialogue key, emotion, and line
  --help     Show this help

Environment:
  GEMINI_API_KEY       Required except in dry-run mode
  TABLE_TALK_TTS_MODEL Optional (default: gemini-3.1-flash-tts-preview)`);
  process.exit(0);
}
if (unknownArgs.length > 0) {
  throw new Error(`Unknown option(s): ${unknownArgs.join(", ")}. Use --help for usage.`);
}

const VOICE_BY_CHARACTER = {
  mabel: "Sulafat",
  arthur: "Achird",
  clara: "Laomedeia",
};

const INSTRUCTIONS_BY_CHARACTER = {
  mabel: "Warm, relaxed, and lightly mischievous, like a friend enjoying a casual card game.",
  arthur: "Dry and mildly competitive, with an easygoing, friendly undertone and a natural conversational pace.",
  clara: "Cheerful and optimistic, but relaxed and grounded, like she is quietly enjoying the game.",
};

const INSTRUCTIONS_BY_EMOTION = {
  supportive: "Sound gently encouraging and warm.",
  playful: "Sound lightly amused, never theatrical or mean.",
  dry: "Sound understated and wry, with subtle sarcasm.",
  optimistic: "Sound quietly upbeat and confident.",
  competitive: "Sound engaged and mildly competitive, without intensity.",
  concerned: "Sound thoughtful and slightly cautious, not anxious.",
  self_deprecating: "Use understated humor at your own expense.",
};

function eventTypeForDialogueKey(key) {
  if (key === "self_large_hand" || key === "opp_large_hand") return "large_hand_scored";
  if (key === "self_large_crib" || key === "opp_large_crib") return "large_crib_scored";
  if (
    key === "self_fifteen"
    || key === "opp_fifteen"
    || key === "self_thirty_one"
    || key === "opp_thirty_one"
    || key === "self_pair"
    || key === "opp_pair"
    || key === "self_pair_royal"
    || key === "opp_pair_royal"
    || key === "self_double_pair_royal"
    || key === "opp_double_pair_royal"
    || key === "self_pegging_run"
    || key === "opp_pegging_run"
    || key === "self_last_card"
    || key === "opp_last_card"
  ) return "pegging_scored";
  return key;
}

function eventInstruction(eventType) {
  if (eventType === "game_won") return "Give a pleased but restrained, friendly reaction.";
  if (eventType === "game_lost") return "Deliver graciously and respectfully.";
  if (eventType === "large_hand_scored" || eventType === "large_crib_scored") {
    return "Sound pleasantly surprised without making a big performance of it.";
  }
  if (eventType === "pegging_scored") {
    return "Keep it brief and natural, like an immediate table reaction.";
  }
  if (eventType === "go_declared") return "Say exactly the single word 'Go.' with no extra words.";
  return "Keep delivery conversational and table-friendly.";
}

function ttsInstructions(line) {
  return `${INSTRUCTIONS_BY_CHARACTER[line.characterId]} ${INSTRUCTIONS_BY_EMOTION[line.emotion]} ${eventInstruction(eventTypeForDialogueKey(line.dialogueKey))} Speak at a brisk, fluid conversational pace. Minimize pauses within and between sentences; do not add dramatic timing, hesitation, or reflective breaks. Keep the delivery concise and natural, as if play is moving quickly around a card table. Have fun without sounding overexcited, theatrical, performative, or exaggerated.`;
}

function propertyName(node) {
  if (ts.isIdentifier(node) || ts.isStringLiteralLike(node)) return node.text;
  throw new Error(`Unsupported property name at ${dialoguePath}:${node.getStart()}.`);
}

function objectProperty(object, name) {
  const property = object.properties.find(
    candidate => ts.isPropertyAssignment(candidate) && propertyName(candidate.name) === name,
  );
  if (!property || !ts.isPropertyAssignment(property)) {
    throw new Error(`Missing "${name}" property in dialogue library.`);
  }
  return property.initializer;
}

function expectObject(node, context) {
  if (!ts.isObjectLiteralExpression(node)) throw new Error(`${context} must be an object literal.`);
  return node;
}

function expectString(node, context) {
  if (!ts.isStringLiteralLike(node)) throw new Error(`${context} must be a string literal.`);
  return node.text;
}

async function extractDialogueLines() {
  const sourceText = await fs.readFile(dialoguePath, "utf8");
  const source = ts.createSourceFile(
    dialoguePath,
    sourceText,
    ts.ScriptTarget.Latest,
    true,
    ts.ScriptKind.TS,
  );
  let library;
  for (const statement of source.statements) {
    if (!ts.isVariableStatement(statement)) continue;
    const declaration = statement.declarationList.declarations.find(
      item => ts.isIdentifier(item.name) && item.name.text === "DIALOGUE_LIBRARY",
    );
    if (declaration?.initializer) library = declaration.initializer;
  }
  if (!library) throw new Error(`Could not find DIALOGUE_LIBRARY in ${dialoguePath}.`);

  const root = expectObject(library, "DIALOGUE_LIBRARY");
  const extracted = [];
  for (const characterProperty of root.properties) {
    if (!ts.isPropertyAssignment(characterProperty)) continue;
    const characterId = propertyName(characterProperty.name);
    if (!(characterId in VOICE_BY_CHARACTER)) {
      throw new Error(`No Gemini voice configured for character "${characterId}".`);
    }
    const pools = expectObject(characterProperty.initializer, characterId);
    for (const poolProperty of pools.properties) {
      if (!ts.isPropertyAssignment(poolProperty)) continue;
      const dialogueKey = propertyName(poolProperty.name);
      const pool = expectObject(poolProperty.initializer, `${characterId}.${dialogueKey}`);
      const emotion = expectString(
        objectProperty(pool, "emotion"),
        `${characterId}.${dialogueKey}.emotion`,
      );
      if (!(emotion in INSTRUCTIONS_BY_EMOTION)) {
        throw new Error(`No TTS instruction configured for emotion "${emotion}".`);
      }
      const lineNodes = objectProperty(pool, "lines");
      if (!ts.isArrayLiteralExpression(lineNodes)) {
        throw new Error(`${characterId}.${dialogueKey}.lines must be an array literal.`);
      }
      for (const node of lineNodes.elements) {
        extracted.push({
          characterId,
          dialogueKey,
          emotion,
          text: expectString(node, `${characterId}.${dialogueKey} line`),
        });
      }
    }
  }
  return extracted;
}

function deduplicateLines(lines) {
  const unique = new Map();
  for (const line of lines) {
    const key = `${line.characterId}|${line.text}`;
    const existing = unique.get(key);
    if (
      existing
      && (existing.emotion !== line.emotion || existing.dialogueKey !== line.dialogueKey)
    ) {
      console.warn(
        `Duplicate exact line uses first delivery metadata: ${key} `
        + `(${existing.dialogueKey}/${existing.emotion}, also ${line.dialogueKey}/${line.emotion})`,
      );
    }
    if (!existing) unique.set(key, line);
  }
  return [...unique.values()];
}

async function loadDevVars() {
  try {
    const text = await fs.readFile(path.join(webRoot, ".dev.vars"), "utf8");
    return Object.fromEntries(
      text
        .split(/\r?\n/)
        .map(line => line.match(/^([^#=\s]+)=(.*)$/))
        .filter(Boolean)
        .map(match => [match[1], match[2].trim().replace(/^["']|["']$/g, "")]),
    );
  } catch (error) {
    if (error?.code === "ENOENT") return {};
    throw error;
  }
}

function clipId(characterId, text) {
  return createHash("sha256")
    .update(`${characterId}|${text}`)
    .digest("hex")
    .slice(0, 20);
}

function filenameFor(line) {
  return `${line.characterId}-${clipId(line.characterId, line.text)}.webm`;
}

function sleep(milliseconds) {
  return new Promise(resolve => setTimeout(resolve, milliseconds));
}

async function fetchGeminiAudio(line, apiKey, model) {
  const voiceName = VOICE_BY_CHARACTER[line.characterId];
  const response = await fetch(
    `https://generativelanguage.googleapis.com/v1beta/models/${encodeURIComponent(model)}:generateContent`,
    {
      method: "POST",
      headers: {
        "x-goog-api-key": apiKey,
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        contents: [{
          parts: [{
            text: `${ttsInstructions(line)}\n\nSay exactly: ${line.text}`,
          }],
        }],
        generationConfig: {
          responseModalities: ["AUDIO"],
          speechConfig: {
            voiceConfig: {
              prebuiltVoiceConfig: { voiceName },
            },
          },
        },
      }),
    },
  );

  if (!response.ok) {
    const detail = (await response.text()).slice(0, 500);
    const error = new Error(`Gemini TTS failed (${response.status}): ${detail}`);
    error.status = response.status;
    throw error;
  }
  const payload = await response.json();
  const parts = payload?.candidates?.[0]?.content?.parts ?? [];
  const inlineData = parts.find(part => part?.inlineData?.data)?.inlineData;
  if (!inlineData?.data) throw new Error("Gemini TTS returned no audio.");
  const rateMatch = (inlineData.mimeType ?? "").match(/rate=(\d+)/i);
  return {
    pcm: Buffer.from(inlineData.data, "base64"),
    sampleRate: rateMatch ? Number(rateMatch[1]) : 24000,
  };
}

async function fetchGeminiAudioWithRetry(line, apiKey, model, maxAttempts = 5) {
  for (let attempt = 1; attempt <= maxAttempts; attempt++) {
    try {
      return await fetchGeminiAudio(line, apiKey, model);
    } catch (error) {
      const transient = error?.status === 429 || (error?.status >= 500 && error?.status <= 599);
      if (!transient || attempt === maxAttempts) throw error;
      const delay = Math.min(30_000, 1_000 * (2 ** (attempt - 1)));
      console.warn(`  Retry ${attempt}/${maxAttempts - 1} in ${delay / 1000}s: ${error.message}`);
      await sleep(delay);
    }
  }
  throw new Error("Retry loop ended unexpectedly.");
}

async function encodeWebmOpus(pcm, sampleRate, outputPath) {
  if (!ffmpegPath) throw new Error("The project-local ffmpeg-static encoder is unavailable.");
  await new Promise((resolve, reject) => {
    const encoder = spawn(
      ffmpegPath,
      [
        "-hide_banner",
        "-loglevel", "error",
        "-f", "s16le",
        "-ar", String(sampleRate),
        "-ac", "1",
        "-i", "pipe:0",
        "-c:a", "libopus",
        "-b:a", "24k",
        "-vbr", "on",
        "-compression_level", "10",
        "-application", "voip",
        "-f", "webm",
        "-y",
        outputPath,
      ],
      { stdio: ["pipe", "ignore", "pipe"] },
    );
    let stderr = "";
    encoder.stderr.on("data", chunk => {
      stderr += chunk.toString();
    });
    encoder.on("error", reject);
    encoder.on("close", code => {
      if (code === 0) resolve();
      else reject(new Error(`Opus encoder failed (${code}): ${stderr.slice(-500)}`));
    });
    encoder.stdin.on("error", error => {
      if (error.code !== "EPIPE") reject(error);
    });
    encoder.stdin.end(pcm);
  });
}

async function isValidWebm(filePath) {
  try {
    const handle = await fs.open(filePath, "r");
    try {
      const stats = await handle.stat();
      if (stats.size < 128) return false;
      const signature = Buffer.alloc(4);
      await handle.read(signature, 0, 4, 0);
      return signature.equals(Buffer.from([0x1a, 0x45, 0xdf, 0xa3]));
    } finally {
      await handle.close();
    }
  } catch (error) {
    if (error?.code === "ENOENT") return false;
    throw error;
  }
}

async function writeJsonAtomically(filePath, value) {
  const temporaryPath = `${filePath}.tmp-${process.pid}`;
  await fs.writeFile(temporaryPath, `${JSON.stringify(value, null, 2)}\n`);
  await fs.rename(temporaryPath, filePath);
}

const extractedLines = await extractDialogueLines();
const lines = deduplicateLines(extractedLines);
console.log(
  `Dialogue inventory: ${extractedLines.length} entries, ${lines.length} unique character/text clips.`,
);

if (listLines) {
  for (const line of lines) {
    console.log(
      `${line.characterId}\t${line.dialogueKey}\t${line.emotion}\t${filenameFor(line)}\t${line.text}`,
    );
  }
}

if (dryRun) {
  console.log(`Dry run complete: ${lines.length} Gemini requests required for an empty pack.`);
  process.exit(0);
}

const devVars = await loadDevVars();
const apiKey = process.env.GEMINI_API_KEY || devVars.GEMINI_API_KEY;
if (!apiKey) {
  throw new Error("GEMINI_API_KEY is required in the environment or web/.dev.vars.");
}
const model = process.env.TABLE_TALK_TTS_MODEL
  || devVars.TABLE_TALK_TTS_MODEL
  || "gemini-3.1-flash-tts-preview";

await fs.mkdir(outputDir, { recursive: true });
const manifest = {};
const failures = [];
let generated = 0;
let reused = 0;

for (const [index, line] of lines.entries()) {
  const filename = filenameFor(line);
  const outputPath = path.join(outputDir, filename);
  const manifestKey = `${line.characterId}|${line.text}`;
  const progress = `[${index + 1}/${lines.length}]`;
  if (await isValidWebm(outputPath)) {
    reused++;
    manifest[manifestKey] = `table-talk-voice/${filename}`;
    console.log(`${progress} Reused ${line.characterId}: "${line.text}"`);
    continue;
  }

  const temporaryPath = `${outputPath}.tmp-${process.pid}`;
  try {
    await fs.rm(temporaryPath, { force: true });
    const { pcm, sampleRate } = await fetchGeminiAudioWithRetry(line, apiKey, model);
    await encodeWebmOpus(pcm, sampleRate, temporaryPath);
    if (!(await isValidWebm(temporaryPath))) {
      throw new Error("Encoder produced an invalid or empty WebM file.");
    }
    await fs.rename(temporaryPath, outputPath);
    generated++;
    manifest[manifestKey] = `table-talk-voice/${filename}`;
    console.log(`${progress} Generated ${line.characterId}: "${line.text}"`);
  } catch (error) {
    await fs.rm(temporaryPath, { force: true });
    failures.push({ ...line, error: error instanceof Error ? error.message : String(error) });
    console.error(`${progress} Failed ${line.characterId}: "${line.text}" — ${failures.at(-1).error}`);
  }
}

await writeJsonAtomically(path.join(outputDir, "manifest.json"), manifest);
console.log(
  `Voice pack finished: ${generated} generated, ${reused} reused, `
  + `${failures.length} failed, ${Object.keys(manifest).length} manifest entries.`,
);

if (failures.length > 0) {
  console.error("\nFailed lines (rerun the command to resume):");
  for (const failure of failures) {
    console.error(`- ${failure.characterId}/${failure.dialogueKey}: "${failure.text}" — ${failure.error}`);
  }
  process.exitCode = 1;
}

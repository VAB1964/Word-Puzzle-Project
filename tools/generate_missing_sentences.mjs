#!/usr/bin/env node

import fs from "node:fs";
import path from "node:path";
import process from "node:process";

const ROOT = path.resolve(import.meta.dirname, "..");
const DEFAULT_INPUT = path.join(ROOT, "esdb-candidate", "words_processed.csv");
const DEFAULT_OUTPUT = path.join(ROOT, "tools", "generated_sentences.json");
const MODEL = "gpt-4.1-mini";
const BATCH_SIZE = 40;

const parseArgs = () => {
  const args = { input: DEFAULT_INPUT, output: DEFAULT_OUTPUT, concurrency: 5, limit: Infinity };
  for (let i = 2; i < process.argv.length; i += 1) {
    const value = process.argv[i + 1];
    if (process.argv[i] === "--input") args.input = path.resolve(value), i += 1;
    else if (process.argv[i] === "--output") args.output = path.resolve(value), i += 1;
    else if (process.argv[i] === "--concurrency") args.concurrency = Number(value), i += 1;
    else if (process.argv[i] === "--limit") args.limit = Number(value), i += 1;
    else throw new Error(`Unknown argument: ${process.argv[i]}`);
  }
  if (!Number.isInteger(args.concurrency) || args.concurrency < 1 || args.concurrency > 20) {
    throw new Error("--concurrency must be an integer from 1 to 20");
  }
  return args;
};

const loadEnv = () => {
  const envPath = path.join(ROOT, ".env");
  if (!fs.existsSync(envPath)) return;
  for (const line of fs.readFileSync(envPath, "utf8").split(/\r?\n/)) {
    const match = line.match(/^\s*([^#=\s]+)\s*=\s*(.*)\s*$/);
    if (!match || process.env[match[1]]) continue;
    let value = match[2];
    if ((value.startsWith('"') && value.endsWith('"')) || (value.startsWith("'") && value.endsWith("'"))) {
      value = value.slice(1, -1);
    }
    process.env[match[1]] = value;
  }
};

const parseCsv = (text) => {
  const records = [];
  let fields = [];
  let field = "";
  let quoted = false;
  for (let index = 0; index < text.length; index += 1) {
    const character = text[index];
    if (character === '"') {
      if (quoted && text[index + 1] === '"') field += text[index++];
      else quoted = !quoted;
    } else if (character === "," && !quoted) {
      fields.push(field);
      field = "";
    } else if ((character === "\n" || character === "\r") && !quoted) {
      if (character === "\r" && text[index + 1] === "\n") index += 1;
      fields.push(field);
      field = "";
      if (fields.some((value) => value.length > 0)) records.push(fields);
      fields = [];
    } else {
      field += character;
    }
  }
  if (field.length > 0 || fields.length > 0) {
    fields.push(field);
    records.push(fields);
  }
  const headers = records.shift() ?? [];
  return records.map((record) => Object.fromEntries(headers.map((header, index) => [header, record[index] ?? ""])));
};

const wordPattern = (word) => new RegExp(`(^|[^A-Za-z])${word}([^A-Za-z]|$)`, "i");
const validateSentence = (word, sentence) => {
  if (typeof sentence !== "string") return "not a string";
  const normalized = sentence.replace(/\s+/g, " ").trim();
  const words = normalized.match(/[A-Za-z]+(?:['-][A-Za-z]+)*/g) ?? [];
  if (normalized.length < 12 || normalized.length > 180) return "length outside 12-180 characters";
  if (words.length < 4 || words.length > 20) return "word count outside 4-20";
  if (!wordPattern(word).test(normalized)) return "does not contain the exact spelling";
  if (!/^[A-Z]/.test(normalized) || !/[.!?]$/.test(normalized)) return "not a complete sentence";
  if ((normalized.match(/[.!?]/g) ?? []).length !== 1) return "not exactly one sentence";
  if (/^(?:[\w-]+ means\b|[\w-]+ is a word\b|To [\w-]+ is\b)/i.test(normalized)) {
    return "definition-restatement template";
  }
  return null;
};

const loadExisting = (output) => {
  if (!fs.existsSync(output)) return { schema_version: 1, model: MODEL, entries: {} };
  const value = JSON.parse(fs.readFileSync(output, "utf8"));
  if (value.schema_version !== 1 || typeof value.entries !== "object") {
    throw new Error(`Invalid checkpoint: ${output}`);
  }
  return value;
};

const save = (output, checkpoint) => {
  const temp = `${output}.tmp`;
  const ordered = Object.fromEntries(Object.entries(checkpoint.entries).sort(([left], [right]) => left.localeCompare(right)));
  fs.writeFileSync(temp, `${JSON.stringify({ ...checkpoint, entries: ordered }, null, 2)}\n`, "utf8");
  fs.renameSync(temp, output);
};

const schemaFor = (items) => ({
  name: "dictionary_examples",
  strict: true,
  schema: {
    type: "object",
    properties: {
      examples: {
        type: "array",
        minItems: items.length,
        maxItems: items.length,
        items: {
          type: "object",
          properties: {
            word: { type: "string", enum: items.map((item) => item.word) },
            sentence: { type: "string" }
          },
          required: ["word", "sentence"],
          additionalProperties: false
        }
      }
    },
    required: ["examples"],
    additionalProperties: false
  }
});

const systemPrompt = `Write one original example sentence for each dictionary entry.
Rules:
- The sentence MUST literally contain the exact supplied spelling as a standalone word, with the supplied part of speech and meaning.
- Never replace that spelling with a synonym, lemma, corrected spelling, or related form, even when the definition uses one.
- Demonstrate the meaning naturally in context; do not define or discuss the word itself.
- Write exactly one modern, grammatical, family-friendly sentence of 4-20 words.
- Preserve the exact spelling even for plurals, conjugations, comparatives, and unusual words.
- Do not use quotations, dialogue, fragments, proper names, or a second sentence.
- Return every requested word exactly once.`;

const requestExamples = async (apiKey, items) => {
  const response = await fetch("https://api.openai.com/v1/chat/completions", {
    method: "POST",
    headers: { "Authorization": `Bearer ${apiKey}`, "Content-Type": "application/json" },
    body: JSON.stringify({
      model: MODEL,
      messages: [
        { role: "system", content: systemPrompt },
        {
          role: "user",
          content: JSON.stringify(items.map((item) => ({ ...item, required_literal: item.word })))
        }
      ],
      response_format: { type: "json_schema", json_schema: schemaFor(items) },
      temperature: 0.2
    })
  });
  const body = await response.json();
  if (!response.ok) {
    const error = new Error(body?.error?.message ?? `OpenAI API returned ${response.status}`);
    error.status = response.status;
    throw error;
  }
  const parsed = JSON.parse(body.choices?.[0]?.message?.content ?? "");
  return { examples: parsed.examples, usage: body.usage ?? {} };
};

const sleep = (milliseconds) => new Promise((resolve) => setTimeout(resolve, milliseconds));

const generateBatch = async (apiKey, items, attempts = 0) => {
  try {
    const result = await requestExamples(apiKey, items);
    const byWord = new Map(result.examples.map((example) => [example.word, example.sentence.replace(/\s+/g, " ").trim()]));
    const errors = items.flatMap((item) => {
      const sentence = byWord.get(item.word);
      const error = validateSentence(item.word, sentence);
      return error ? [`${item.word}: ${error}`] : [];
    });
    if (byWord.size !== items.length || errors.length > 0) throw new Error(errors.slice(0, 5).join("; "));
    return { byWord, usage: result.usage };
  } catch (error) {
    const retryable = error.status === 429 || (error.status ?? 500) >= 500 || attempts < 2;
    if (retryable && attempts < 4) {
      await sleep(Math.min(30_000, 1_000 * 2 ** attempts));
      return generateBatch(apiKey, items, attempts + 1);
    }
    if (items.length > 1) {
      const middle = Math.ceil(items.length / 2);
      const left = await generateBatch(apiKey, items.slice(0, middle));
      const right = await generateBatch(apiKey, items.slice(middle));
      return {
        byWord: new Map([...left.byWord, ...right.byWord]),
        usage: {
          prompt_tokens: (left.usage.prompt_tokens ?? 0) + (right.usage.prompt_tokens ?? 0),
          completion_tokens: (left.usage.completion_tokens ?? 0) + (right.usage.completion_tokens ?? 0),
          total_tokens: (left.usage.total_tokens ?? 0) + (right.usage.total_tokens ?? 0)
        }
      };
    }
    throw new Error(`Could not generate ${items[0].word}: ${error.message}`);
  }
};

const main = async () => {
  const args = parseArgs();
  loadEnv();
  const apiKey = process.env.OPENAI_API_KEY || process.env.VITE_OPENAI_API_KEY;
  if (!apiKey) throw new Error("Set OPENAI_API_KEY or VITE_OPENAI_API_KEY in the environment or .env file.");

  const rows = parseCsv(fs.readFileSync(args.input, "utf8"));
  const checkpoint = loadExisting(args.output);
  const pending = rows
    .filter((row) => !row.Sentence.trim())
    .filter((row) => {
      const existing = checkpoint.entries[row.word];
      return !existing || existing.pos !== row.pos || existing.definition !== row.Definition || validateSentence(row.word, existing.sentence);
    })
    .slice(0, args.limit)
    .map((row) => ({ word: row.word, pos: row.pos, definition: row.Definition }));

  const batches = [];
  for (let index = 0; index < pending.length; index += BATCH_SIZE) batches.push(pending.slice(index, index + BATCH_SIZE));
  let nextBatch = 0;
  let completed = 0;
  const usage = { prompt_tokens: 0, completion_tokens: 0, total_tokens: 0 };
  console.log(`Generating ${pending.length} examples in ${batches.length} batches with ${MODEL}.`);

  const worker = async () => {
    while (true) {
      const batchIndex = nextBatch++;
      if (batchIndex >= batches.length) return;
      const batch = batches[batchIndex];
      const result = await generateBatch(apiKey, batch);
      for (const item of batch) {
        checkpoint.entries[item.word] = {
          pos: item.pos,
          definition: item.definition,
          sentence: result.byWord.get(item.word),
          model: MODEL
        };
      }
      for (const key of Object.keys(usage)) usage[key] += result.usage[key] ?? 0;
      completed += batch.length;
      save(args.output, checkpoint);
      console.log(`Completed ${completed}/${pending.length}; tokens ${usage.total_tokens}.`);
    }
  };

  await Promise.all(Array.from({ length: Math.min(args.concurrency, batches.length) }, () => worker()));
  console.log(JSON.stringify({ generated: completed, total_entries: Object.keys(checkpoint.entries).length, usage }, null, 2));
};

main().catch((error) => {
  console.error(error instanceof Error ? error.message : error);
  process.exitCode = 1;
});

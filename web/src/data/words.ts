import type { WordInfo } from "../core/types";

const trimString = (input: string) => input.trim();

const parseCsvLine = (line: string) => {
  const fields: string[] = [];
  let field = "";
  let inQuotes = false;

  for (let i = 0; i < line.length; i += 1) {
    const c = line[i];
    if (c === '"') {
      if (inQuotes && i + 1 < line.length && line[i + 1] === '"') {
        field += '"';
        i += 1;
      } else {
        inQuotes = !inQuotes;
      }
    } else if (c === "," && !inQuotes) {
      fields.push(field);
      field = "";
    } else {
      field += c;
    }
  }
  fields.push(field);
  return fields;
};

const readCsvRecords = (text: string) => {
  const lines = text.split(/\r?\n/);
  const records: string[] = [];
  let current = "";
  let inQuotes = false;

  for (const line of lines) {
    if (current) current += "\n";
    current += line;

    for (let i = 0; i < line.length; i += 1) {
      if (line[i] === '"') {
        if (inQuotes && i + 1 < line.length && line[i + 1] === '"') {
          i += 1;
        } else {
          inQuotes = !inQuotes;
        }
      }
    }

    if (!inQuotes) {
      records.push(current);
      current = "";
    }
  }

  if (current) records.push(current);
  return records;
};

const emptyWordInfo = (): WordInfo => ({
  text: "",
  rarity: 0,
  pos: "",
  definition: "",
  sentence: "",
  avgSubLen: 0,
  countGE3: 0,
  countGE4: 0,
  countGE5: 0,
  easyValidCount: 0,
  mediumValidCount: 0,
  hardValidCount: 0
});

export const loadProcessedWordList = async (url: string) => {
  const res = await fetch(url);
  if (!res.ok) {
    throw new Error(`Failed to load word list: ${res.status} ${res.statusText}`);
  }

  const text = await res.text();
  const records = readCsvRecords(text);
  if (records.length <= 1) return [];

  const wordList: WordInfo[] = [];
  for (let i = 1; i < records.length; i += 1) {
    const record = records[i];
    if (!record.trim()) continue;

    try {
      const fields = parseCsvLine(record);
      const info = emptyWordInfo();
      if (fields.length > 0) {
        info.text = trimString(fields[0]).toLowerCase();
      }
      if (fields.length > 1) info.rarity = parseInt(trimString(fields[1]) || "0", 10);
      if (fields.length > 2) info.pos = trimString(fields[2]);
      if (fields.length > 3) info.definition = trimString(fields[3]);
      if (fields.length > 4) info.sentence = trimString(fields[4]);

      if (fields.length > 5) info.avgSubLen = parseFloat(trimString(fields[5]) || "0");
      if (fields.length > 6) info.countGE3 = parseInt(trimString(fields[6]) || "0", 10);
      if (fields.length > 7) info.countGE4 = parseInt(trimString(fields[7]) || "0", 10);
      if (fields.length > 8) info.countGE5 = parseInt(trimString(fields[8]) || "0", 10);
      if (fields.length > 9) info.easyValidCount = parseInt(trimString(fields[9]) || "0", 10);
      if (fields.length > 10) info.mediumValidCount = parseInt(trimString(fields[10]) || "0", 10);
      if (fields.length > 11) info.hardValidCount = parseInt(trimString(fields[11]) || "0", 10);

      if (info.text) {
        wordList.push(info);
      }
    } catch (err) {
      console.warn("Skipping malformed word record", err);
    }
  }

  return wordList;
};

export const withLength = (wordList: WordInfo[], len: number) =>
  wordList.filter((info) => info.text.length === len);

export const isSubWord = (sub: string, base: string) => {
  if (!sub || sub.length > base.length) return false;
  if (sub.toLowerCase() === base.toLowerCase()) return false;

  const baseFreq = new Map<string, number>();
  for (const c of base.toLowerCase()) {
    baseFreq.set(c, (baseFreq.get(c) ?? 0) + 1);
  }

  const subFreq = new Map<string, number>();
  for (const c of sub.toLowerCase()) {
    subFreq.set(c, (subFreq.get(c) ?? 0) + 1);
  }

  for (const [char, count] of subFreq.entries()) {
    if ((baseFreq.get(char) ?? 0) < count) return false;
  }
  return true;
};

export const subWords = (base: string, wordList: WordInfo[]) => {
  if (!base) return [];
  const lowerBase = base.toLowerCase();
  const baseFreq = new Map<string, number>();
  for (const c of lowerBase) {
    baseFreq.set(c, (baseFreq.get(c) ?? 0) + 1);
  }

  const result: WordInfo[] = [];
  for (const info of wordList) {
    const word = info.text.toLowerCase();
    if (!word || word.length > lowerBase.length) continue;
    if (word === lowerBase) continue;

    const wordFreq = new Map<string, number>();
    let possible = true;
    for (const c of word) {
      wordFreq.set(c, (wordFreq.get(c) ?? 0) + 1);
      if ((baseFreq.get(c) ?? 0) < (wordFreq.get(c) ?? 0)) {
        possible = false;
        break;
      }
    }
    if (possible) result.push(info);
  }
  return result;
};

export const sortForGrid = (words: WordInfo[]) =>
  [...words].sort((a, b) => {
    if (a.text.length !== b.text.length) {
      return a.text.length - b.text.length;
    }
    return a.text.localeCompare(b.text, undefined, { sensitivity: "base" });
  });

import type {
  Difficulty,
  GameMode,
  PuzzleCellRef,
  PuzzleDefinition,
  PuzzleWordDefinition
} from "./types";
import { Direction, generateCrossword } from "../../web/src/data/crossword";
import type { WordInfo } from "../../web/src/core/types";

export interface PuzzleWordData {
  text: string;
  rarity: number;
  countGE4: number;
  easyValidCount: number;
  mediumValidCount: number;
  hardValidCount: number;
}

const splitCsvRecords = (text: string) => {
  const records: string[] = [];
  let record = "";
  let quoted = false;
  for (let index = 0; index < text.length; index += 1) {
    const character = text[index];
    record += character;
    if (character === '"') {
      if (quoted && text[index + 1] === '"') {
        record += text[++index];
      } else {
        quoted = !quoted;
      }
    } else if (character === "\n" && !quoted) {
      records.push(record.trimEnd());
      record = "";
    }
  }
  if (record.trim()) records.push(record);
  return records;
};

const splitCsvFields = (record: string) => {
  const fields: string[] = [];
  let field = "";
  let quoted = false;
  for (let index = 0; index < record.length; index += 1) {
    const character = record[index];
    if (character === '"') {
      if (quoted && record[index + 1] === '"') {
        field += '"';
        index += 1;
      } else {
        quoted = !quoted;
      }
    } else if (character === "," && !quoted) {
      fields.push(field);
      field = "";
    } else {
      field += character;
    }
  }
  fields.push(field);
  return fields;
};

export const parseMultiplayerWordData = (csv: string): PuzzleWordData[] =>
  splitCsvRecords(csv)
    .slice(1)
    .map(splitCsvFields)
    .map((fields) => ({
      text: (fields[0] ?? "").trim().toLowerCase(),
      rarity: Number.parseInt(fields[1] ?? "0", 10) || 0,
      countGE4: Number.parseInt(fields[7] ?? "0", 10) || 0,
      easyValidCount: Number.parseInt(fields[9] ?? "0", 10) || 0,
      mediumValidCount: Number.parseInt(fields[10] ?? "0", 10) || 0,
      hardValidCount: Number.parseInt(fields[11] ?? "0", 10) || 0
    }))
    .filter((word) => /^[a-z]+$/.test(word.text) && word.text.length >= 3 && word.text.length <= 7);

const hashSeed = (seed: string) => {
  let hash = 2166136261;
  for (let index = 0; index < seed.length; index += 1) {
    hash ^= seed.charCodeAt(index);
    hash = Math.imul(hash, 16777619);
  }
  return hash >>> 0;
};

const randomForSeed = (seed: string) => {
  let value = hashSeed(seed) || 1;
  return () => {
    value ^= value << 13;
    value ^= value >>> 17;
    value ^= value << 5;
    return (value >>> 0) / 4294967296;
  };
};

const shuffled = <T>(items: T[], random: () => number) => {
  const copy = [...items];
  for (let index = copy.length - 1; index > 0; index -= 1) {
    const swap = Math.floor(random() * (index + 1));
    [copy[index], copy[swap]] = [copy[swap], copy[index]];
  }
  return copy;
};

const canSpell = (word: string, letters: string) => {
  const available = new Map<string, number>();
  for (const letter of letters) available.set(letter, (available.get(letter) ?? 0) + 1);
  for (const letter of word) {
    const count = available.get(letter) ?? 0;
    if (count === 0) return false;
    available.set(letter, count - 1);
  }
  return true;
};

const limits = {
  Casual: { Easy: 7, Medium: 12, Hard: 15 },
  Crossword: { Easy: 10, Medium: 15, Hard: 20 }
} satisfies Record<GameMode, Record<Difficulty, number>>;

const allowedRarities = (difficulty: Difficulty, lastPuzzle: boolean) => {
  if (difficulty === "Easy") return [1, 2];
  if (difficulty === "Medium") return [1, 2, 3];
  return lastPuzzle ? [2, 3, 4] : [2, 3, 4];
};

const chooseBase = (
  words: PuzzleWordData[],
  difficulty: Difficulty,
  lastPuzzle: boolean,
  usedBases: Set<string>,
  random: () => number
) => {
  const baseRarities =
    difficulty === "Easy" ? [1] : difficulty === "Medium" ? [1, 2, 3] : lastPuzzle ? [4] : [3, 4];
  const candidates = words.filter((word) => {
    if (word.text.length !== 7 || usedBases.has(word.text) || !baseRarities.includes(word.rarity)) return false;
    const count =
      difficulty === "Easy"
        ? word.easyValidCount
        : difficulty === "Medium"
          ? word.mediumValidCount
          : word.hardValidCount;
    return count >= 5;
  });
  const fallback = words.filter(
    (word) =>
      word.text.length === 7 &&
      !usedBases.has(word.text) &&
      baseRarities.includes(word.rarity)
  );
  const pool = candidates.length > 0 ? candidates : fallback;
  if (pool.length === 0) {
    throw new Error(`No eligible ${difficulty.toLowerCase()} seven-letter base words are available.`);
  }
  return pool[Math.floor(random() * pool.length)];
};

interface Placed {
  word: PuzzleWordData;
  row: number;
  col: number;
  vertical: boolean;
}

const coordinates = (placed: Placed, index: number): PuzzleCellRef => ({
  row: placed.row + (placed.vertical ? index : 0),
  col: placed.col + (placed.vertical ? 0 : index)
});

const crosswordLayout = (words: PuzzleWordData[]) => {
  const byText = new Map(words.map((word) => [word.text, word]));
  const singlePlayerWords: WordInfo[] = words.map((word) => ({
    ...word,
    pos: "",
    definition: "",
    sentence: "",
    avgSubLen: 0,
    countGE3: 0,
    countGE5: 0
  }));
  const layout = generateCrossword(singlePlayerWords);
  const placed = layout.placedWords.map((word, index) => {
    const placement = layout.placements[index];
    return {
      word: byText.get(word.text) ?? words[0],
      row: placement.gridRow,
      col: placement.gridCol,
      vertical: placement.dir === Direction.Vertical
    };
  });
  return { placed, rows: layout.gridRows, cols: layout.gridCols };
};

export const generateMultiplayerPuzzle = (
  data: PuzzleWordData[],
  mode: GameMode,
  difficulty: Difficulty,
  seed: string,
  puzzleIndex: number,
  puzzleCount: number,
  usedBases: Set<string>
): PuzzleDefinition => {
  const random = randomForSeed(seed);
  const baseWord = chooseBase(data, difficulty, puzzleIndex === puzzleCount - 1, usedBases, random);
  usedBases.add(baseWord.text);
  const minimumLength = difficulty === "Hard" ? 4 : 3;
  const rarities = allowedRarities(difficulty, puzzleIndex === puzzleCount - 1);
  const possible = data.filter(
    (word) =>
      word.text.length >= minimumLength &&
      rarities.includes(word.rarity) &&
      canSpell(word.text, baseWord.text)
  );
  const unique = new Map(possible.map((word) => [word.text, word]));
  if (!unique.has(baseWord.text)) unique.set(baseWord.text, baseWord);
  const ordered = [...unique.values()].sort(
    (left, right) =>
      right.text.length - left.text.length ||
      left.rarity - right.rarity ||
      left.text.localeCompare(right.text)
  );
  const selected = ordered.slice(0, limits[mode][difficulty]);
  const selectedSet = new Set(selected.map((word) => word.text));
  const bonusWords = possible.map((word) => word.text).filter((word) => !selectedSet.has(word));

  let placed: Placed[];
  let rows: number;
  let cols: number;
  if (mode === "Crossword") {
    const layout = crosswordLayout(selected);
    placed = layout.placed;
    rows = layout.rows;
    cols = layout.cols;
  } else {
    placed = selected.map((word, row) => ({ word, row, col: 0, vertical: false }));
    rows = placed.length;
    cols = Math.max(...selected.map((word) => word.text.length));
  }

  const words: PuzzleWordDefinition[] = placed.map((entry, index) => ({
    id: `w${index}`,
    answer: entry.word.text,
    rarity: entry.word.rarity,
    cells: entry.word.text.split("").map((_, position) => coordinates(entry, position))
  }));
  return {
    id: `${seed}-${puzzleIndex}`,
    mode,
    baseLetters: shuffled(baseWord.text.split(""), random).join("").toUpperCase(),
    rows,
    cols,
    words,
    bonusWords
  };
};

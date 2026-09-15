import { readFileSync } from "node:fs";
import { afterEach, describe, expect, it, vi } from "vitest";
import { loadProcessedWordList } from "../../web/src/data/words";
import { generateMultiplayerPuzzle, parseMultiplayerWordData } from "../../shared/multiplayer/puzzles";

const csv = readFileSync(new URL("../../words_processed.csv", import.meta.url), "utf8");
const data = parseMultiplayerWordData(csv);

afterEach(() => vi.unstubAllGlobals());

describe("published dictionary", () => {
  it("loads the same unique vocabulary and rarities in the browser and Worker", async () => {
    vi.stubGlobal("fetch", vi.fn().mockResolvedValue(new Response(csv)));
    const browserWords = await loadProcessedWordList("/words.csv");
    expect(browserWords.map(({ text, rarity }) => ({ text, rarity }))).toEqual(
      data.map(({ text, rarity }) => ({ text, rarity }))
    );
    expect(new Set(data.map((w) => w.text)).size).toBe(data.length);
    expect(browserWords.every((w) => /^[a-z]{3,7}$/.test(w.text))).toBe(true);
    expect(browserWords.every((w) => w.definition && !w.definition.includes("\uFFFD"))).toBe(true);
    expect(browserWords.every((w) => !/^(?:\w+ (?:means\b|is a word\b)|To \w+ is\b)/i.test(w.sentence))).toBe(true);
    const words = new Map(browserWords.map((w) => [w.text, w]));
    for (const word of ["true", "okay", "ought", "cat", "dog", "dogs", "ran", "running", "meant", "stolen", "halt", "halted", "corgi", "sub", "specs", "teddy", "cafe", "naive"]) {
      expect(words.has(word), word).toBe(true);
    }
    for (const word of ["abt", "adv", "bsc", "i18n", "decieve", "potatos", "pre"]) {
      expect(words.has(word), word).toBe(false);
    }
    expect(words.get("apple")?.rarity).toBe(2);
    expect(words.get("apple")?.definition).toContain("fruit");
    expect(words.get("train")?.definition).toContain("railway");
    expect(words.get("orange")?.definition).toContain("citrus fruit");
    expect(words.get("abs")?.definition).toContain("abdominal muscles");
  });

  it("ships identical web/Worker and standalone dictionaries", () => {
    expect(readFileSync(new URL("../../Standalone/words_processed.csv", import.meta.url), "utf8")).toBe(csv);
  });

  for (const mode of ["Casual", "Crossword"] as const) {
    for (const difficulty of ["Easy", "Medium", "Hard"] as const) {
      it(`generates a five-puzzle ${difficulty} ${mode} session`, () => {
        const used = new Set<string>();
        for (let index = 0; index < 5; index += 1) {
          const puzzle = generateMultiplayerPuzzle(data, mode, difficulty, `dictionary-${mode}-${difficulty}-${index}`, index, 5, used);
          expect(puzzle.words.length).toBeGreaterThan(0);
          const available = puzzle.baseLetters.toLowerCase().split("");
          for (const word of [...puzzle.words.map((w) => w.answer), ...puzzle.bonusWords]) {
            const remaining = [...available];
            for (const letter of word) {
              const position = remaining.indexOf(letter);
              expect(position, `${word} from ${puzzle.baseLetters}`).toBeGreaterThanOrEqual(0);
              remaining.splice(position, 1);
            }
          }
          if (difficulty === "Easy") expect(puzzle.words.every((w) => w.rarity <= 2)).toBe(true);
          if (difficulty === "Medium") expect(puzzle.words.every((w) => w.rarity <= 3)).toBe(true);
          const cells = new Map<string, string>();
          for (const word of puzzle.words) {
            word.cells.forEach((cell, letterIndex) => {
              const key = `${cell.row},${cell.col}`;
              if (cells.has(key)) expect(cells.get(key)).toBe(word.answer[letterIndex]);
              cells.set(key, word.answer[letterIndex]);
            });
          }
        }
        expect(used.size).toBe(5);
      });
    }
  }
});

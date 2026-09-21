import { readFileSync } from "node:fs";
import { afterEach, describe, expect, it, vi } from "vitest";
import { loadProcessedWordList, subWords, puzzleWordCandidates } from "../../web/src/data/words";
import { generateMultiplayerPuzzle, parseMultiplayerWordData } from "../../shared/multiplayer/puzzles";

const csv = readFileSync(new URL("../../words_processed.csv", import.meta.url), "utf8");
const data = parseMultiplayerWordData(csv);

afterEach(() => vi.unstubAllGlobals());

describe("published dictionary", () => {
  it("loads the same unique vocabulary and rarities in the browser and Worker", async () => {
    vi.stubGlobal("fetch", vi.fn().mockResolvedValue(new Response(csv)));
    const browserWords = await loadProcessedWordList("/words.csv");
    expect(browserWords.map(({ text, rarity, pos, definition, sentence }) => ({ text, rarity, pos, definition, sentence }))).toEqual(
      data.map(({ text, rarity, pos, definition, sentence }) => ({ text, rarity, pos, definition, sentence }))
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

  it("includes modesty, some, and most in the single-player candidate pool", async () => {
    vi.stubGlobal("fetch", vi.fn().mockResolvedValue(new Response(csv)));
    const words = await loadProcessedWordList("/words.csv");
    for (const letters of ["modesty", "ytsedom"]) {
      const possible = subWords(letters, words, true).map((word) => word.text);
      for (const word of ["modesty", "some", "most", "toy", "dye"]) {
        expect(possible, letters).toContain(word);
      }
      expect(possible).not.toContain("moss");
    }
    for (const [rarities, minimumLength] of [[[1, 2], 3], [[1, 2, 3], 3], [[2, 3, 4], 4]] as const) {
      const candidates = puzzleWordCandidates("modesty", words, [...rarities], minimumLength);
      for (const word of ["some", "most", "toy", "dye"]) expect(candidates.all.map((w) => w.text)).toContain(word);
      expect(candidates.board.every((w) => w.text.length >= minimumLength && [...rarities].some((r) => r === w.rarity))).toBe(true);
      if (minimumLength === 4) {
        expect(candidates.board.map((w) => w.text)).not.toContain("some");
        expect(candidates.board.map((w) => w.text)).not.toContain("most");
      }
    }
    // Root-analysis callers retain their original exclusion behavior.
    expect(subWords("modesty", words).map((word) => word.text)).not.toContain("modesty");
  });

  it("accepts zen, zee, and zees as board or bonus words for SNEEZED", () => {
    const available = [..."sneezed"];
    const sneezedWords = data.filter((word) => {
      const remaining = [...available];
      return [...word.text].every((letter) => {
        const index = remaining.indexOf(letter);
        if (index < 0) return false;
        remaining.splice(index, 1);
        return true;
      });
    });

    for (const mode of ["Casual", "Crossword"] as const) {
      const puzzle = generateMultiplayerPuzzle(sneezedWords, mode, "Medium", `sneezed-${mode}`, 0, 1, new Set());
      expect([...puzzle.baseLetters].sort().join("")).toBe([..."SNEEZED"].sort().join(""));
      const boardWords = new Set(puzzle.words.map((word) => word.answer));
      const bonusWords = new Set(puzzle.bonusWords);
      for (const word of ["zen", "zee", "zees"]) {
        expect(boardWords.has(word) || bonusWords.has(word), `${word} in ${mode}`).toBe(true);
      }
    }
  });

  it("includes every word from the multiplayer playtest report", () => {
    const playable = new Set(data.map((word) => word.text));
    for (const word of ["rinse", "tinge", "sours", "tills", "bing"]) {
      expect(playable.has(word), word).toBe(true);
    }
  });

  it("ships identical web/Worker and standalone dictionaries", () => {
    expect(readFileSync(new URL("../../Standalone/words_processed.csv", import.meta.url), "utf8")).toBe(csv);
  });

  it("uses American size-60 vocabulary plus explicitly reviewed editorial words", () => {
    const base = new Set(readFileSync(new URL("../../data/esdb/american-60.txt", import.meta.url), "utf8").trim().split(/\r?\n/));
    const editorial = JSON.parse(readFileSync(new URL("../../tools/esdb_editorial.json", import.meta.url), "utf8"));
    const approved = new Set(Object.keys(editorial.larger_level_approvals));
    const supplemental = new Set(Object.keys(editorial.supplemental_entries));
    const words = new Map(data.map((word) => [word.text, word]));
    expect(data.every((word) => base.has(word.text) || approved.has(word.text) || supplemental.has(word.text))).toBe(true);
    for (const word of ["color", "center", "honor", "theater", "aisle", "aisles", "awesome", "accepts", "acted"]) {
      expect(words.has(word), word).toBe(true);
    }
    for (const word of ["colour", "centre", "honour", "theatre", "india", "york", "english", "french"]) {
      expect(words.has(word), word).toBe(false);
    }
    for (const word of approved) expect(words.get(word)?.rarity, word).toBe(4);
    for (const word of supplemental) expect(words.has(word), word).toBe(true);
    const pending = readFileSync(new URL("../../docs/esdb-audit/pending-definitions.csv", import.meta.url), "utf8").trim().split(/\r?\n/).slice(1);
    expect(pending.every((row) => !words.has(row.split(",")[0]))).toBe(true);
  });

  for (const mode of ["Casual", "Crossword"] as const) {
    for (const difficulty of ["Easy", "Medium", "Hard"] as const) {
      it(`generates a five-puzzle ${difficulty} ${mode} session`, () => {
        const used = new Set<string>();
        for (let index = 0; index < 5; index += 1) {
          const puzzle = generateMultiplayerPuzzle(data, mode, difficulty, `dictionary-${mode}-${difficulty}-${index}`, index, 5, used);
          expect(puzzle.words.length).toBeGreaterThan(0);
          expect(puzzle.words.every((word) => Boolean(word.definition))).toBe(true);
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
          const expected = data.filter((word) => {
            const letters = [...available];
            return [...word.text].every((letter) => {
              const index = letters.indexOf(letter);
              if (index < 0) return false;
              letters.splice(index, 1);
              return true;
            });
          }).map((word) => word.text).sort();
          const represented = [...puzzle.words.map((word) => word.answer), ...puzzle.bonusWords];
          expect(represented.slice().sort()).toEqual(expected);
          expect(new Set(represented).size).toBe(represented.length);
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

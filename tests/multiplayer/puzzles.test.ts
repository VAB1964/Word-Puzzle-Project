import * as crossword from "../../web/src/data/crossword";
import { describe, expect, it, vi } from "vitest";
import {
  generateMultiplayerPuzzle,
  type PuzzleWordData
} from "../../shared/multiplayer/puzzles";

const word = (text: string, rarity: number): PuzzleWordData => ({
  text,
  rarity,
  countGE4: 0,
  easyValidCount: 0,
  mediumValidCount: 0,
  hardValidCount: 0
});

describe("multiplayer puzzle difficulty", () => {
  it("keeps Easy base-word fallback restricted to Easy rarity", () => {
    const data = [
      word("axolotl", 4),
      word("teacher", 1),
      word("teach", 1),
      word("each", 1),
      word("chat", 2),
      word("hat", 1),
      word("cat", 1)
    ];

    const puzzle = generateMultiplayerPuzzle(
      data,
      "Casual",
      "Easy",
      "easy-test",
      0,
      5,
      new Set()
    );

    expect(puzzle.baseLetters.toLowerCase().split("").sort().join("")).toBe(
      "teacher".split("").sort().join("")
    );
    expect(puzzle.words.every((candidate) => candidate.rarity <= 2)).toBe(true);
  });

  it("does not substitute a rarity-four base when Easy has no eligible base", () => {
    expect(() =>
      generateMultiplayerPuzzle(
        [word("axolotl", 4)],
        "Casual",
        "Easy",
        "easy-no-fallback",
        0,
        5,
        new Set()
      )
    ).toThrow("No eligible easy seven-letter base words");
  });

  it("allows rarity-three and rarity-four words as bonus-only on Easy", () => {
    const data = [
      word("resting", 1),
      word("string", 2),
      word("stinger", 2),
      word("sting", 2),
      word("rings", 2),
      word("tiger", 2),
      word("tiers", 1),
      word("tire", 1),
      word("rest", 1),
      word("sign", 1),
      word("resign", 3),
      word("ingerts", 4)
    ];

    const puzzle = generateMultiplayerPuzzle(
      data,
      "Casual",
      "Easy",
      "easy-bonus-rarity3",
      0,
      5,
      new Set()
    );

    expect(puzzle.words.every((candidate) => candidate.rarity <= 2)).toBe(true);
    expect(puzzle.bonusWords).toContain("resign");
    expect(puzzle.bonusWords).toContain("ingerts");
  });

  it("uses single-player-style multi-column casual layout when many words are present", () => {
    const data = [
      word("reactor", 1),
      word("trace", 2),
      word("crate", 2),
      word("cater", 2),
      word("caret", 2),
      word("race", 1),
      word("care", 1),
      word("cart", 1),
      word("rate", 1),
      word("tare", 1),
      word("ace", 1),
      word("act", 1),
      word("arc", 1),
      word("are", 1),
      word("art", 1),
      word("car", 1),
      word("cat", 1)
    ];

    const puzzle = generateMultiplayerPuzzle(
      data,
      "Casual",
      "Easy",
      "casual-column-layout",
      0,
      5,
      new Set()
    );

    expect(puzzle.words.length).toBeGreaterThan(5);
    expect(puzzle.rows).toBeLessThanOrEqual(5);
    const distinctStartCols = new Set(puzzle.words.map((entry) => entry.cells[0]?.col ?? 0));
    expect(distinctStartCols.size).toBeGreaterThan(1);
  });
});


describe("complete puzzle and bonus partition", () => {
  const data = [word("modesty", 3), word("modest", 3), word("some", 1),
    word("most", 1), word("toy", 1), word("dye", 2), word("mode", 4),
    word("moss", 2), word("meet", 1)];

  for (const difficulty of ["Medium", "Hard"] as const) {
    for (const mode of ["Casual", "Crossword"] as const) {
      it(`accepts all modesty words in ${difficulty} ${mode}`, () => {
        const puzzle = generateMultiplayerPuzzle(data, mode, difficulty, "modesty", 0, 5, new Set());
        const represented = [...puzzle.words.map((w) => w.answer), ...puzzle.bonusWords];
        expect(represented.sort()).toEqual(["modesty", "modest", "some", "most", "toy", "dye", "mode"].sort());
        if (difficulty === "Hard") {
          for (const text of ["some", "most", "toy", "dye"]) expect(puzzle.bonusWords).toContain(text);
          expect(puzzle.words.every((w) => w.rarity >= 2 && w.answer.length >= 4)).toBe(true);
        } else {
          expect(puzzle.bonusWords).toContain("mode");
          expect(puzzle.words.every((w) => w.rarity <= 3)).toBe(true);
        }
      });
    }
  }

  it("moves selected words that fail crossword placement to bonuses", () => {
    const original = crossword.generateCrossword;
    const spy = vi.spyOn(crossword, "generateCrossword").mockImplementation((words) => {
      const result = original(words);
      return { ...result, placedWords: result.placedWords.slice(0, 1), placements: result.placements.slice(0, 1) };
    });
    try {
      const puzzle = generateMultiplayerPuzzle(data, "Crossword", "Hard", "placement-loss", 0, 5, new Set());
      expect(puzzle.words).toHaveLength(1);
      const represented = [...puzzle.words.map((w) => w.answer), ...puzzle.bonusWords];
      expect(represented.sort()).toEqual(["modesty", "modest", "some", "most", "toy", "dye", "mode"].sort());
      expect(puzzle.bonusWords).toContain("modest");
    } finally {
      spy.mockRestore();
    }
  });
});

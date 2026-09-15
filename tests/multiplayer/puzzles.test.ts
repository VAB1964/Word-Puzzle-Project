import { describe, expect, it } from "vitest";
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
});

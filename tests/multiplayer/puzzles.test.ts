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
});

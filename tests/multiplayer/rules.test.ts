import { describe, expect, it } from "vitest";
import {
  createPuzzleRuntime,
  submitGuess,
  useHint
} from "../../shared/multiplayer/rules";
import {
  emptyScore,
  puzzleMaximumScore,
  scoreIsConsistent
} from "../../shared/multiplayer/scoring";
import type {
  Participant,
  PuzzleDefinition
} from "../../shared/multiplayer/types";

const participant = (id: string, seat: number, hintCredits = 0): Participant => ({
  id,
  name: id,
  color: seat === 0 ? "#00f" : "#f00",
  seat,
  kind: "human",
  connected: true,
  ready: true,
  replaced: false,
  joinedAt: seat,
  hintCredits,
  score: emptyScore(),
  continued: false
});

const crossword: PuzzleDefinition = {
  id: "crossword",
  mode: "Crossword",
  baseLetters: "CART",
  rows: 3,
  cols: 3,
  bonusWords: ["art"],
  words: [
    {
      id: "cat",
      answer: "cat",
      rarity: 1,
      gems: ["none", "none", "none"],
      cells: [
        { row: 0, col: 0 },
        { row: 0, col: 1 },
        { row: 0, col: 2 }
      ]
    },
    {
      id: "car",
      answer: "car",
      rarity: 1,
      gems: ["none", "none", "none"],
      cells: [
        { row: 0, col: 0 },
        { row: 1, col: 0 },
        { row: 2, col: 0 }
      ]
    }
  ]
};

describe("multiplayer scoring rules", () => {
  it("scores an intersection once per word while preserving its first cell owner", () => {
    const players = [participant("Alice", 0), participant("Bob", 1)];
    const runtime = createPuzzleRuntime(crossword);

    expect(submitGuess(crossword, runtime, players, "Alice", "cat").pointsAwarded).toBe(3);
    expect(submitGuess(crossword, runtime, players, "Bob", "car").pointsAwarded).toBe(3);

    expect(runtime.visibleCells["0,0"].ownerId).toBe("Alice");
    expect(runtime.credits.cat[0]?.ownerId).toBe("Alice");
    expect(runtime.credits.car[0]?.ownerId).toBe("Bob");
    expect(players[0].score.total).toBe(3);
    expect(players[1].score.total).toBe(3);
  });

  it("reserves power-up points immediately and gives a later solver only the remainder", () => {
    const puzzle: PuzzleDefinition = {
      id: "gems",
      mode: "Casual",
      baseLetters: "STONE",
      rows: 1,
      cols: 5,
      bonusWords: [],
      words: [
        {
          id: "stone",
          answer: "stone",
          rarity: 4,
          gems: ["emerald", "emerald", "emerald", "emerald", "emerald"],
          cells: Array.from({ length: 5 }, (_, col) => ({ row: 0, col }))
        }
      ]
    };
    const players = [participant("Alice", 0, 20), participant("Bob", 1)];
    const runtime = createPuzzleRuntime(puzzle);

    const hint = useHint(puzzle, runtime, players, "Alice", {
      hint: "letter",
      wordId: "stone",
      position: 0
    });
    const solve = submitGuess(puzzle, runtime, players, "Bob", "stone");

    expect(hint.pointsAwarded).toBe(6);
    expect(solve.pointsAwarded).toBe(24);
    expect(players[0].score.total).toBe(6);
    expect(players[1].score.total).toBe(24);
    expect(players[0].hintCredits).toBe(15);
  });

  it("awards a room-wide bonus word only to its first claimant", () => {
    const players = [participant("Alice", 0), participant("Bob", 1)];
    const runtime = createPuzzleRuntime(crossword);

    expect(submitGuess(crossword, runtime, players, "Alice", "art").hintCreditsAwarded).toBe(1);
    expect(submitGuess(crossword, runtime, players, "Bob", "art").changed).toBe(false);
    expect(players[0].hintCredits).toBe(1);
    expect(players[1].hintCredits).toBe(0);
  });

  it("attributes a crossing-completed word to the action that fills its last cell", () => {
    const players = [participant("Alice", 0), participant("Bob", 1, 10)];
    const runtime = createPuzzleRuntime(crossword);
    useHint(crossword, runtime, players, "Bob", { hint: "letter", wordId: "car", position: 1 });
    useHint(crossword, runtime, players, "Bob", { hint: "letter", wordId: "car", position: 2 });

    const result = submitGuess(crossword, runtime, players, "Alice", "cat");

    expect(result.solvedWords).toEqual(["cat", "car"]);
    expect(result.pointsAwarded).toBe(4);
    expect(runtime.credits.car[0]?.ownerId).toBe("Alice");
    expect(runtime.credits.car[1]?.ownerId).toBe("Bob");
    expect(runtime.credits.car[2]?.ownerId).toBe("Bob");
  });

  it("does not charge for a hint with no eligible target", () => {
    const players = [participant("Alice", 0, 30)];
    const runtime = createPuzzleRuntime(crossword);
    submitGuess(crossword, runtime, players, "Alice", "cat");
    submitGuess(crossword, runtime, players, "Alice", "car");
    const before = players[0].hintCredits;

    const result = useHint(crossword, runtime, players, "Alice", { hint: "random" });

    expect(result.changed).toBe(false);
    expect(players[0].hintCredits).toBe(before);
  });

  it("full-word hint can target a chosen word", () => {
    const players = [participant("Alice", 0, 30)];
    const runtime = createPuzzleRuntime(crossword);

    const result = useHint(crossword, runtime, players, "Alice", {
      hint: "full-word",
      wordId: "car"
    });

    expect(result.changed).toBe(true);
    expect(result.solvedWords).toContain("car");
    expect(runtime.visibleCells["0,0"]).toBeTruthy();
    expect(runtime.visibleCells["1,0"]).toBeTruthy();
    expect(runtime.visibleCells["2,0"]).toBeTruthy();
    expect(players[0].hintCredits).toBe(15);
  });

  it("never exceeds the defined puzzle maximum and keeps totals consistent", () => {
    const players = [participant("Alice", 0, 20), participant("Bob", 1, 20)];
    const runtime = createPuzzleRuntime(crossword);
    useHint(crossword, runtime, players, "Alice", { hint: "random" }, () => 0);
    submitGuess(crossword, runtime, players, "Bob", "cat");
    submitGuess(crossword, runtime, players, "Alice", "car");

    const awarded = players.reduce((sum, player) => sum + player.score.total, 0);
    expect(awarded).toBeLessThanOrEqual(puzzleMaximumScore(crossword.words));
    expect(players.every((player) => scoreIsConsistent(player.score))).toBe(true);
    for (const credits of Object.values(runtime.credits)) {
      expect(credits.every((credit) => credit === null || typeof credit.ownerId === "string")).toBe(true);
    }
  });

  it("records invalid guesses in failedGuesses without duplicates", () => {
    const players = [participant("Alice", 0)];
    const runtime = createPuzzleRuntime(crossword);

    const first = submitGuess(crossword, runtime, players, "Alice", "ploted");
    const second = submitGuess(crossword, runtime, players, "Alice", "PLOTED");

    expect(first.changed).toBe(false);
    expect(second.changed).toBe(false);
    expect(runtime.failedGuesses).toEqual(["ploted"]);
  });
});

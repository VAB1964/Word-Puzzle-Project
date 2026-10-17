import { readFileSync } from "node:fs";
import { describe, expect, it } from "vitest";
import { LIBRARY_RESTORATIONS, earnedRestorations, restorationForPuzzle } from "../../web/src/libraryRestoration";

describe("library restoration rewards", () => {
  it("ships nonempty, complete WebP files for every room and reward", () => {
    for (const room of LIBRARY_RESTORATIONS) {
      for (const file of [room.base, ...room.restorations.map(reward => reward.file)]) {
        const bytes = readFileSync(new URL(`../../assets/living-library/rooms/${file}`, import.meta.url));
        expect(bytes.length, file).toBeGreaterThan(12);
        expect(bytes.toString("ascii", 0, 4), file).toBe("RIFF");
        expect(bytes.toString("ascii", 8, 12), file).toBe("WEBP");
        expect(bytes.readUInt32LE(4) + 8, file).toBe(bytes.length);
      }
    }
  });
  it("assigns exactly one reward to each of the 60 puzzles", () => {
    expect(LIBRARY_RESTORATIONS).toHaveLength(6);
    const puzzles = LIBRARY_RESTORATIONS.flatMap(room => room.restorations.map(reward => reward.puzzle));
    expect(puzzles).toEqual(Array.from({ length: 60 }, (_, i) => i + 1));
    for (const room of LIBRARY_RESTORATIONS) expect(room.restorations).toHaveLength(10);
    expect(restorationForPuzzle(10)?.puzzle).toBe(10);
    expect(restorationForPuzzle(11)?.puzzle).toBe(11);
    expect(restorationForPuzzle(60)?.puzzle).toBe(60);
  });
  it("derives rewards from actual completions, not unlocks or replay counts", () => {
    expect(earnedRestorations(0, [])).toHaveLength(0);
    expect(earnedRestorations(0, [1, 1, 3, 11]).map(reward => reward.puzzle).sort()).toEqual([1, 3]);
    expect(earnedRestorations(1, [1, 3, 11]).map(reward => reward.puzzle)).toEqual([11]);
    expect(earnedRestorations(0, Array.from({ length: 60 }, (_, i) => i + 1))).toHaveLength(10);
  });
});

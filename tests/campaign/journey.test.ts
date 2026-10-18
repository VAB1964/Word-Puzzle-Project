import { describe, expect, it } from "vitest";
import {
  journeyLayout,
  LIBRARY_ROOMS,
  nextRoomPuzzle,
  roomPuzzleButtonLabel
} from "../../web/src/campaignJourney";
import type { Rect } from "../../web/src/core/types";

const overlaps = (a: Rect, b: Rect) => a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height && a.y + a.height > b.y;

describe("library journey layout", () => {
  it.each([[600, 1230], [720, 1100], [1000, 800]])("keeps map and modal targets separate at %sx%s", (width, height) => {
    const layout = journeyLayout(width, height);
    expect(layout.rooms).toHaveLength(6);
    expect(layout.puzzles).toHaveLength(10);
    for (const group of [[...layout.rooms, layout.back, layout.newCampaign], [...layout.puzzles, layout.previous, layout.close, layout.next]]) {
      for (const [i, rect] of group.entries()) {
        expect(rect.x).toBeGreaterThanOrEqual(0);
        expect(rect.y).toBeGreaterThanOrEqual(0);
        expect(rect.x + rect.width).toBeLessThanOrEqual(width);
        expect(rect.y + rect.height).toBeLessThanOrEqual(height);
        expect(rect.height).toBeGreaterThan(40);
        for (const other of group.slice(i + 1)) expect(overlaps(rect, other)).toBe(false);
      }
    }
    // Leave room for the campaign progress text above the footer.
    expect(layout.rooms[0].y + layout.rooms[0].height).toBeLessThan(height - 125);
  });
  it("names all six destinations in ascending order", () => {
    expect(LIBRARY_ROOMS).toHaveLength(6);
    expect(LIBRARY_ROOMS[0].name).toBe("The Front Hall");
    expect(LIBRARY_ROOMS[5].name).toBe("The Observatory");
  });
  it("offers the first or next unfinished puzzle from a room", () => {
    expect(nextRoomPuzzle(0, [])).toBe(1);
    expect(roomPuzzleButtonLabel(0, [])).toBe("First Puzzle");
    expect(nextRoomPuzzle(0, [1, 2, 3])).toBe(4);
    expect(roomPuzzleButtonLabel(0, [1, 2, 3])).toBe("Next Puzzle");
    const completedRoom = Array.from({ length: 10 }, (_, index) => index + 11);
    expect(nextRoomPuzzle(1, completedRoom)).toBe(11);
    expect(roomPuzzleButtonLabel(1, completedRoom)).toBe("First Puzzle");
  });
});

import { describe, expect, it } from "vitest";
import {
  LACHLAN_WORDS,
  buildAcceptedWords,
  clampLachlanProgress
} from "../web/src/lachlan/data";

describe("Lachlan spelling list", () => {
  it("contains 67 unique multi-letter targets in the requested order", () => {
    expect(LACHLAN_WORDS).toHaveLength(67);
    expect(LACHLAN_WORDS[0]?.word).toBe("the");
    expect(LACHLAN_WORDS.at(-1)?.word).toBe("today");
    expect(new Set(LACHLAN_WORDS.map(({ word }) => word)).size).toBe(67);
    expect(LACHLAN_WORDS.every(({ word }) => word.length >= 2)).toBe(true);
  });

  it("provides a sentence containing every assigned spelling word", () => {
    for (const { word, sentence } of LACHLAN_WORDS) {
      expect(sentence.toLowerCase().split(/[^a-z]+/)).toContain(word);
    }
  });

  it("accepts the target and valid bonus words but rejects impossible words", () => {
    const accepted = buildAcceptedWords("there", ["the", "her", "tree", "three", "here", "treee", "cheer"]);
    expect(accepted.has("there")).toBe(true);
    expect(accepted.has("the")).toBe(true);
    expect(accepted.has("three")).toBe(true);
    expect(accepted.has("here")).toBe(true);
    expect(accepted.has("tree")).toBe(true);
    expect(accepted.has("treee")).toBe(false);
    expect(accepted.has("cheer")).toBe(false);
  });

  it("allows two-letter spelling targets", () => {
    const accepted = buildAcceptedWords("be", ["be", "bee", "b", "he"]);
    expect([...accepted]).toEqual(["be"]);
  });

  it("clamps saved progress to the current list", () => {
    expect(clampLachlanProgress("12")).toBe(12);
    expect(clampLachlanProgress("-1")).toBe(0);
    expect(clampLachlanProgress("not-a-number")).toBe(0);
    expect(clampLachlanProgress(999)).toBe(67);
  });
});

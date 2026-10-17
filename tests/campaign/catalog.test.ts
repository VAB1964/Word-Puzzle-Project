import { describe, expect, it } from "vitest";
import {
  CAMPAIGN_LEVELS,
  CAMPAIGN_VARIANT_COUNT,
  campaignLevels,
  campaignTileScaleLimit,
  campaignWheelLetterMaxRadiusFactor,
  campaignWheelLetterScale,
  campaignVariantIndex,
  createCampaignSeed
} from "../../web/src/campaign";
import { GameMode } from "../../web/src/core/types";
import type { WordInfo } from "../../web/src/core/types";
import { generateCrossword } from "../../web/src/data/crossword";

const canBuild = (word: string, rack: string) => {
  const counts = new Map<string, number>();
  for (const letter of rack) counts.set(letter, (counts.get(letter) ?? 0) + 1);
  for (const letter of word) {
    const remaining = counts.get(letter) ?? 0;
    if (remaining === 0) return false;
    counts.set(letter, remaining - 1);
  }
  return true;
};

const connectedBySharedLetters = (words: string[]) => {
  if (words.length < 2) return false;
  const reached = new Set([0]);
  while (reached.size < words.length) {
    let changed = false;
    for (let index = 0; index < words.length; index += 1) {
      if (reached.has(index)) continue;
      if (
        [...reached].some((other) =>
          [...words[index]].some((letter) => words[other].includes(letter))
        )
      ) {
        reached.add(index);
        changed = true;
      }
    }
    if (!changed) return false;
  }
  return true;
};

const allCampaigns = Array.from(
  { length: CAMPAIGN_VARIANT_COUNT },
  (_, index) => campaignLevels(index)
);

describe("mobile campaign catalog", () => {
  it("scales sparse and short puzzles more prominently", () => {
    expect(campaignTileScaleLimit(3, 1)).toBeGreaterThan(
      campaignTileScaleLimit(7, 9)
    );
    expect(campaignWheelLetterScale(3)).toBeGreaterThan(
      campaignWheelLetterScale(7)
    );
    expect(campaignWheelLetterMaxRadiusFactor(3)).toBeGreaterThan(
      campaignWheelLetterMaxRadiusFactor(7)
    );
  });

  it("contains sequential levels and the intended rack-length stages", () => {
    expect(CAMPAIGN_VARIANT_COUNT).toBeGreaterThan(1);
    for (const levels of allCampaigns) {
      expect(levels).toHaveLength(70);
      expect(levels.map((level) => level.level)).toEqual(
        Array.from({ length: 70 }, (_, index) => index + 1)
      );
      expect(levels.slice(0, 5).every((level) => level.rackLength === 3)).toBe(true);
      expect(levels.slice(5, 15).every((level) => level.rackLength === 4)).toBe(true);
      expect(levels.slice(15, 30).every((level) => level.rackLength === 5)).toBe(true);
      expect(levels.slice(30, 50).every((level) => level.rackLength === 6)).toBe(true);
      expect(levels.slice(50).every((level) => level.rackLength === 7)).toBe(true);
    }
  });

  it("uses unique racks and constructible required words", () => {
    for (const levels of allCampaigns) {
      const letterSets = new Set<string>();
      for (const level of levels) {
        const letters = [...level.rackWord].sort().join("");
        expect(letterSets.has(letters)).toBe(false);
        letterSets.add(letters);
        expect(level.requiredWords.length).toBeGreaterThan(0);
        for (const word of level.requiredWords) {
          expect(canBuild(word, level.rackWord)).toBe(true);
          expect(word.length).toBeGreaterThanOrEqual(3);
          expect(word.length).toBeLessThanOrEqual(level.maximumRequiredLength);
        }
      }
    }
  });

  it("only marks letter-connected word sets as crosswords", () => {
    const crosswords = allCampaigns
      .flat()
      .filter((level) => level.mode === GameMode.Crossword);
    expect(crosswords.length).toBeGreaterThan(0);
    for (const level of crosswords) {
      expect(connectedBySharedLetters(level.requiredWords)).toBe(true);
    }
  });

  it("places every required word in each crossword level", () => {
    const asWordInfo = (text: string): WordInfo => ({
      text,
      rarity: 1,
      pos: "",
      definition: "",
      sentence: "",
      avgSubLen: 0,
      countGE3: 0,
      countGE4: 0,
      countGE5: 0,
      easyValidCount: 0,
      mediumValidCount: 0,
      hardValidCount: 0
    });
    const failures: string[] = [];
    for (const [campaignIndex, levels] of allCampaigns.entries()) {
      for (const level of levels.filter(
        (entry) => entry.mode === GameMode.Crossword
      )) {
        const result = generateCrossword(
          level.requiredWords.map(asWordInfo),
          level.level
        );
        if (result.placedWords.length !== level.requiredWords.length) {
          failures.push(`campaign ${campaignIndex + 1}, level ${level.level}`);
        }
      }
    }
    expect(failures).toEqual([]);
  });

  it("creates a reproducible new seed for a different campaign variant", () => {
    const seed = createCampaignSeed(0, 16);
    expect(campaignVariantIndex(seed)).not.toBe(campaignVariantIndex(0));
    expect(createCampaignSeed(0, 16)).toBe(seed);
    expect(campaignLevels(seed)).toBe(campaignLevels(seed));
  });

  it("keeps campaign crossword layouts stable for a level seed", () => {
    const level = CAMPAIGN_LEVELS.find((entry) => entry.mode === GameMode.Crossword)!;
    const words = level.requiredWords.map((text) => ({
      text,
      rarity: 1,
      pos: "",
      definition: "",
      sentence: "",
      avgSubLen: 0,
      countGE3: 0,
      countGE4: 0,
      countGE5: 0,
      easyValidCount: 0,
      mediumValidCount: 0,
      hardValidCount: 0
    }));
    const first = generateCrossword(words, level.level);
    const second = generateCrossword(words, level.level);
    expect(second.placements).toEqual(first.placements);
    expect(second.placedWords.map((word) => word.text)).toEqual(
      first.placedWords.map((word) => word.text)
    );
  });
});

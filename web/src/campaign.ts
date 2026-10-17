import catalogJson from "./data/campaign-levels.json";
import { GameMode } from "./core/types";

export interface CampaignLevel {
  level: number;
  mode: GameMode;
  rackWord: string;
  requiredWords: string[];
  rackLength: number;
  minimumRequiredLength: number;
  maximumRequiredLength: number;
  maximumRarity: number;
}

interface CampaignCatalog {
  version: number;
  levels: CampaignLevel[];
  campaigns?: Array<{
    id: number;
    seed: number;
    levels: CampaignLevel[];
  }>;
}

export interface CampaignProgress {
  version: number;
  currentLevel: number;
  highestUnlockedLevel: number;
  completedLevels: number[];
  totalScore: number;
  hintPoints: number;
  tutorialCompleted: boolean;
  campaignSeed: number;
  campaignNumber: number;
}

const STORAGE_KEY = "wordpuzzle.campaign.v1";
const catalog = catalogJson as CampaignCatalog;
const campaignVariants =
  catalog.campaigns?.map((campaign) => campaign.levels) ?? [catalog.levels];

export const CAMPAIGN_LEVELS = catalog.levels;
export const CAMPAIGN_VERSION = catalog.version;
export const CAMPAIGN_VARIANT_COUNT = campaignVariants.length;
export const CAMPAIGN_PUZZLES_PER_LEVEL = 10;

export const campaignGroup = (puzzleNumber: number) =>
  Math.floor((Math.max(1, puzzleNumber) - 1) / CAMPAIGN_PUZZLES_PER_LEVEL) + 1;

export const campaignPuzzleInGroup = (puzzleNumber: number) =>
  ((Math.max(1, puzzleNumber) - 1) % CAMPAIGN_PUZZLES_PER_LEVEL) + 1;

export const campaignTileScaleLimit = (rackLength: number, wordCount: number) => {
  const rackAdjustment = Math.max(0, rackLength - 3) * 0.1;
  const densityAdjustment = Math.max(0, wordCount - 1) * 0.025;
  return Math.max(1.35, Math.min(2.6, 2.6 - rackAdjustment - densityAdjustment));
};

export const campaignWheelLetterScale = (rackLength: number) =>
  Math.max(1.05, 1.7 - Math.max(0, rackLength - 3) * 0.14);

export const campaignWheelLetterMaxRadiusFactor = (rackLength: number) =>
  Math.max(0.2, 0.3 - Math.max(0, rackLength - 3) * 0.025);

export const campaignVariantIndex = (seed: number) => {
  if (campaignVariants.length <= 1 || seed === 0) return 0;
  return (Math.trunc(seed) >>> 0) % campaignVariants.length;
};

export const campaignLevels = (seed = 0) =>
  campaignVariants[campaignVariantIndex(seed)] ?? CAMPAIGN_LEVELS;

export const createCampaignSeed = (currentSeed: number, entropy = Date.now()) => {
  if (campaignVariants.length <= 1) return 0;
  const currentVariant = campaignVariantIndex(currentSeed);
  let candidate = (Math.trunc(entropy) >>> 0) || 1;
  while (campaignVariantIndex(candidate) === currentVariant) {
    candidate = (candidate + 1) >>> 0;
    if (candidate === 0) candidate = 1;
  }
  return candidate;
};

export const freshCampaignProgress = (
  campaignSeed = 0,
  campaignNumber = 1
): CampaignProgress => ({
  version: CAMPAIGN_VERSION,
  currentLevel: 1,
  highestUnlockedLevel: 1,
  completedLevels: [],
  totalScore: 0,
  hintPoints: 3,
  tutorialCompleted: false,
  campaignSeed,
  campaignNumber
});

export const loadCampaignProgress = (): CampaignProgress => {
  try {
    const parsed = JSON.parse(localStorage.getItem(STORAGE_KEY) ?? "") as Partial<CampaignProgress>;
    if (parsed.version !== CAMPAIGN_VERSION) return freshCampaignProgress();
    const completedLevels = Array.isArray(parsed.completedLevels)
      ? parsed.completedLevels.filter((level) => Number.isInteger(level) && level > 0)
      : [];
    const highestUnlockedLevel = Math.min(
      CAMPAIGN_LEVELS.length,
      Math.max(1, Number(parsed.highestUnlockedLevel) || 1)
    );
    return {
      version: CAMPAIGN_VERSION,
      currentLevel: Math.min(
        highestUnlockedLevel,
        Math.max(1, Number(parsed.currentLevel) || 1)
      ),
      highestUnlockedLevel,
      completedLevels,
      totalScore: Math.max(0, Number(parsed.totalScore) || 0),
      hintPoints: Math.max(0, Number(parsed.hintPoints) || 0),
      tutorialCompleted: Boolean(parsed.tutorialCompleted),
      campaignSeed: Math.max(0, Math.trunc(Number(parsed.campaignSeed) || 0)),
      campaignNumber: Math.max(1, Math.trunc(Number(parsed.campaignNumber) || 1))
    };
  } catch {
    return freshCampaignProgress();
  }
};

export const saveCampaignProgress = (progress: CampaignProgress) => {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(progress));
  } catch {
    // The game remains playable if WebView storage is unavailable.
  }
};

export const campaignLevel = (levelNumber: number, seed = 0) => {
  const levels = campaignLevels(seed);
  return levels.find((level) => level.level === levelNumber) ?? levels[0];
};

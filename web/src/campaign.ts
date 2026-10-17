import catalogJson from "./data/campaign-levels.json";
import { GameMode } from "./core/types";

export interface CampaignLevel {
  level: number;
  mode: GameMode;
  rackWord: string;
  requiredWords: string[];
  rackLength: number;
  maximumRequiredLength: number;
  maximumRarity: number;
}

interface CampaignCatalog {
  version: number;
  levels: CampaignLevel[];
}

export interface CampaignProgress {
  version: number;
  currentLevel: number;
  highestUnlockedLevel: number;
  completedLevels: number[];
  totalScore: number;
  hintPoints: number;
  tutorialCompleted: boolean;
}

const STORAGE_KEY = "wordpuzzle.campaign.v1";
const catalog = catalogJson as CampaignCatalog;

export const CAMPAIGN_LEVELS = catalog.levels;
export const CAMPAIGN_VERSION = catalog.version;

export const campaignTileScaleLimit = (rackLength: number, wordCount: number) => {
  const rackAdjustment = Math.max(0, rackLength - 3) * 0.1;
  const densityAdjustment = Math.max(0, wordCount - 1) * 0.025;
  return Math.max(1.35, Math.min(2.6, 2.6 - rackAdjustment - densityAdjustment));
};

export const campaignWheelLetterScale = (rackLength: number) =>
  Math.max(1.05, 1.7 - Math.max(0, rackLength - 3) * 0.14);

export const campaignWheelLetterMaxRadiusFactor = (rackLength: number) =>
  Math.max(0.2, 0.3 - Math.max(0, rackLength - 3) * 0.025);

const freshProgress = (): CampaignProgress => ({
  version: CAMPAIGN_VERSION,
  currentLevel: 1,
  highestUnlockedLevel: 1,
  completedLevels: [],
  totalScore: 0,
  hintPoints: 3,
  tutorialCompleted: false
});

export const loadCampaignProgress = (): CampaignProgress => {
  try {
    const parsed = JSON.parse(localStorage.getItem(STORAGE_KEY) ?? "") as Partial<CampaignProgress>;
    if (parsed.version !== CAMPAIGN_VERSION) return freshProgress();
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
      tutorialCompleted: Boolean(parsed.tutorialCompleted)
    };
  } catch {
    return freshProgress();
  }
};

export const saveCampaignProgress = (progress: CampaignProgress) => {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(progress));
  } catch {
    // The game remains playable if WebView storage is unavailable.
  }
};

export const campaignLevel = (levelNumber: number) =>
  CAMPAIGN_LEVELS.find((level) => level.level === levelNumber) ?? CAMPAIGN_LEVELS[0];

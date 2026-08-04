import type { TableTalkLevel } from "./types";
import type { TableTalkVoiceMode } from "./types";

const TABLE_TALK_KEY = "cribbage_tableTalkLevel";
const TABLE_TALK_VOICE_ENABLED_KEY = "cribbage_tableTalkVoiceEnabled";
const TABLE_TALK_VOICE_MODE_KEY = "cribbage_tableTalkVoiceMode";
const TABLE_TALK_CLOUD_USAGE_KEY = "cribbage_tableTalkCloudUsage";

const LEVELS: TableTalkLevel[] = ["off", "occasional", "chatty"];

export function loadTableTalkLevel(defaultLevel: TableTalkLevel = "occasional"): TableTalkLevel {
  try {
    const saved = localStorage.getItem(TABLE_TALK_KEY);
    if (!saved) return defaultLevel;
    return LEVELS.includes(saved as TableTalkLevel) ? (saved as TableTalkLevel) : defaultLevel;
  } catch {
    return defaultLevel;
  }
}

export function saveTableTalkLevel(level: TableTalkLevel) {
  try {
    localStorage.setItem(TABLE_TALK_KEY, level);
  } catch {
    // Ignore storage failures to avoid blocking gameplay.
  }
}

export function loadTableTalkVoiceEnabled(defaultEnabled = false): boolean {
  try {
    const saved = localStorage.getItem(TABLE_TALK_VOICE_ENABLED_KEY);
    if (saved === null) return defaultEnabled;
    return saved === "true";
  } catch {
    return defaultEnabled;
  }
}

export function saveTableTalkVoiceEnabled(enabled: boolean) {
  try {
    localStorage.setItem(TABLE_TALK_VOICE_ENABLED_KEY, String(enabled));
  } catch {
    // Ignore storage failures to avoid blocking gameplay.
  }
}

const VOICE_MODES: TableTalkVoiceMode[] = ["browser", "cloud"];

export function loadTableTalkVoiceMode(defaultMode: TableTalkVoiceMode = "browser"): TableTalkVoiceMode {
  try {
    const saved = localStorage.getItem(TABLE_TALK_VOICE_MODE_KEY);
    if (!saved) return defaultMode;
    return VOICE_MODES.includes(saved as TableTalkVoiceMode) ? (saved as TableTalkVoiceMode) : defaultMode;
  } catch {
    return defaultMode;
  }
}

export function saveTableTalkVoiceMode(mode: TableTalkVoiceMode) {
  try {
    localStorage.setItem(TABLE_TALK_VOICE_MODE_KEY, mode);
  } catch {
    // Ignore storage failures to avoid blocking gameplay.
  }
}

export type CloudVoiceUsageStats = {
  requestCount: number;
  charCount: number;
  estimatedUsd: number;
};

const EMPTY_USAGE: CloudVoiceUsageStats = {
  requestCount: 0,
  charCount: 0,
  estimatedUsd: 0,
};

export function loadCloudVoiceUsageStats(): CloudVoiceUsageStats {
  try {
    const raw = localStorage.getItem(TABLE_TALK_CLOUD_USAGE_KEY);
    if (!raw) return { ...EMPTY_USAGE };
    const parsed = JSON.parse(raw) as Partial<CloudVoiceUsageStats>;
    return {
      requestCount: Number.isFinite(parsed.requestCount) ? Math.max(0, parsed.requestCount ?? 0) : 0,
      charCount: Number.isFinite(parsed.charCount) ? Math.max(0, parsed.charCount ?? 0) : 0,
      estimatedUsd: Number.isFinite(parsed.estimatedUsd) ? Math.max(0, parsed.estimatedUsd ?? 0) : 0,
    };
  } catch {
    return { ...EMPTY_USAGE };
  }
}

export function saveCloudVoiceUsageStats(stats: CloudVoiceUsageStats) {
  try {
    localStorage.setItem(TABLE_TALK_CLOUD_USAGE_KEY, JSON.stringify(stats));
  } catch {
    // Ignore storage failures to avoid blocking gameplay.
  }
}

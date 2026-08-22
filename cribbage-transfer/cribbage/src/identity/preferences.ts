import type { TableTalkLevel } from "../tableTalk/types";

export type AvatarCategory = "male" | "female" | "generic";
export type Avatar = { id: string; label: string; glyph: string; category: AvatarCategory };

export const AVATARS: readonly Avatar[] = [
  { id: "m-1", label: "Gentleman with cap", glyph: "🧢", category: "male" },
  { id: "m-2", label: "Gentleman with glasses", glyph: "👨‍🏫", category: "male" },
  { id: "m-3", label: "Gentleman with beard", glyph: "🧔", category: "male" },
  { id: "m-4", label: "Gentleman with hat", glyph: "🤠", category: "male" },
  { id: "f-1", label: "Lady with curls", glyph: "👩‍🦱", category: "female" },
  { id: "f-2", label: "Lady with glasses", glyph: "👩‍🏫", category: "female" },
  { id: "f-3", label: "Lady with hat", glyph: "👒", category: "female" },
  { id: "f-4", label: "Lady with silver hair", glyph: "👵", category: "female" },
  { id: "g-1", label: "Gold club", glyph: "♣", category: "generic" },
  { id: "g-2", label: "Red heart", glyph: "♥", category: "generic" },
  { id: "g-3", label: "Blue spade", glyph: "♠", category: "generic" },
  { id: "g-4", label: "Ivory diamond", glyph: "♦", category: "generic" },
] as const;

export type PlayerPreferences = {
  displayName: string;
  avatarId: string;
  soundEnabled: boolean;
  volume: number;
  tableTalk: TableTalkLevel;
  voiceEnabled: boolean;
  reducedAnimation: boolean;
};

const STORAGE_KEY = "cribbage.player-preferences.v1";
export const DEFAULT_PREFERENCES: PlayerPreferences = {
  displayName: "",
  avatarId: AVATARS[0].id,
  soundEnabled: true,
  volume: 0.55,
  tableTalk: "occasional",
  voiceEnabled: false,
  reducedAnimation: false,
};

export function loadPreferences(): PlayerPreferences {
  try {
    const value = JSON.parse(localStorage.getItem(STORAGE_KEY) ?? "{}") as Partial<PlayerPreferences>;
    return {
      ...DEFAULT_PREFERENCES,
      ...value,
      avatarId: AVATARS.some(avatar => avatar.id === value.avatarId) ? value.avatarId! : DEFAULT_PREFERENCES.avatarId,
      volume: Math.max(0, Math.min(1, Number(value.volume ?? DEFAULT_PREFERENCES.volume))),
    };
  } catch {
    return DEFAULT_PREFERENCES;
  }
}

export function savePreferences(preferences: PlayerPreferences) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(preferences));
}

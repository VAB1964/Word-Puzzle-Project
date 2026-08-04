import type { CharacterId, TableTalkEmotion } from "./types";

export type PersonalityProfile = {
  id: CharacterId;
  displayName: string;
  primaryEmotions: TableTalkEmotion[];
  styleNotes: string[];
};

export const PERSONALITIES: Record<CharacterId, PersonalityProfile> = {
  mabel: {
    id: "mabel",
    displayName: "Mabel",
    primaryEmotions: ["supportive", "playful", "competitive"],
    styleNotes: [
      "Warm and encouraging with gentle teasing.",
      "Compliments good plays and keeps table spirits high.",
    ],
  },
  arthur: {
    id: "arthur",
    displayName: "Arthur",
    primaryEmotions: ["dry", "competitive", "self_deprecating"],
    styleNotes: [
      "Dry humor and understated delivery.",
      "Competitive but gracious, frequently jokes about bad luck.",
    ],
  },
  clara: {
    id: "clara",
    displayName: "Clara",
    primaryEmotions: ["optimistic", "supportive", "competitive"],
    styleNotes: [
      "Cheerful, positive, and friendly while staying concise.",
      "Celebrates strong plays without becoming repetitive.",
    ],
  },
};

const NAME_TO_CHARACTER: Record<string, CharacterId> = {
  mabel: "mabel",
  arthur: "arthur",
  clara: "clara",
};

export function characterIdFromName(name: string): CharacterId | null {
  return NAME_TO_CHARACTER[name.trim().toLowerCase()] ?? null;
}

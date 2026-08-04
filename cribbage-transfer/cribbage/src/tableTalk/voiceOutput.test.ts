import { describe, expect, it, vi } from "vitest";
import { TableTalkVoiceOutput } from "./voiceOutput";
import type { CharacterDialogueEmission } from "./types";

function emission(characterId: CharacterDialogueEmission["characterId"], characterName: string): CharacterDialogueEmission {
  return {
    characterId,
    characterName,
    text: "That was a lovely run!",
    eventType: "pegging_scored",
    emotion: "optimistic",
    timestamp: 10,
    dynamic: true,
    event: { type: "pegging_scored", actorIndex: 2, points: 3, kind: "pegging_run", runningTotal: 21 },
    context: {
      level: "chatty",
      playerCount: 2,
      dealerIndex: 1,
      runningCount: 21,
      scores: [
        { playerIndex: 0, team: 0, name: "You", score: 20 },
        { playerIndex: 1, team: 1, name: characterName, score: 22 },
      ],
      participants: [
        { playerIndex: 1, team: 1, name: characterName, color: "blue", characterId },
      ],
    },
  };
}

describe("TableTalkVoiceOutput", () => {
  it("does nothing when disabled", () => {
    const speak = vi.fn();
    const output = new TableTalkVoiceOutput({
      enabled: false,
      synthesis: {
        speaking: false,
        getVoices: () => [],
        speak,
        cancel: vi.fn(),
      },
      makeUtterance: text => ({ text } as SpeechSynthesisUtterance),
    });

    output.speak(emission("clara", "Clara"));
    expect(speak).not.toHaveBeenCalled();
  });

  it("speaks dialogue and applies character voice profile", () => {
    type InspectableUtterance = {
      text: string;
      rate: number;
      pitch: number;
      volume: number;
      voice: SpeechSynthesisVoice | null;
    };

    const speak = vi.fn();
    const output = new TableTalkVoiceOutput({
      enabled: true,
      synthesis: {
        speaking: false,
        getVoices: () => [
          { name: "Samantha", lang: "en-US" } as SpeechSynthesisVoice,
          { name: "Google UK English Male", lang: "en-GB" } as SpeechSynthesisVoice,
          { name: "Libby", lang: "en-US" } as SpeechSynthesisVoice,
        ],
        speak,
        cancel: vi.fn(),
      },
      makeUtterance: text => {
        const created = {
          text,
          rate: 1,
          pitch: 1,
          volume: 1,
          voice: null,
        };
        return created as unknown as SpeechSynthesisUtterance;
      },
    });

    output.setVolume(0.55);
    output.speak(emission("arthur", "Arthur"));

    expect(speak).toHaveBeenCalledTimes(1);
    const spoken = speak.mock.calls[0]?.[0] as unknown as InspectableUtterance;
    expect(spoken.text).toBe("That was a lovely run!");
    expect(spoken.volume).toBe(0.55);
    expect(spoken.rate).toBe(1.32);
    expect(spoken.pitch).toBe(0.97);
    expect(spoken.voice?.name).toBe("Google UK English Male");
  });

  it("picks different voices across characters when available", () => {
    const spoken: Array<{ voiceName: string | null; text: string }> = [];
    const output = new TableTalkVoiceOutput({
      enabled: true,
      synthesis: {
        speaking: false,
        getVoices: () => [
          { name: "Samantha", lang: "en-US" } as SpeechSynthesisVoice,
          { name: "Google UK English Male", lang: "en-GB" } as SpeechSynthesisVoice,
          { name: "Libby", lang: "en-US" } as SpeechSynthesisVoice,
        ],
        speak: utterance => {
          spoken.push({
            voiceName: (utterance.voice?.name ?? null),
            text: utterance.text,
          });
        },
        cancel: vi.fn(),
      },
      makeUtterance: text => ({ text, voice: null } as SpeechSynthesisUtterance),
    });

    output.speak(emission("mabel", "Mabel"));
    output.speak(emission("arthur", "Arthur"));
    output.speak(emission("clara", "Clara"));

    expect(spoken).toHaveLength(3);
    const voiceSet = new Set(spoken.map(item => item.voiceName));
    expect(voiceSet.size).toBeGreaterThan(1);
  });

  it("drops a new line while already speaking", () => {
    const cancel = vi.fn();
    const speak = vi.fn();
    const output = new TableTalkVoiceOutput({
      enabled: true,
      synthesis: {
        speaking: true,
        getVoices: () => [],
        speak,
        cancel,
      },
      makeUtterance: text => ({ text } as SpeechSynthesisUtterance),
    });

    output.speak(emission("mabel", "Mabel"));
    expect(cancel).toHaveBeenCalledTimes(0);
    expect(speak).toHaveBeenCalledTimes(0);
  });
});

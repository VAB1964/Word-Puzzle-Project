import type { CharacterDialogueEmission } from "./types";

type CharacterVoiceProfile = {
  rate: number;
  pitch: number;
  preferredHints: string[];
};

type SpeechSynthesisLike = {
  speaking: boolean;
  getVoices: () => SpeechSynthesisVoice[];
  speak: (utterance: SpeechSynthesisUtterance) => void;
  cancel: () => void;
};

type UtteranceFactory = (text: string) => SpeechSynthesisUtterance;

const VOICE_PROFILE: Record<CharacterDialogueEmission["characterId"], CharacterVoiceProfile> = {
  mabel: { rate: 1.18, pitch: 1.08, preferredHints: ["samantha", "zira", "jenny", "aria", "female"] },
  arthur: { rate: 1.32, pitch: 0.97, preferredHints: ["david", "guy", "george", "male"] },
  clara: { rate: 1.22, pitch: 1.14, preferredHints: ["ava", "libby", "susan", "female"] },
};

function defaultUtteranceFactory(text: string): SpeechSynthesisUtterance {
  return new SpeechSynthesisUtterance(text);
}

export class TableTalkVoiceOutput {
  private enabled: boolean;
  private volume = 0.8;
  private readonly synthesis: SpeechSynthesisLike | null;
  private readonly makeUtterance: UtteranceFactory;
  private finishActiveSpeech: (() => void) | null = null;

  constructor(options?: {
    enabled?: boolean;
    synthesis?: SpeechSynthesisLike | null;
    makeUtterance?: UtteranceFactory;
  }) {
    this.enabled = options?.enabled ?? false;
    this.synthesis = options?.synthesis ?? (typeof window !== "undefined" ? window.speechSynthesis : null);
    this.makeUtterance = options?.makeUtterance ?? defaultUtteranceFactory;
  }

  setEnabled(enabled: boolean) {
    this.enabled = enabled;
    if (!enabled) this.cancel();
  }

  setVolume(volume: number) {
    this.volume = Math.max(0, Math.min(1, volume));
  }

  speak(line: CharacterDialogueEmission, onPlaybackStart?: () => void): Promise<boolean> {
    if (!this.enabled || !this.synthesis || this.synthesis.speaking) return Promise.resolve(false);
    const synthesis = this.synthesis;
    return new Promise(resolve => {
      const utterance = this.makeUtterance(line.text);
      const profile = VOICE_PROFILE[line.characterId];
      utterance.rate = profile.rate;
      utterance.pitch = profile.pitch;
      utterance.volume = this.volume;
      utterance.voice = this.pickVoice(line.characterId);
      const finish = () => {
        if (this.finishActiveSpeech === finish) this.finishActiveSpeech = null;
        resolve(true);
      };
      this.finishActiveSpeech = finish;
      utterance.onend = finish;
      utterance.onerror = finish;
      onPlaybackStart?.();
      synthesis.speak(utterance);
    });
  }

  cancel() {
    if (!this.synthesis) return;
    this.synthesis.cancel();
    this.finishActiveSpeech?.();
  }

  private pickVoice(characterId: CharacterDialogueEmission["characterId"]): SpeechSynthesisVoice | null {
    if (!this.synthesis) return null;
    const voices = this.synthesis.getVoices();
    if (!voices.length) return null;

    const englishVoices = voices.filter(voice => voice.lang.toLowerCase().startsWith("en"));
    const candidates = englishVoices.length ? englishVoices : voices;
    const profile = VOICE_PROFILE[characterId];

    const byHint = profile.preferredHints
      .map(hint => candidates.find(voice => voice.name.toLowerCase().includes(hint)))
      .find(Boolean);
    if (byHint) return byHint ?? null;

    const bucket = [...candidates].sort((a, b) => a.name.localeCompare(b.name));
    if (!bucket.length) return null;
    if (bucket.length === 1) return bucket[0];

    if (characterId === "mabel") return bucket[0];
    if (characterId === "arthur") return bucket[Math.floor(bucket.length / 2)];
    return bucket[bucket.length - 1];
  }
}

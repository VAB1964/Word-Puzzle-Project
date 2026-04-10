const WORD_FOUND_TEMPLATES = [
  "Nice one! WORD is a great find!",
  "You got WORD!",
  "WORD, impressive vocabulary!",
  "WORD, well spotted!",
  "Great catch, WORD!",
  "WORD, that's a good one!",
  "Look at that, WORD!",
  "WORD, nicely done!",
  "Bravo! WORD!",
  "WORD, you're on fire!",
  "Excellent, WORD!",
  "WORD, keep it up!",
  "WORD, what a word!",
  "You found WORD, amazing!",
  "There's WORD, wonderful!",
];

const RARE_WORD_FOUND_TEMPLATES = [
  "Wow, WORD! That's a rare one!",
  "WORD! Impressive, that's uncommon!",
  "WORD, what a find! Very rare!",
  "WORD! Not many would get that!",
  "Outstanding! WORD is a tough one!",
  "WORD! You really know your words!",
  "Incredible, WORD! That's a gem!",
  "WORD, brilliant! A rare discovery!",
];

const PUZZLE_SOLVED_TEMPLATES = [
  "Puzzle complete! You nailed it!",
  "All words found, well done!",
  "Puzzle solved! Fantastic work!",
  "You cleared the whole puzzle!",
  "Every word found, amazing!",
  "Puzzle finished! Brilliant!",
  "That's a wrap! Great job!",
  "All done! You're a natural!",
  "Perfect puzzle clearance!",
  "Solved it! Nothing gets past you!",
];

const SESSION_COMPLETE_TEMPLATES = [
  "Session finished! You're a word master!",
  "What a performance, every puzzle solved!",
  "Session complete! Absolutely phenomenal!",
  "You conquered every puzzle! Incredible!",
  "All puzzles done! You're unstoppable!",
  "Session cleared! Take a bow!",
  "That was brilliant, session complete!",
  "Every puzzle beaten! What a champion!",
  "Full session victory! Outstanding!",
  "You did it! All puzzles mastered!",
];

function shuffleArray<T>(arr: T[]): T[] {
  const shuffled = [...arr];
  for (let i = shuffled.length - 1; i > 0; i--) {
    const j = Math.floor(Math.random() * (i + 1));
    [shuffled[i], shuffled[j]] = [shuffled[j], shuffled[i]];
  }
  return shuffled;
}

class TemplatePool {
  private templates: string[];
  private queue: string[] = [];

  constructor(templates: string[]) {
    this.templates = templates;
    this.refill();
  }

  private refill() {
    this.queue = shuffleArray(this.templates);
  }

  next(replacements?: Record<string, string>): string {
    if (this.queue.length === 0) this.refill();
    let text = this.queue.pop()!;
    if (replacements) {
      for (const [key, value] of Object.entries(replacements)) {
        text = text.replaceAll(key, value);
      }
    }
    return text;
  }
}

const STORAGE_KEY_VOICE_ENABLED = "wordPuzzle_voiceEnabled";

export class VoiceCommentary {
  private enabled: boolean;
  private synth: SpeechSynthesis | null;
  private wordPool: TemplatePool;
  private rareWordPool: TemplatePool;
  private puzzlePool: TemplatePool;
  private sessionPool: TemplatePool;
  private selectedVoice: SpeechSynthesisVoice | null = null;

  constructor() {
    this.synth = typeof window !== "undefined" && window.speechSynthesis ? window.speechSynthesis : null;
    this.enabled = this.loadEnabledState();
    this.wordPool = new TemplatePool(WORD_FOUND_TEMPLATES);
    this.rareWordPool = new TemplatePool(RARE_WORD_FOUND_TEMPLATES);
    this.puzzlePool = new TemplatePool(PUZZLE_SOLVED_TEMPLATES);
    this.sessionPool = new TemplatePool(SESSION_COMPLETE_TEMPLATES);

    if (this.synth) {
      const pickVoice = () => {
        const voices = this.synth!.getVoices();
        if (voices.length === 0) return;
        this.selectedVoice =
          voices.find((v) => v.lang.startsWith("en") && v.name.toLowerCase().includes("female")) ??
          voices.find((v) => v.lang.startsWith("en")) ??
          voices[0];
      };
      pickVoice();
      if (this.synth.onvoiceschanged !== undefined) {
        this.synth.onvoiceschanged = pickVoice;
      }
    }
  }

  private loadEnabledState(): boolean {
    try {
      const stored = localStorage.getItem(STORAGE_KEY_VOICE_ENABLED);
      return stored === null ? true : stored === "true";
    } catch {
      return true;
    }
  }

  private saveEnabledState() {
    try {
      localStorage.setItem(STORAGE_KEY_VOICE_ENABLED, String(this.enabled));
    } catch { /* localStorage unavailable */ }
  }

  get isEnabled(): boolean {
    return this.enabled;
  }

  toggle(): boolean {
    this.enabled = !this.enabled;
    this.saveEnabledState();
    if (!this.enabled && this.synth) {
      this.synth.cancel();
    }
    return this.enabled;
  }

  private speak(text: string) {
    if (!this.enabled || !this.synth) return;
    this.synth.cancel();
    const utterance = new SpeechSynthesisUtterance(text);
    if (this.selectedVoice) utterance.voice = this.selectedVoice;
    utterance.rate = 1.05;
    utterance.pitch = 1.1;
    this.synth.speak(utterance);
  }

  onWordFound(word: string, rarity: number) {
    const displayWord = word.charAt(0).toUpperCase() + word.slice(1).toLowerCase();
    const pool = rarity > 1 ? this.rareWordPool : this.wordPool;
    this.speak(pool.next({ WORD: displayWord }));
  }

  onPuzzleSolved() {
    this.speak(this.puzzlePool.next());
  }

  onSessionComplete() {
    this.speak(this.sessionPool.next());
  }
}

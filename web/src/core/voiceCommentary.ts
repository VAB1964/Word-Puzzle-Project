import { Assets } from "../assets";

// ---------------------------------------------------------------------------
// Template text pools (used by SpeechSynthesis fallback & for reference)
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Shuffle / pool utilities
// ---------------------------------------------------------------------------

function shuffleArray<T>(arr: T[]): T[] {
  const shuffled = [...arr];
  for (let i = shuffled.length - 1; i > 0; i--) {
    const j = Math.floor(Math.random() * (i + 1));
    [shuffled[i], shuffled[j]] = [shuffled[j], shuffled[i]];
  }
  return shuffled;
}

class TemplatePool<T> {
  private items: T[];
  private queue: T[] = [];

  constructor(items: T[]) {
    this.items = items;
    this.refill();
  }

  private refill() {
    this.queue = shuffleArray(this.items);
  }

  next(): T {
    if (this.queue.length === 0) this.refill();
    return this.queue.pop()!;
  }

  nextWithReplacements(replacements: Record<string, string>): string {
    let text = this.next() as unknown as string;
    for (const [key, value] of Object.entries(replacements)) {
      text = text.split(key).join(value);
    }
    return text;
  }
}

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

const STORAGE_KEY_VOICE_ENABLED = "wordPuzzle_voiceEnabled";
const STORAGE_KEY_API_KEY = "wordPuzzle_openaiApiKey";
const AI_TIMEOUT_MS = 4000;
const AI_WORD_BUDGET = 4;
const AI_GENERATION_ENABLED = false;
const MAX_WORD_COMMENTS_PER_PUZZLE = 4;

const COMMENTATOR_SYSTEM_PROMPT = `You are a witty, warm word puzzle game commentator. Rules:
1. One sentence only, under 15 words.
2. Vary your style — sometimes enthusiastic, sometimes impressed, sometimes playful or witty.
3. Occasionally comment on the word's meaning, origin, or cleverly use it in a sentence.
4. For puzzle-solved and session-complete events, celebrate the achievement.
5. Never start two consecutive comments the same way.
6. Do not use hashtags or emojis.`;

const TTS_VOICE_INSTRUCTIONS =
  "Speak with enthusiasm and warmth, like a friendly game show host. Keep energy high but natural. Vary your pacing for emphasis.";

type VoiceMode = "ai" | "prebaked" | "synthesis";

// ---------------------------------------------------------------------------
// VoiceCommentary class
// ---------------------------------------------------------------------------

export interface WordInfo {
  text: string;
  rarity: number;
}

export class VoiceCommentary {
  private enabled: boolean;
  private apiKey: string | null;

  // Tier 3: SpeechSynthesis fallback
  private synth: SpeechSynthesis | null;
  private selectedVoice: SpeechSynthesisVoice | null = null;
  private wordPool: TemplatePool<string>;
  private rareWordPool: TemplatePool<string>;
  private puzzlePool: TemplatePool<string>;
  private sessionPool: TemplatePool<string>;

  // Tier 2: Pre-baked audio clips
  private preBakedWordPool: TemplatePool<string> | null = null;
  private preBakedRarePool: TemplatePool<string> | null = null;
  private preBakedPuzzlePool: TemplatePool<string> | null = null;
  private preBakedSessionPool: TemplatePool<string> | null = null;
  private preBakedReady = false;

  // Tier 1: AI dynamic commentary
  private currentAudio: HTMLAudioElement | null = null;
  private lastAIComment = "";

  // Pre-warm cache: generated at puzzle start, keyed by lowercase word
  private audioCache = new Map<string, Blob>();
  private _cacheTotal = 0;
  private _cacheReady = 0;
  private _cacheActive = false;
  private prewarmController: AbortController | null = null;

  // Throttle: limits how many words get commentary per puzzle
  private wordCommentsThisPuzzle = 0;

  constructor() {
    this.synth =
      typeof window !== "undefined" && window.speechSynthesis
        ? window.speechSynthesis
        : null;
    this.enabled = this.loadSetting(STORAGE_KEY_VOICE_ENABLED, true);
    this.apiKey = this.loadApiKey();

    this.wordPool = new TemplatePool(WORD_FOUND_TEMPLATES);
    this.rareWordPool = new TemplatePool(RARE_WORD_FOUND_TEMPLATES);
    this.puzzlePool = new TemplatePool(PUZZLE_SOLVED_TEMPLATES);
    this.sessionPool = new TemplatePool(SESSION_COMPLETE_TEMPLATES);

    this.initSynthVoice();
    this.initPreBakedPools();
  }

  // -------------------------------------------------------------------------
  // Public API (unchanged interface -- game.ts needs no modifications)
  // -------------------------------------------------------------------------

  get isEnabled(): boolean {
    return this.enabled;
  }

  get activeMode(): VoiceMode {
    if (this.apiKey) return "ai";
    if (this.preBakedReady) return "prebaked";
    return "synthesis";
  }

  get hasApiKey(): boolean {
    return !!this.apiKey;
  }

  toggle(): boolean {
    this.enabled = !this.enabled;
    this.saveSetting(STORAGE_KEY_VOICE_ENABLED, this.enabled);
    if (!this.enabled) this.stopAll();
    return this.enabled;
  }

  get cacheProgress(): number {
    return this._cacheTotal > 0 ? this._cacheReady / this._cacheTotal : 1;
  }

  get isCaching(): boolean {
    return this._cacheActive;
  }

  setApiKey(key: string | null) {
    this.apiKey = key && key.trim().length > 0 ? key.trim() : null;
    if (this.apiKey) {
      this.saveSetting(STORAGE_KEY_API_KEY, this.apiKey);
    } else {
      try { localStorage.removeItem(STORAGE_KEY_API_KEY); } catch { /* */ }
    }
  }

  /**
   * Call at puzzle start with the full word list. Picks up to AI_WORD_BUDGET
   * words (preferring longer/rarer ones) for AI commentary. The rest fall back
   * to pre-baked clips. Also pre-generates puzzle-solved / session-complete.
   */
  prewarmForPuzzle(words: WordInfo[]) {
    if (this.prewarmController) this.prewarmController.abort();
    this.prewarmController = new AbortController();

    this.audioCache.clear();
    this._cacheReady = 0;
    this._cacheActive = false;
    this.wordCommentsThisPuzzle = 0;

    if (!AI_GENERATION_ENABLED || !this.apiKey || !this.enabled || words.length === 0) {
      this._cacheTotal = 0;
      return;
    }

    const selected = this.pickWordsForAI(words);

    // +2 for puzzle_solved and session_complete event clips
    this._cacheTotal = selected.length + 2;
    this._cacheActive = true;

    const signal = this.prewarmController.signal;

    this.runPrewarm(selected, signal).catch(() => {}).finally(() => {
      this._cacheActive = false;
    });
  }

  private pickWordsForAI(words: WordInfo[]): WordInfo[] {
    if (words.length <= AI_WORD_BUDGET) return [...words];

    const ranked = [...words].sort((a, b) => {
      if (b.rarity !== a.rarity) return b.rarity - a.rarity;
      return b.text.length - a.text.length;
    });

    return ranked.slice(0, AI_WORD_BUDGET);
  }

  onWordFound(word: string, rarity: number) {
    if (!this.enabled) return;

    if (this.wordCommentsThisPuzzle >= MAX_WORD_COMMENTS_PER_PUZZLE) return;
    this.wordCommentsThisPuzzle++;

    const displayWord =
      word.charAt(0).toUpperCase() + word.slice(1).toLowerCase();
    const isRare = rarity > 1;

    const cached = this.audioCache.get(word.toLowerCase());
    if (cached) {
      this.stopAll();
      this.playBlob(cached).catch(() => {});
      return;
    }

    const eventType = isRare ? "rare_word_found" : "word_found";
    this.speakCascade(
      eventType,
      `The player just found the word "${displayWord}" in a word puzzle. Rarity: ${isRare ? "rare" : "common"}.`,
      isRare ? this.preBakedRarePool : this.preBakedWordPool,
      () => {
        const pool = isRare ? this.rareWordPool : this.wordPool;
        return pool.nextWithReplacements({ WORD: displayWord });
      }
    );
  }

  onPuzzleSolved() {
    if (!this.enabled) return;

    const cached = this.audioCache.get("__puzzle_solved__");
    if (cached) {
      this.stopAll();
      this.playBlob(cached).catch(() => {});
      return;
    }

    this.speakCascade(
      "puzzle_solved",
      "The player just solved the entire puzzle, finding every hidden word!",
      this.preBakedPuzzlePool,
      () => this.puzzlePool.next()
    );
  }

  onSessionComplete() {
    if (!this.enabled) return;

    const cached = this.audioCache.get("__session_complete__");
    if (cached) {
      this.stopAll();
      this.playBlob(cached).catch(() => {});
      return;
    }

    this.speakCascade(
      "session_complete",
      "The player just completed the entire session, beating every puzzle! This is a major achievement.",
      this.preBakedSessionPool,
      () => this.sessionPool.next()
    );
  }

  // -------------------------------------------------------------------------
  // Tier 1: AI Dynamic Commentary (gpt-4o-mini text + gpt-4o-mini-tts speech)
  // -------------------------------------------------------------------------

  private async speakAI(
    eventType: string,
    contextMessage: string
  ): Promise<boolean> {
    if (!this.apiKey) return false;

    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), AI_TIMEOUT_MS);

    try {
      const commentText = await this.generateComment(
        contextMessage,
        controller.signal
      );
      const audioBlob = await this.generateSpeech(
        commentText,
        controller.signal
      );
      clearTimeout(timeout);

      this.lastAIComment = commentText;
      await this.playBlob(audioBlob);
      return true;
    } catch {
      clearTimeout(timeout);
      return false;
    }
  }

  private async generateComment(
    context: string,
    signal: AbortSignal
  ): Promise<string> {
    const avoidPrefix = this.lastAIComment
      ? `\nAvoid starting with "${this.lastAIComment.split(" ")[0]}".`
      : "";

    const res = await fetch("https://api.openai.com/v1/chat/completions", {
      method: "POST",
      headers: {
        Authorization: `Bearer ${this.apiKey}`,
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        model: "gpt-4o-mini",
        messages: [
          { role: "system", content: COMMENTATOR_SYSTEM_PROMPT + avoidPrefix },
          { role: "user", content: context },
        ],
        max_tokens: 50,
        temperature: 1.0,
      }),
      signal,
    });

    if (!res.ok) throw new Error(`Chat API ${res.status}`);
    const data = await res.json();
    return data.choices[0].message.content.trim();
  }

  private async generateSpeech(
    text: string,
    signal: AbortSignal
  ): Promise<Blob> {
    const res = await fetch("https://api.openai.com/v1/audio/speech", {
      method: "POST",
      headers: {
        Authorization: `Bearer ${this.apiKey}`,
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        model: "gpt-4o-mini-tts",
        input: text,
        voice: "nova",
        instructions: TTS_VOICE_INSTRUCTIONS,
        response_format: "mp3",
      }),
      signal,
    });

    if (!res.ok) throw new Error(`TTS API ${res.status}`);
    return res.blob();
  }

  // -------------------------------------------------------------------------
  // Pre-warm: batch generation at puzzle start
  // -------------------------------------------------------------------------

  private async runPrewarm(words: WordInfo[], signal: AbortSignal) {
    // Step 1: Single API call generates comments for ALL words + event clips
    const commentMap = await this.batchGenerateComments(words, signal);

    // Step 2: Generate TTS for each in parallel (limited concurrency)
    const ttsJobs: Array<{ key: string; text: string }> = [];

    for (const w of words) {
      const comment = commentMap.get(w.text.toLowerCase());
      if (comment) {
        ttsJobs.push({ key: w.text.toLowerCase(), text: comment });
      }
    }

    const puzzleComment = commentMap.get("__puzzle_solved__");
    if (puzzleComment) {
      ttsJobs.push({ key: "__puzzle_solved__", text: puzzleComment });
    }
    const sessionComment = commentMap.get("__session_complete__");
    if (sessionComment) {
      ttsJobs.push({ key: "__session_complete__", text: sessionComment });
    }

    const CONCURRENCY = 2;
    let idx = 0;
    const runNext = async (): Promise<void> => {
      while (idx < ttsJobs.length) {
        if (signal.aborted) return;
        const job = ttsJobs[idx++];
        try {
          const blob = await this.generateSpeech(job.text, signal);
          this.audioCache.set(job.key, blob);
        } catch { /* failed for this word, will fall back at playback */ }
        this._cacheReady++;
      }
    };

    const workers = Array.from({ length: CONCURRENCY }, () => runNext());
    await Promise.all(workers);
  }

  private async batchGenerateComments(
    words: WordInfo[],
    signal: AbortSignal
  ): Promise<Map<string, string>> {
    const wordList = words
      .map((w) => {
        const display = w.text.charAt(0).toUpperCase() + w.text.slice(1).toLowerCase();
        return `"${display}"${w.rarity > 1 ? " (rare)" : ""}`;
      })
      .join(", ");

    const prompt =
      `Generate brief game commentary for a word puzzle. The player needs to find these words: ${wordList}\n\n` +
      `Return a JSON object with:\n` +
      `- A key for each word (lowercase) with a short enthusiastic comment (1 sentence, under 15 words). ` +
      `Occasionally mention the word's meaning or use it cleverly in a sentence.\n` +
      `- A key "__puzzle_solved__" with a celebration comment for completing the puzzle.\n` +
      `- A key "__session_complete__" with a big celebration comment for finishing all puzzles.\n\n` +
      `Example format: {"castle": "Castle — a word fit for royalty, nice find!", "__puzzle_solved__": "Every word found, outstanding!", "__session_complete__": "What a champion, full session cleared!"}`;

    const res = await fetch("https://api.openai.com/v1/chat/completions", {
      method: "POST",
      headers: {
        Authorization: `Bearer ${this.apiKey}`,
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        model: "gpt-4o-mini",
        messages: [
          { role: "system", content: COMMENTATOR_SYSTEM_PROMPT },
          { role: "user", content: prompt },
        ],
        max_tokens: 600,
        temperature: 1.0,
        response_format: { type: "json_object" },
      }),
      signal,
    });

    if (!res.ok) throw new Error(`Batch chat API ${res.status}`);
    const data = await res.json();
    const raw = data.choices[0].message.content.trim();

    const result = new Map<string, string>();
    try {
      const parsed = JSON.parse(raw) as Record<string, string>;
      for (const [key, value] of Object.entries(parsed)) {
        if (typeof value === "string" && value.length > 0) {
          result.set(key.toLowerCase(), value);
        }
      }
    } catch { /* JSON parse failed, cache will be empty */ }

    return result;
  }

  // -------------------------------------------------------------------------
  // Tier 2: Pre-baked .wav clips
  // -------------------------------------------------------------------------

  private initPreBakedPools() {
    try {
      const v = Assets.voice;
      if (!v) return;

      const test = new Audio();
      test.src = v.wordFound[0];
      test.addEventListener(
        "canplaythrough",
        () => {
          this.preBakedWordPool = new TemplatePool(v.wordFound);
          this.preBakedRarePool = new TemplatePool(v.rareWord);
          this.preBakedPuzzlePool = new TemplatePool(v.puzzleSolved);
          this.preBakedSessionPool = new TemplatePool(v.sessionComplete);
          this.preBakedReady = true;
        },
        { once: true }
      );
      test.addEventListener("error", () => {
        this.preBakedReady = false;
      }, { once: true });
    } catch {
      this.preBakedReady = false;
    }
  }

  private speakPreBaked(pool: TemplatePool<string> | null): boolean {
    if (!pool) return false;
    try {
      const url = pool.next();
      this.stopAll();
      const audio = new Audio(url);
      this.currentAudio = audio;
      audio.play().catch(() => {});
      return true;
    } catch {
      return false;
    }
  }

  // -------------------------------------------------------------------------
  // Tier 3: SpeechSynthesis (existing, unchanged)
  // -------------------------------------------------------------------------

  private initSynthVoice() {
    if (!this.synth) return;
    const pickVoice = () => {
      const voices = this.synth!.getVoices();
      if (voices.length === 0) return;
      this.selectedVoice =
        voices.find(
          (v) =>
            v.lang.startsWith("en") &&
            v.name.toLowerCase().includes("female")
        ) ??
        voices.find((v) => v.lang.startsWith("en")) ??
        voices[0];
    };
    pickVoice();
    if (this.synth.onvoiceschanged !== undefined) {
      this.synth.onvoiceschanged = pickVoice;
    }
  }

  private speakSynthesis(text: string) {
    if (!this.synth) return;
    this.synth.cancel();
    const utterance = new SpeechSynthesisUtterance(text);
    if (this.selectedVoice) utterance.voice = this.selectedVoice;
    utterance.rate = 1.05;
    utterance.pitch = 1.1;
    this.synth.speak(utterance);
  }

  // -------------------------------------------------------------------------
  // Cascade orchestration
  // -------------------------------------------------------------------------

  private async speakCascade(
    eventType: string,
    aiContext: string,
    preBakedPool: TemplatePool<string> | null,
    synthFallback: () => string
  ) {
    this.stopAll();

    // Tier 1: Try AI (disabled when AI_GENERATION_ENABLED is false)
    if (AI_GENERATION_ENABLED && this.apiKey) {
      const ok = await this.speakAI(eventType, aiContext);
      if (ok) return;
    }

    // Tier 2: Try pre-baked clips
    if (this.preBakedReady && this.speakPreBaked(preBakedPool)) {
      return;
    }

    // Tier 3: SpeechSynthesis
    this.speakSynthesis(synthFallback());
  }

  // -------------------------------------------------------------------------
  // Helpers
  // -------------------------------------------------------------------------

  private stopAll() {
    if (this.currentAudio) {
      this.currentAudio.pause();
      this.currentAudio.currentTime = 0;
      this.currentAudio = null;
    }
    if (this.synth) this.synth.cancel();
  }

  private playBlob(blob: Blob): Promise<void> {
    return new Promise((resolve, reject) => {
      const url = URL.createObjectURL(blob);
      const audio = new Audio(url);
      this.currentAudio = audio;
      audio.addEventListener("ended", () => {
        URL.revokeObjectURL(url);
        resolve();
      }, { once: true });
      audio.addEventListener("error", () => {
        URL.revokeObjectURL(url);
        reject(new Error("Audio playback failed"));
      }, { once: true });
      audio.play().catch(reject);
    });
  }

  private loadApiKey(): string | null {
    try {
      const stored = localStorage.getItem(STORAGE_KEY_API_KEY);
      if (stored) return stored;
    } catch { /* */ }

    try {
      const envKey = (import.meta as any).env?.VITE_OPENAI_API_KEY;
      if (envKey) return envKey;
    } catch { /* */ }

    return null;
  }

  private loadSetting(key: string, defaultValue: boolean): boolean {
    try {
      const stored = localStorage.getItem(key);
      return stored === null ? defaultValue : stored === "true";
    } catch {
      return defaultValue;
    }
  }

  private saveSetting(key: string, value: boolean | string) {
    try {
      localStorage.setItem(key, String(value));
    } catch { /* */ }
  }
}

type AudioWindow = Window & { webkitAudioContext?: typeof AudioContext };

export type MysteryAudioEffect =
  | "triple-word"
  | "extra-turn"
  | "free-power"
  | "gem-bloom"
  | "letter-burst";
export type AudioGem = "none" | "emerald" | "ruby" | "diamond";

// C major pentatonic — every step stays consonant as a word grows or shrinks.
const SPELL_NOTES_HZ = [261.63, 293.66, 329.63, 392.0, 440.0, 523.25, 587.33, 659.25, 783.99];

const SAMPLE_VOLUMES: Record<string, number> = {
  click: 0.18,
  select: 0.32,
  place: 0.38,
  hintUsed: 0.32,
  error: 0.5,
  win: 0.55
};

let context: AudioContext | null = null;
let masterGain: GainNode | null = null;
const MUTE_KEY = "word-puzzle-audio-muted";
const VOLUME_KEY = "word-puzzle-audio-volume";

const readStoredNumber = (key: string, fallback: number) => {
  try {
    const stored = window.localStorage.getItem(key);
    if (stored === null) return fallback;
    const value = Number(stored);
    return Number.isFinite(value) ? value : fallback;
  } catch {
    return fallback;
  }
};

const readStoredMuted = () => {
  try {
    return window.localStorage.getItem(MUTE_KEY) === "true";
  } catch {
    return false;
  }
};

let muted = typeof window !== "undefined" ? readStoredMuted() : false;
let masterVolume = typeof window !== "undefined" ? Math.max(0, Math.min(1, readStoredNumber(VOLUME_KEY, 0.72))) : 0.72;

const audioConstructor = () => {
  const win = window as AudioWindow;
  return window.AudioContext ?? win.webkitAudioContext;
};

export const unlockGameAudio = () => {
  const Ctor = audioConstructor();
  if (!Ctor) return null;
  if (!context || context.state === "closed") {
    context = new Ctor();
    masterGain = context.createGain();
    masterGain.gain.value = muted ? 0 : masterVolume;
    masterGain.connect(context.destination);
  }
  if (context.state === "suspended") void context.resume();
  return context;
};

const output = (ctx: AudioContext) => masterGain ?? ctx.destination;

export const isGameAudioMuted = () => muted;
export const gameAudioVolume = () => masterVolume;

export const setGameAudioMuted = (value: boolean) => {
  muted = value;
  try {
    window.localStorage.setItem(MUTE_KEY, String(value));
  } catch {
    // Audio preferences can remain session-only when storage is unavailable.
  }
  if (masterGain && context) {
    masterGain.gain.cancelScheduledValues(context.currentTime);
    masterGain.gain.setTargetAtTime(value ? 0 : masterVolume, context.currentTime, 0.015);
  }
};

export const setGameAudioVolume = (value: number) => {
  masterVolume = Math.max(0, Math.min(1, value));
  try {
    window.localStorage.setItem(VOLUME_KEY, String(masterVolume));
  } catch {
    // Audio preferences can remain session-only when storage is unavailable.
  }
  if (masterGain && context && !muted) {
    masterGain.gain.cancelScheduledValues(context.currentTime);
    masterGain.gain.setTargetAtTime(masterVolume, context.currentTime, 0.015);
  }
};

export const sampleVolume = (name: string) => SAMPLE_VOLUMES[name] ?? 0.6;

const playPartial = (
  ctx: AudioContext,
  dest: AudioNode,
  type: OscillatorType,
  hz: number,
  peak: number,
  start: number,
  attack: number,
  duration: number,
  startHz = hz
) => {
  const oscillator = ctx.createOscillator();
  const volume = ctx.createGain();
  oscillator.type = type;
  oscillator.frequency.setValueAtTime(startHz, start);
  if (startHz !== hz) {
    oscillator.frequency.exponentialRampToValueAtTime(hz, start + Math.min(0.12, duration * 0.25));
  }
  volume.gain.setValueAtTime(0.0001, start);
  volume.gain.exponentialRampToValueAtTime(peak, start + attack);
  volume.gain.exponentialRampToValueAtTime(0.0001, start + duration);
  oscillator.connect(volume).connect(dest);
  oscillator.start(start);
  oscillator.stop(start + duration + 0.02);
};

export const playSpellTone = (letterCount: number) => {
  if (letterCount <= 0) return;
  const ctx = unlockGameAudio();
  if (!ctx) return;
  const index = Math.min(SPELL_NOTES_HZ.length - 1, letterCount - 1);
  const hz = SPELL_NOTES_HZ[index];
  const start = ctx.currentTime + 0.004;
  const dest = output(ctx);
  playPartial(ctx, dest, "triangle", hz, 0.05, start, 0.012, 0.22);
  playPartial(ctx, dest, "sine", hz * 2, 0.012, start, 0.01, 0.18);
};

export const playWordSuccess = () => {
  const ctx = unlockGameAudio();
  if (!ctx) return;
  const start = ctx.currentTime + 0.006;
  const filter = ctx.createBiquadFilter();
  filter.type = "lowpass";
  filter.frequency.setValueAtTime(1180, start);
  filter.Q.setValueAtTime(0.65, start);
  filter.connect(output(ctx));

  // Slow-attack C major “aah”: a vocal swell rather than a percussive pluck.
  const voices: Array<[OscillatorType, number, number, number]> = [
    ["triangle", 261.63, 0.048, 0.248],
    ["triangle", 329.63, 0.04, 0.314],
    ["sine", 392.0, 0.044, 0.372],
    ["sine", 523.25, 0.02, 0.496]
  ];
  for (const [type, hz, peak, startHz] of voices) {
    const oscillator = ctx.createOscillator();
    const volume = ctx.createGain();
    oscillator.type = type;
    oscillator.frequency.setValueAtTime(startHz, start);
    oscillator.frequency.exponentialRampToValueAtTime(hz, start + 0.14);
    volume.gain.setValueAtTime(0.0001, start);
    volume.gain.exponentialRampToValueAtTime(peak, start + 0.1);
    volume.gain.exponentialRampToValueAtTime(peak * 0.72, start + 0.42);
    volume.gain.exponentialRampToValueAtTime(0.0001, start + 0.92);
    oscillator.connect(volume).connect(filter);
    oscillator.start(start);
    oscillator.stop(start + 0.95);
  }
};

const playNotes = (
  notes: number[],
  options: { spacing?: number; peak?: number; duration?: number; type?: OscillatorType; start?: number } = {}
) => {
  const ctx = unlockGameAudio();
  if (!ctx || muted) return;
  const start = options.start ?? ctx.currentTime + 0.006;
  const spacing = options.spacing ?? 0.075;
  notes.forEach((hz, index) => {
    playPartial(
      ctx,
      output(ctx),
      options.type ?? "sine",
      hz,
      options.peak ?? 0.035,
      start + index * spacing,
      0.008,
      options.duration ?? 0.2
    );
  });
};

export const playSolvedTileCascade = (cellCount: number) => {
  const count = Math.max(1, Math.min(8, cellCount));
  const notes = Array.from(
    { length: count },
    (_, index) => SPELL_NOTES_HZ[Math.min(index + 1, SPELL_NOTES_HZ.length - 1)]
  );
  playNotes(notes, { spacing: 0.045, peak: 0.018, duration: 0.13, type: "triangle" });
};

export const playSolvedWordBell = () => {
  const ctx = unlockGameAudio();
  if (!ctx || muted) return;
  const start = ctx.currentTime + 0.006;
  const dest = output(ctx);
  // Additive bell partials: a clear fundamental with quiet, quickly fading overtones.
  const partials: Array<[number, number, number]> = [
    [659.25, 0.044, 0.92],
    [1318.5, 0.017, 0.7],
    [1979.6, 0.009, 0.48],
    [2768.9, 0.005, 0.34]
  ];
  for (const [hz, peak, duration] of partials) {
    playPartial(ctx, dest, "sine", hz, peak, start, 0.004, duration);
  }
  // A soft answering tone gives the solve a tiny bit of lift without becoming a fanfare.
  playPartial(ctx, dest, "sine", 987.77, 0.021, start + 0.13, 0.006, 0.62);
};

export const playMysteryReveal = (effect: MysteryAudioEffect) => {
  const ctx = unlockGameAudio();
  if (!ctx || muted) return;
  const start = ctx.currentTime + 0.006;
  playPartial(ctx, output(ctx), "sine", 880, 0.027, start, 0.012, 0.28, 220);
  const motifs: Record<MysteryAudioEffect, number[]> = {
    "triple-word": [261.63, 392, 523.25],
    "extra-turn": [329.63, 392, 493.88],
    "free-power": [293.66, 440, 587.33],
    "gem-bloom": [392, 523.25, 659.25, 783.99],
    "letter-burst": [523.25, 659.25, 783.99]
  };
  playNotes(motifs[effect], {
    start: start + 0.16,
    spacing: effect === "gem-bloom" ? 0.06 : 0.085,
    peak: 0.034,
    duration: 0.3,
    type: effect === "triple-word" ? "triangle" : "sine"
  });
};

export const playMysteryTarget = (index: number) => {
  const note = SPELL_NOTES_HZ[Math.min(SPELL_NOTES_HZ.length - 1, 3 + (index % 5))];
  playNotes([note], { peak: 0.014, duration: 0.11, type: "sine" });
};

export const playGemBloomSparkle = (index: number) => {
  const base = [659.25, 783.99, 880, 1046.5][index % 4];
  playNotes([base, base * 1.5], {
    spacing: 0.055,
    peak: 0.024,
    duration: 0.22,
    type: "sine"
  });
};

export const playScoreFlight = () => {
  playNotes([329.63, 440], { spacing: 0.07, peak: 0.022, duration: 0.22, type: "triangle" });
};

export const playScoreImpact = (gem: AudioGem) => {
  const top = gem === "diamond" ? 1046.5 : gem === "ruby" ? 880 : gem === "emerald" ? 783.99 : 659.25;
  playNotes([top / 2, top], { spacing: 0.035, peak: 0.035, duration: 0.26, type: "triangle" });
};

export const playBonusTallyStep = (step: number) => {
  const index = Math.max(0, Math.min(SPELL_NOTES_HZ.length - 1, step));
  const note = SPELL_NOTES_HZ[index];
  const finalStep = index === SPELL_NOTES_HZ.length - 1;
  playNotes(finalStep ? [note, note * 1.25, note * 1.5] : [note, note * 2], {
    spacing: finalStep ? 0.045 : 0,
    peak: finalStep ? 0.033 : 0.021,
    duration: finalStep ? 0.42 : 0.2,
    type: finalStep ? "sine" : "triangle"
  });
};

type AudioWindow = Window & { webkitAudioContext?: typeof AudioContext };

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

const audioConstructor = () => {
  const win = window as AudioWindow;
  return window.AudioContext ?? win.webkitAudioContext;
};

export const unlockGameAudio = () => {
  const Ctor = audioConstructor();
  if (!Ctor) return null;
  if (!context || context.state === "closed") {
    context = new Ctor();
  }
  if (context.state === "suspended") void context.resume();
  return context;
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
  playPartial(ctx, ctx.destination, "triangle", hz, 0.05, start, 0.012, 0.22);
  playPartial(ctx, ctx.destination, "sine", hz * 2, 0.012, start, 0.01, 0.18);
};

export const playWordSuccess = () => {
  const ctx = unlockGameAudio();
  if (!ctx) return;
  const start = ctx.currentTime + 0.006;
  const filter = ctx.createBiquadFilter();
  filter.type = "lowpass";
  filter.frequency.setValueAtTime(1180, start);
  filter.Q.setValueAtTime(0.65, start);
  filter.connect(ctx.destination);

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

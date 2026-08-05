import type { CharacterDialogueEmission } from "./types";

type FetchLike = typeof fetch;
const FIXED_PHRASES = new Set(["go", "your turn", "thinking about my next move"]);
const FIXED_VOICE_CACHE = "cribbage-fixed-voice-v2";

export type CloudVoiceUsageDelta = {
  requestCount: number;
  charCount: number;
  estimatedUsd: number;
};

export class CloudTableTalkVoiceOutput {
  private enabled = false;
  private volume = 0.8;
  private busy = false;
  private token: string | null = null;
  private tokenExpMs = 0;
  private apiUnavailable = false;
  private reportedApiUnavailable = false;
  private readonly fetcher: FetchLike;
  private readonly onError?: (message: string) => void;
  private readonly onUsage?: (delta: CloudVoiceUsageDelta) => void;
  private readonly onStatus?: (message: string) => void;
  private reportedConnection = false;
  private audioContext: AudioContext | null = null;
  private activeSource: AudioBufferSourceNode | null = null;
  private voicePackManifestPromise: Promise<Record<string, string>> | null = null;

  constructor(options?: {
    enabled?: boolean;
    fetcher?: FetchLike;
    onError?: (message: string) => void;
    onUsage?: (delta: CloudVoiceUsageDelta) => void;
    onStatus?: (message: string) => void;
  }) {
    this.enabled = options?.enabled ?? false;
    this.fetcher = options?.fetcher ?? ((input: RequestInfo | URL, init?: RequestInit) => fetch(input, init));
    this.onError = options?.onError;
    this.onUsage = options?.onUsage;
    this.onStatus = options?.onStatus;
  }

  setEnabled(enabled: boolean) {
    this.enabled = enabled;
    if (!enabled) this.cancel();
  }

  setVolume(volume: number) {
    this.volume = Math.max(0, Math.min(1, volume));
  }

  async speak(line: CharacterDialogueEmission, onPlaybackStart?: () => void): Promise<boolean> {
    if (!this.enabled || this.busy) return false;
    this.busy = true;
    try {
      const packedAudio = await this.readVoicePack(line);
      if (packedAudio) {
        onPlaybackStart?.();
        await this.playAudioBuffer(packedAudio);
        return true;
      }
      const cachedAudio = await this.readFixedPhrase(line);
      if (cachedAudio) {
        onPlaybackStart?.();
        await this.playAudioBuffer(cachedAudio);
        return true;
      }
      const token = await this.ensureSessionToken();
      if (!token) return false;

      const response = await this.fetcher("/api/table-talk-tts", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          "Authorization": `Bearer ${token}`,
        },
        body: JSON.stringify({
          text: line.text,
          characterId: line.characterId,
          eventType: line.eventType,
          emotion: line.emotion,
        }),
      });
      if (!response.ok) {
        const detail = (await response.text()).slice(0, 120);
        this.onError?.(`Cloud voice request failed (${response.status}). ${detail}`);
        return false;
      }
      if (!this.reportedConnection) {
        const provider = response.headers.get("x-table-talk-provider") ?? "unknown";
        const contentType = response.headers.get("content-type") ?? "unknown";
        this.onStatus?.(`Cloud voice connected: ${provider} (${contentType}).`);
        this.reportedConnection = true;
      }
      this.recordUsageFromHeaders(response.headers);

      if (this.isFixedPhrase(line.text)) await this.storeFixedPhrase(line, response.clone());
      const audioData = await response.arrayBuffer();
      onPlaybackStart?.();
      await this.playAudioBuffer(audioData);
      return true;
    } catch (error) {
      const message = error instanceof Error ? error.message : "Unknown cloud voice error.";
      this.onError?.(message);
      return false;
    } finally {
      this.busy = false;
    }
  }

  private recordUsageFromHeaders(headers: Headers) {
    const charCount = Number(headers.get("x-table-talk-char-count") ?? "0");
    const estimatedUsd = Number(headers.get("x-table-talk-estimated-usd") ?? "0");
    this.onUsage?.({
      requestCount: 1,
      charCount: Number.isFinite(charCount) ? Math.max(0, charCount) : 0,
      estimatedUsd: Number.isFinite(estimatedUsd) ? Math.max(0, estimatedUsd) : 0,
    });
  }

  private isFixedPhrase(text: string): boolean {
    return FIXED_PHRASES.has(text.trim().replace(/[.!?]+$/, "").toLowerCase());
  }

  private async readVoicePack(line: CharacterDialogueEmission): Promise<ArrayBuffer | null> {
    if (typeof window === "undefined") return null;
    if (!this.voicePackManifestPromise) {
      const manifestUrl = new URL("table-talk-voice/manifest.json", document.baseURI);
      this.voicePackManifestPromise = this.fetcher(manifestUrl)
        .then(response => response.ok ? response.json() as Promise<Record<string, string>> : {})
        .catch(() => ({}));
    }
    const manifest = await this.voicePackManifestPromise;
    const clipPath = manifest[`${line.characterId}|${line.text}`];
    if (!clipPath) return null;
    try {
      const response = await this.fetcher(new URL(clipPath, document.baseURI));
      return response.ok ? await response.arrayBuffer() : null;
    } catch {
      return null;
    }
  }

  private fixedPhraseRequest(line: CharacterDialogueEmission): Request {
    const phrase = line.text.trim().replace(/[.!?]+$/, "").toLowerCase();
    return new Request(`${window.location.origin}/table-talk-fixed-voice/${line.characterId}/${encodeURIComponent(phrase)}`);
  }

  private async readFixedPhrase(line: CharacterDialogueEmission): Promise<ArrayBuffer | null> {
    if (typeof window === "undefined" || !("caches" in window) || !this.isFixedPhrase(line.text)) return null;
    try {
      const cache = await caches.open(FIXED_VOICE_CACHE);
      const response = await cache.match(this.fixedPhraseRequest(line));
      return response ? await response.arrayBuffer() : null;
    } catch {
      return null;
    }
  }

  private async storeFixedPhrase(line: CharacterDialogueEmission, response: Response): Promise<void> {
    if (typeof window === "undefined" || !("caches" in window)) return;
    try {
      const cache = await caches.open(FIXED_VOICE_CACHE);
      await cache.put(this.fixedPhraseRequest(line), response);
    } catch {
      // Voice still plays if persistent browser caching is unavailable.
    }
  }

  cancel() {
    if (this.activeSource) {
      try {
        this.activeSource.stop();
      } catch {
        // Ignore if source is already ended.
      }
      this.activeSource = null;
    }
    this.busy = false;
  }

  private async playAudioBuffer(audioData: ArrayBuffer): Promise<void> {
    const context = this.getAudioContext();
    if (!context) {
      this.onError?.("Browser does not support AudioContext for cloud voice playback.");
      return;
    }
    if (context.state === "suspended") {
      try {
        await context.resume();
      } catch {
        this.onError?.("Cloud voice is blocked by browser audio permissions.");
        return;
      }
    }

    let decoded: AudioBuffer;
    try {
      decoded = await context.decodeAudioData(audioData.slice(0));
    } catch {
      this.onError?.("Cloud voice audio decode failed.");
      return;
    }

    const source = context.createBufferSource();
    const gain = context.createGain();
    gain.gain.value = this.volume;
    source.buffer = decoded;
    source.connect(gain).connect(context.destination);
    this.activeSource = source;

    await new Promise<void>(resolve => {
      source.onended = () => {
        if (this.activeSource === source) this.activeSource = null;
        resolve();
      };
      source.start();
    });
  }

  private async ensureSessionToken(): Promise<string | null> {
    if (this.apiUnavailable) return null;
    const now = Date.now();
    if (this.token && now < this.tokenExpMs - 5_000) return this.token;

    const response = await this.fetcher("/api/table-talk-session", { method: "POST" });
    if (!response.ok) {
      if (response.status === 404) {
        this.apiUnavailable = true;
        if (!this.reportedApiUnavailable) {
          this.onStatus?.("Cloud voice API is unavailable in this server environment. Falling back to browser voice.");
          this.reportedApiUnavailable = true;
        }
      }
      return null;
    }
    const body = await response.json() as { token?: string; expiresAt?: number };
    if (!body.token || !body.expiresAt) return null;
    this.token = body.token;
    this.tokenExpMs = body.expiresAt;
    return this.token;
  }

  private getAudioContext(): AudioContext | null {
    if (typeof window === "undefined") return null;
    if (this.audioContext) return this.audioContext;
    const AudioCtx = window.AudioContext || (window as unknown as { webkitAudioContext?: typeof AudioContext }).webkitAudioContext;
    if (!AudioCtx) return null;
    this.audioContext = new AudioCtx();
    return this.audioContext;
  }
}

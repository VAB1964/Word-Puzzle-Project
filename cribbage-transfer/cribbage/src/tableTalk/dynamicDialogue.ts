import type { CharacterDialogueEmission } from "./types";

type FetchLike = typeof fetch;

export class DynamicTableTalkClient {
  private token: string | null = null;
  private tokenExpMs = 0;
  private readonly fetcher: FetchLike;
  private readonly timeoutMs: number;

  constructor(options?: { fetcher?: FetchLike; timeoutMs?: number }) {
    this.fetcher = options?.fetcher ?? ((input: RequestInfo | URL, init?: RequestInit) => fetch(input, init));
    this.timeoutMs = options?.timeoutMs ?? 2500;
  }

  async generate(line: CharacterDialogueEmission, recentDialogue: string[] = []): Promise<string | null> {
    if (!line.dynamic) return null;
    const controller = new AbortController();
    const timer = globalThis.setTimeout(() => controller.abort(), this.timeoutMs);
    try {
      const token = await this.ensureSessionToken(controller.signal);
      if (!token) return null;
      const response = await this.fetcher("/api/table-talk-generate", {
        method: "POST",
        signal: controller.signal,
        headers: {
          "Content-Type": "application/json",
          "Authorization": `Bearer ${token}`,
        },
        body: JSON.stringify({
          characterId: line.characterId,
          characterName: line.characterName,
          fallbackText: line.text,
          emotion: line.emotion,
          event: line.event,
          context: line.context,
          recentDialogue: recentDialogue.slice(-4),
        }),
      });
      if (!response.ok) return null;
      const body = await response.json() as { text?: unknown };
      if (typeof body.text !== "string") return null;
      const text = body.text.replace(/\s+/g, " ").trim();
      const wordCount = text.match(/[\p{L}\p{N}]+/gu)?.length ?? 0;
      return wordCount >= 3 && text.length <= 180 ? text : null;
    } catch {
      return null;
    } finally {
      globalThis.clearTimeout(timer);
    }
  }

  private async ensureSessionToken(signal: AbortSignal): Promise<string | null> {
    const now = Date.now();
    if (this.token && now < this.tokenExpMs - 5_000) return this.token;
    const response = await this.fetcher("/api/table-talk-session", { method: "POST", signal });
    if (!response.ok) return null;
    const body = await response.json() as { token?: string; expiresAt?: number };
    if (!body.token || !body.expiresAt) return null;
    this.token = body.token;
    this.tokenExpMs = body.expiresAt;
    return this.token;
  }
}

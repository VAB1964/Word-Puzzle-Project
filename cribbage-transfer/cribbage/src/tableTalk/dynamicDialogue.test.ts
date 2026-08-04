import { describe, expect, it, vi } from "vitest";
import { DynamicTableTalkClient } from "./dynamicDialogue";
import type { CharacterDialogueEmission } from "./types";

function emission(dynamic = true): CharacterDialogueEmission {
  return {
    characterId: "arthur",
    characterName: "Arthur",
    text: "A strong hand. I have no complaints.",
    eventType: "large_hand_scored",
    emotion: "competitive",
    timestamp: 1,
    dynamic,
    event: { type: "large_hand_scored", actorIndex: 1, points: 12 },
    context: {
      level: "chatty",
      playerCount: 2,
      dealerIndex: 0,
      runningCount: 0,
      scores: [
        { playerIndex: 0, team: 0, name: "You", score: 40 },
        { playerIndex: 1, team: 1, name: "Arthur", score: 52 },
      ],
      participants: [
        { playerIndex: 1, team: 1, name: "Arthur", color: "blue", characterId: "arthur" },
      ],
    },
  };
}

describe("DynamicTableTalkClient", () => {
  it("returns a short generated line using only public context", async () => {
    const fetcher = vi.fn()
      .mockResolvedValueOnce(new Response(JSON.stringify({ token: "token", expiresAt: Date.now() + 60_000 }), { status: 200 }))
      .mockResolvedValueOnce(new Response(JSON.stringify({ text: "Twelve points. Apparently the cards approve." }), { status: 200 }));
    const client = new DynamicTableTalkClient({ fetcher, timeoutMs: 100 });

    const text = await client.generate(emission(), ["Mabel: Nicely played."]);

    expect(text).toBe("Twelve points. Apparently the cards approve.");
    const request = JSON.parse(fetcher.mock.calls[1][1].body as string);
    expect(request.context.scores).toHaveLength(2);
    expect(request).not.toHaveProperty("hand");
    expect(request).not.toHaveProperty("crib");
  });

  it("falls back when generation times out", async () => {
    const fetcher = vi.fn()
      .mockResolvedValueOnce(new Response(JSON.stringify({ token: "token", expiresAt: Date.now() + 60_000 }), { status: 200 }))
      .mockImplementationOnce((_input: RequestInfo | URL, init?: RequestInit) => new Promise<Response>((_resolve, reject) => {
        init?.signal?.addEventListener("abort", () => reject(new Error("aborted")));
      }));
    const client = new DynamicTableTalkClient({ fetcher, timeoutMs: 5 });

    await expect(client.generate(emission())).resolves.toBeNull();
  });

  it("does not call the API for mechanical dialogue", async () => {
    const fetcher = vi.fn();
    const client = new DynamicTableTalkClient({ fetcher });

    await expect(client.generate(emission(false))).resolves.toBeNull();
    expect(fetcher).not.toHaveBeenCalled();
  });

  it("rejects an incomplete one-word response", async () => {
    const fetcher = vi.fn()
      .mockResolvedValueOnce(new Response(JSON.stringify({ token: "token", expiresAt: Date.now() + 60_000 }), { status: 200 }))
      .mockResolvedValueOnce(new Response(JSON.stringify({ text: "Well," }), { status: 200 }));
    const client = new DynamicTableTalkClient({ fetcher, timeoutMs: 100 });

    await expect(client.generate(emission())).resolves.toBeNull();
  });
});

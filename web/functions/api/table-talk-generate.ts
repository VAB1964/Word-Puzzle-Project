import { verifySessionToken } from "./_tableTalkSession";

type CharacterId = "mabel" | "arthur" | "clara";
type Env = {
  TABLE_TALK_SESSION_SECRET?: string;
  GEMINI_API_KEY?: string;
  TABLE_TALK_DIALOGUE_MODEL?: string;
};
type RequestBody = {
  characterId?: CharacterId;
  characterName?: string;
  fallbackText?: string;
  emotion?: string;
  event?: { type?: string; [key: string]: unknown };
  context?: {
    playerCount?: number;
    dealerIndex?: number;
    runningCount?: number;
    scores?: Array<{ playerIndex: number; team: number; name: string; score: number }>;
    participants?: Array<{ playerIndex: number; team: number; name: string; characterId: CharacterId }>;
  };
  recentDialogue?: string[];
};

const DYNAMIC_EVENTS = new Set([
  "pegging_scored", "large_hand_scored", "zero_point_hand", "large_crib_scored",
  "lead_changed", "player_close_to_winning", "opponent_close_to_winning",
  "computer_falls_well_behind", "computer_catches_up", "game_won", "game_lost",
]);
const PERSONALITY: Record<CharacterId, string> = {
  mabel: "Warm and encouraging, with gentle teasing. Friendly, relaxed, and never patronizing.",
  arthur: "Dry and understated, with mild self-deprecating humor. Friendly, never cruel or boastful without factual cause.",
  clara: "Cheerful, positive, concise, and grounded. Encouraging without sounding overexcited.",
};

function extractText(payload: unknown): string {
  const data = payload as { candidates?: Array<{ content?: { parts?: Array<{ text?: string }> } }> };
  return data.candidates?.[0]?.content?.parts?.map(part => part.text ?? "").join("").trim() ?? "";
}

function cleanLine(raw: string): string {
  const line = raw.replace(/^["']|["']$/g, "").replace(/\s+/g, " ").trim();
  const wordCount = line.match(/[\p{L}\p{N}]+/gu)?.length ?? 0;
  if (!line || wordCount < 3 || line.length > 180 || /[\r\n]/.test(raw)) return "";
  return line;
}

export const onRequestPost: PagesFunction<Env> = async (context) => {
  const secret = context.env.TABLE_TALK_SESSION_SECRET;
  if (!secret) return new Response("Session secret not configured.", { status: 500 });
  const auth = context.request.headers.get("Authorization") ?? "";
  const token = auth.startsWith("Bearer ") ? auth.slice(7).trim() : "";
  if (!token || !await verifySessionToken(token, secret)) {
    return new Response("Invalid or expired session token.", { status: 401 });
  }

  let body: RequestBody;
  try {
    body = await context.request.json<RequestBody>();
  } catch {
    return new Response("Invalid JSON body.", { status: 400 });
  }
  const characterId = body.characterId;
  const eventType = body.event?.type;
  if (!characterId || !PERSONALITY[characterId] || !eventType || !DYNAMIC_EVENTS.has(eventType)) {
    return new Response("Unsupported character or event.", { status: 400 });
  }
  if (!Array.isArray(body.context?.scores) || body.context.scores.length < 2 || body.context.scores.length > 4) {
    return new Response("Invalid public score context.", { status: 400 });
  }

  const apiKey = context.env.GEMINI_API_KEY;
  if (!apiKey) return new Response("Gemini dialogue key is not configured.", { status: 503 });
  const model = context.env.TABLE_TALK_DIALOGUE_MODEL || "gemini-2.5-flash";
  const publicState = {
    speaker: { id: characterId, name: body.characterName },
    personality: PERSONALITY[characterId],
    emotion: body.emotion,
    event: body.event,
    game: {
      playerCount: body.context.playerCount,
      dealerIndex: body.context.dealerIndex,
      runningCount: body.context.runningCount,
      scores: body.context.scores,
      participants: body.context.participants,
    },
    recentDialogue: (body.recentDialogue ?? []).slice(-4).map(line => String(line).slice(0, 180)),
    scriptedFallback: String(body.fallbackText ?? "").slice(0, 180),
  };
  const prompt = [
    "Write exactly one short spoken reaction for a casual cribbage game.",
    "Use only the supplied public facts. Never invent cards, scores, who is leading, or who won.",
    "The speaker may claim a lead, score, or win only when the supplied event and scores prove it.",
    "Stay in character, avoid repeating recent dialogue, use at most 22 words, and return only the line.",
    JSON.stringify(publicState),
  ].join("\n");

  const response = await fetch(
    `https://generativelanguage.googleapis.com/v1beta/models/${encodeURIComponent(model)}:generateContent`,
    {
      method: "POST",
      headers: { "x-goog-api-key": apiKey, "Content-Type": "application/json" },
      body: JSON.stringify({
        contents: [{ parts: [{ text: prompt }] }],
        generationConfig: { temperature: 0.75, maxOutputTokens: 60 },
      }),
    },
  );
  if (!response.ok) {
    return new Response(`Gemini dialogue error: ${(await response.text()).slice(0, 160)}`, { status: 502 });
  }
  const text = cleanLine(extractText(await response.json()));
  if (!text) return new Response("Gemini returned an invalid dialogue line.", { status: 502 });
  return Response.json({ text }, { headers: { "Cache-Control": "no-store", "x-table-talk-provider": "gemini" } });
};

import { verifySessionToken } from "./_tableTalkSession";

type Env = {
  TABLE_TALK_SESSION_SECRET?: string;
  OPENAI_API_KEY?: string;
  GEMINI_API_KEY?: string;
  TABLE_TALK_TTS_PROVIDER?: string;
  TABLE_TALK_TTS_MODEL?: string;
  TABLE_TALK_TTS_USD_PER_1K_CHARS?: string;
};

type RequestBody = {
  text?: string;
  characterId?: "mabel" | "arthur" | "clara";
  emotion?: "supportive" | "playful" | "dry" | "optimistic" | "competitive" | "concerned" | "self_deprecating";
  eventType?: string;
};

const VOICE_BY_CHARACTER: Record<NonNullable<RequestBody["characterId"]>, string> = {
  mabel: "nova",
  arthur: "onyx",
  clara: "shimmer",
};

const INSTRUCTIONS_BY_CHARACTER: Record<NonNullable<RequestBody["characterId"]>, string> = {
  mabel: "Warm, relaxed, and lightly mischievous, like a friend enjoying a casual card game.",
  arthur: "Dry and mildly competitive, with an easygoing, friendly undertone and a natural conversational pace.",
  clara: "Cheerful and optimistic, but relaxed and grounded, like she is quietly enjoying the game.",
};

const GEMINI_VOICE_BY_CHARACTER: Record<NonNullable<RequestBody["characterId"]>, string> = {
  mabel: "Sulafat",
  arthur: "Achird",
  clara: "Laomedeia",
};

const INSTRUCTIONS_BY_EMOTION: Record<NonNullable<RequestBody["emotion"]>, string> = {
  supportive: "Sound gently encouraging and warm.",
  playful: "Sound lightly amused, never theatrical or mean.",
  dry: "Sound understated and wry, with subtle sarcasm.",
  optimistic: "Sound quietly upbeat and confident.",
  competitive: "Sound engaged and mildly competitive, without intensity.",
  concerned: "Sound thoughtful and slightly cautious, not anxious.",
  self_deprecating: "Use understated humor at your own expense.",
};

function eventInstruction(eventType?: string): string {
  if (!eventType) return "Keep delivery conversational and table-friendly.";
  if (eventType === "game_won") return "Give a pleased but restrained, friendly reaction.";
  if (eventType === "game_lost") return "Deliver graciously and respectfully.";
  if (eventType === "large_hand_scored" || eventType === "large_crib_scored") return "Sound pleasantly surprised without making a big performance of it.";
  if (eventType === "pegging_scored") return "Keep it brief and natural, like an immediate table reaction.";
  if (eventType === "go_declared") return "Say exactly the single word 'Go.' with no extra words.";
  return "Keep delivery conversational and table-friendly.";
}

function decodeBase64(data: string): Uint8Array {
  const binary = atob(data);
  const bytes = new Uint8Array(binary.length);
  for (let index = 0; index < binary.length; index++) bytes[index] = binary.charCodeAt(index);
  return bytes;
}

function pcmToWav(pcm: Uint8Array, sampleRate = 24000): ArrayBuffer {
  const headerSize = 44;
  const buffer = new ArrayBuffer(headerSize + pcm.byteLength);
  const view = new DataView(buffer);
  const writeAscii = (offset: number, value: string) => {
    for (let index = 0; index < value.length; index++) view.setUint8(offset + index, value.charCodeAt(index));
  };
  writeAscii(0, "RIFF");
  view.setUint32(4, 36 + pcm.byteLength, true);
  writeAscii(8, "WAVE");
  writeAscii(12, "fmt ");
  view.setUint32(16, 16, true);
  view.setUint16(20, 1, true);
  view.setUint16(22, 1, true);
  view.setUint32(24, sampleRate, true);
  view.setUint32(28, sampleRate * 2, true);
  view.setUint16(32, 2, true);
  view.setUint16(34, 16, true);
  writeAscii(36, "data");
  view.setUint32(40, pcm.byteLength, true);
  new Uint8Array(buffer, headerSize).set(pcm);
  return buffer;
}

function extractGeminiAudio(payload: unknown): { data: string; mimeType: string } | null {
  const response = payload as {
    candidates?: Array<{ content?: { parts?: Array<{ inlineData?: { data?: string; mimeType?: string } }> } }>;
  };
  const inlineData = response.candidates?.[0]?.content?.parts?.find(part => part.inlineData?.data)?.inlineData;
  if (!inlineData?.data) return null;
  return {
    data: inlineData.data,
    mimeType: inlineData.mimeType ?? "audio/L16;rate=24000",
  };
}

export const onRequestPost: PagesFunction<Env> = async (context) => {
  const secret = context.env.TABLE_TALK_SESSION_SECRET;
  if (!secret) return new Response("Session secret not configured.", { status: 500 });

  const auth = context.request.headers.get("Authorization") ?? "";
  const token = auth.startsWith("Bearer ") ? auth.slice("Bearer ".length).trim() : "";
  if (!token) return new Response("Missing session token.", { status: 401 });

  const valid = await verifySessionToken(token, secret);
  if (!valid) return new Response("Invalid or expired session token.", { status: 401 });

  let body: RequestBody;
  try {
    body = await context.request.json<RequestBody>();
  } catch {
    return new Response("Invalid JSON body.", { status: 400 });
  }

  const rawText = (body.text ?? "").trim();
  const text = body.eventType === "go_declared" ? "Go." : rawText;
  if (!text) return new Response("Missing text.", { status: 400 });
  if (text.length > 260) return new Response("Text too long.", { status: 413 });

  const characterId = body.characterId ?? "mabel";
  const characterInstruction = INSTRUCTIONS_BY_CHARACTER[characterId] ?? "Friendly card-table narration.";
  const emotionInstruction = body.emotion ? INSTRUCTIONS_BY_EMOTION[body.emotion] : "Use a neutral friendly style.";
  const dynamicEventInstruction = eventInstruction(body.eventType);
  const instructions = `${characterInstruction} ${emotionInstruction} ${dynamicEventInstruction} Speak at a brisk, fluid conversational pace. Minimize pauses within and between sentences; do not add dramatic timing, hesitation, or reflective breaks. Keep the delivery concise and natural, as if play is moving quickly around a card table. Have fun without sounding overexcited, theatrical, performative, or exaggerated.`;
  const provider = (context.env.TABLE_TALK_TTS_PROVIDER || "openai").toLowerCase();

  let audioData: ArrayBuffer;
  let contentType: string;
  let estimatedUsd: number;

  if (provider === "gemini") {
    const apiKey = context.env.GEMINI_API_KEY;
    if (!apiKey) return new Response("Gemini TTS key is not configured.", { status: 503 });
    const model = context.env.TABLE_TALK_TTS_MODEL || "gemini-3.1-flash-tts-preview";
    const voiceName = GEMINI_VOICE_BY_CHARACTER[characterId] ?? "Achird";
    const response = await fetch(
      `https://generativelanguage.googleapis.com/v1beta/models/${encodeURIComponent(model)}:generateContent`,
      {
        method: "POST",
        headers: {
          "x-goog-api-key": apiKey,
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          contents: [{ parts: [{ text: `${instructions}\n\nSay exactly: ${text}` }] }],
          generationConfig: {
            responseModalities: ["AUDIO"],
            speechConfig: {
              voiceConfig: {
                prebuiltVoiceConfig: { voiceName },
              },
            },
          },
        }),
      },
    );
    if (!response.ok) {
      const providerError = await response.text();
      return new Response(`Gemini TTS error: ${providerError}`, { status: 502 });
    }
    const payload = await response.json();
    const audio = extractGeminiAudio(payload);
    if (!audio) return new Response("Gemini TTS returned no audio.", { status: 502 });
    const pcm = decodeBase64(audio.data);
    const rateMatch = audio.mimeType.match(/rate=(\d+)/i);
    const sampleRate = rateMatch ? Number(rateMatch[1]) : 24000;
    audioData = pcmToWav(pcm, sampleRate);
    contentType = "audio/wav";
    const durationSeconds = pcm.byteLength / (sampleRate * 2);
    estimatedUsd = durationSeconds * 0.0005;
  } else {
    const apiKey = context.env.OPENAI_API_KEY;
    if (!apiKey) return new Response("OpenAI TTS key is not configured.", { status: 503 });
    const model = context.env.TABLE_TALK_TTS_MODEL || "gpt-4o-mini-tts";
    const voice = VOICE_BY_CHARACTER[characterId] ?? "alloy";
    const response = await fetch("https://api.openai.com/v1/audio/speech", {
      method: "POST",
      headers: {
        "Authorization": `Bearer ${apiKey}`,
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        model,
        voice,
        input: text,
        format: "mp3",
        instructions,
      }),
    });
    if (!response.ok) {
      const providerError = await response.text();
      return new Response(`OpenAI TTS error: ${providerError}`, { status: 502 });
    }
    audioData = await response.arrayBuffer();
    contentType = "audio/mpeg";
    const usdPer1kCharsRaw = Number(context.env.TABLE_TALK_TTS_USD_PER_1K_CHARS ?? "0.015");
    const usdPer1kChars = Number.isFinite(usdPer1kCharsRaw) && usdPer1kCharsRaw >= 0 ? usdPer1kCharsRaw : 0.015;
    estimatedUsd = (text.length / 1000) * usdPer1kChars;
  }

  return new Response(audioData, {
    headers: {
      "Content-Type": contentType,
      "Cache-Control": "no-store",
      "x-table-talk-char-count": String(text.length),
      "x-table-talk-estimated-usd": estimatedUsd.toFixed(6),
      "x-table-talk-provider": provider,
    },
  });
};

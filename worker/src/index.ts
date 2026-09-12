import {
  type CreateRoomResponse,
  type JoinRoomResponse
} from "../../shared/multiplayer/types";
import { WordPuzzleRoom } from "./game-room";
import {
  normalizeRoomCode,
  parseJsonBody,
  ProtocolError,
  validateCreateRequest,
  validateJoinRequest
} from "./protocol";

export { WordPuzzleRoom };

const CODE_ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

const createRoomCode = () => {
  const bytes = new Uint8Array(6);
  crypto.getRandomValues(bytes);
  return Array.from(bytes, (value) => CODE_ALPHABET[value % CODE_ALPHABET.length]).join("");
};

const json = (value: unknown, status = 200) =>
  Response.json(value, {
    status,
    headers: {
      "cache-control": "no-store",
      "content-type": "application/json; charset=utf-8"
    }
  });

const assertOrigin = (request: Request, env: Env) => {
  const origin = request.headers.get("origin");
  if (!origin) return;
  const requestOrigin = new URL(request.url).origin;
  const allowed = new Set([
    requestOrigin,
    ...env.ALLOWED_ORIGIN.split(",").map((value) => value.trim()).filter(Boolean)
  ]);
  if (!allowed.has(origin)) throw new ProtocolError("ORIGIN_REJECTED", "Request origin is not allowed.", 403);
};

const websocketUrl = (request: Request, code: string, participantId: string, token: string) => {
  const url = new URL(`/api/wordpuzzle/rooms/${code}/ws`, request.url);
  url.protocol = url.protocol === "https:" ? "wss:" : "ws:";
  url.searchParams.set("participantId", participantId);
  url.searchParams.set("token", token);
  return url.toString();
};

export default {
  async fetch(request: Request, env: Env): Promise<Response> {
    try {
      const url = new URL(request.url);
      if (request.method === "GET" && url.pathname === "/api/wordpuzzle/health") {
        return json({ ok: true });
      }
      if (request.method === "GET" && /^\/wordpuzzle\/room\/[A-Za-z2-9]{6}\/?$/.test(url.pathname)) {
        const code = normalizeRoomCode(url.pathname.split("/").filter(Boolean).at(-1) ?? "");
        return Response.redirect(new URL(`/wordpuzzle/?room=${code}`, url), 302);
      }
      if (request.method === "POST" && url.pathname === "/api/wordpuzzle/rooms") {
        assertOrigin(request, env);
        const body = validateCreateRequest(await parseJsonBody(request));
        for (let attempt = 0; attempt < 8; attempt += 1) {
          const code = createRoomCode();
          const room = env.WORD_PUZZLE_ROOMS.getByName(code);
          const result = await room.createRoom(code, body.name, body.settings);
          if (!result) continue;
          return json({
            code,
            participantId: result.participantId,
            reconnectToken: result.reconnectToken,
            wsUrl: websocketUrl(request, code, result.participantId, result.reconnectToken)
          } satisfies CreateRoomResponse, 201);
        }
        throw new ProtocolError("CODE_EXHAUSTED", "Could not allocate a room code.", 503);
      }

      const joinMatch = url.pathname.match(/^\/api\/wordpuzzle\/rooms\/([A-Za-z2-9]{6})\/join$/);
      if (request.method === "POST" && joinMatch) {
        assertOrigin(request, env);
        const code = normalizeRoomCode(joinMatch[1]);
        const body = validateJoinRequest(await parseJsonBody(request));
        const room = env.WORD_PUZZLE_ROOMS.getByName(code);
        const result = await room.joinRoom(body.name);
        if (!result.ok) {
          throw new ProtocolError(result.code, result.message, result.status);
        }
        return json({
          code,
          participantId: result.participantId,
          reconnectToken: result.reconnectToken,
          wsUrl: websocketUrl(request, code, result.participantId, result.reconnectToken)
        } satisfies JoinRoomResponse);
      }

      const wsMatch = url.pathname.match(/^\/api\/wordpuzzle\/rooms\/([A-Za-z2-9]{6})\/ws$/);
      if (request.method === "GET" && wsMatch) {
        assertOrigin(request, env);
        if (request.headers.get("upgrade")?.toLowerCase() !== "websocket") {
          throw new ProtocolError("UPGRADE_REQUIRED", "A WebSocket upgrade is required.", 426);
        }
        const code = normalizeRoomCode(wsMatch[1]);
        const participantId = url.searchParams.get("participantId") ?? "";
        const token = url.searchParams.get("token") ?? "";
        if (!participantId || !token) {
          throw new ProtocolError("MISSING_CREDENTIAL", "Room credentials are required.", 401);
        }
        return env.WORD_PUZZLE_ROOMS.getByName(code).fetch(request);
      }

      return json({ code: "NOT_FOUND", message: "Route not found." }, 404);
    } catch (error) {
      const known =
        error instanceof ProtocolError
          ? error
          : new ProtocolError("SERVER_ERROR", "The multiplayer service encountered an error.", 500);
      const log = JSON.stringify({
        event: "request_error",
        code: known.code,
        status: known.status,
        message: error instanceof Error ? error.message : String(error)
      });
      if (known.status >= 500) console.error(log);
      else console.warn(log);
      return json({ code: known.code, message: known.message }, known.status);
    }
  }
} satisfies ExportedHandler<Env>;

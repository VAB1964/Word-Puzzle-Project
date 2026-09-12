import {
  MULTIPLAYER_PROTOCOL_VERSION,
  type ClientCommand,
  type CreateRoomRequest,
  type CreateRoomResponse,
  type JoinRoomResponse,
  type PresentationEvent,
  type RoomSnapshot,
  type ServerMessage
} from "../../../shared/multiplayer/types";

const API_BASE = (import.meta.env.VITE_MULTIPLAYER_API_BASE as string | undefined)?.replace(/\/$/, "") ?? "";
const IDENTITY_KEY = "wordpuzzle.multiplayer.identity.v1";
const ROOM_KEY_PREFIX = "wordpuzzle.multiplayer.room.v1.";

export interface SavedIdentity {
  name: string;
}

export interface SavedRoomCredential {
  code: string;
  participantId: string;
  reconnectToken: string;
}

export interface MultiplayerClientHooks {
  onSnapshot(snapshot: RoomSnapshot, events: PresentationEvent[]): void;
  onConnection(status: "connecting" | "connected" | "reconnecting" | "closed"): void;
  onError(message: string): void;
}

type ClientCommandInput<T extends ClientCommand = ClientCommand> = T extends ClientCommand
  ? Omit<T, "protocolVersion" | "commandId" | "sessionId" | "puzzleId"> &
      Partial<Pick<T, "sessionId" | "puzzleId">>
  : never;

const apiUrl = (path: string) => `${API_BASE}${path}`;

const parseApiError = async (response: Response) => {
  const body = (await response.json().catch(() => ({}))) as { message?: string };
  throw new Error(body.message ?? `Multiplayer request failed (${response.status}).`);
};

export const loadIdentity = (): SavedIdentity => {
  try {
    const saved = JSON.parse(localStorage.getItem(IDENTITY_KEY) ?? "{}") as Partial<SavedIdentity>;
    return { name: typeof saved.name === "string" ? saved.name : "" };
  } catch {
    return { name: "" };
  }
};

export const saveIdentity = (identity: SavedIdentity) => {
  localStorage.setItem(IDENTITY_KEY, JSON.stringify(identity));
};

export const saveRoomCredential = (credential: SavedRoomCredential) => {
  localStorage.setItem(`${ROOM_KEY_PREFIX}${credential.code}`, JSON.stringify(credential));
};

export const loadRoomCredential = (code: string): SavedRoomCredential | null => {
  try {
    const value = JSON.parse(localStorage.getItem(`${ROOM_KEY_PREFIX}${code}`) ?? "null") as
      | SavedRoomCredential
      | null;
    return value?.code === code && value.participantId && value.reconnectToken ? value : null;
  } catch {
    return null;
  }
};

export const createRoom = async (request: CreateRoomRequest) => {
  const response = await fetch(apiUrl("/api/wordpuzzle/rooms"), {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify(request)
  });
  if (!response.ok) return parseApiError(response);
  return response.json() as Promise<CreateRoomResponse>;
};

export const joinRoom = async (code: string, name: string) => {
  const response = await fetch(apiUrl(`/api/wordpuzzle/rooms/${code}/join`), {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify({ name })
  });
  if (!response.ok) return parseApiError(response);
  return response.json() as Promise<JoinRoomResponse>;
};

export class MultiplayerClient {
  private socket: WebSocket | null = null;
  private reconnectTimer: number | null = null;
  private reconnectAttempt = 0;
  private intentionallyClosed = false;
  private lastEventSequence = 0;
  private snapshot: RoomSnapshot | null = null;

  constructor(
    private credential: SavedRoomCredential,
    private readonly hooks: MultiplayerClientHooks
  ) {}

  connect() {
    this.intentionallyClosed = false;
    this.openSocket();
  }

  close() {
    this.intentionallyClosed = true;
    if (this.reconnectTimer !== null) window.clearTimeout(this.reconnectTimer);
    this.reconnectTimer = null;
    this.socket?.close(1000, "Left room");
    this.socket = null;
    this.hooks.onConnection("closed");
  }

  send(command: ClientCommandInput) {
    if (!this.socket || this.socket.readyState !== WebSocket.OPEN) {
      this.hooks.onError("You are offline. Reconnect before trying that action.");
      return false;
    }
    const message = {
      ...command,
      protocolVersion: MULTIPLAYER_PROTOCOL_VERSION,
      commandId: crypto.randomUUID(),
      sessionId: command.sessionId ?? this.snapshot?.sessionId ?? null,
      puzzleId: command.puzzleId ?? this.snapshot?.puzzle?.id ?? null
    } as ClientCommand;
    this.socket.send(JSON.stringify(message));
    return true;
  }

  private openSocket() {
    this.hooks.onConnection(this.reconnectAttempt === 0 ? "connecting" : "reconnecting");
    const url = new URL(
      apiUrl(`/api/wordpuzzle/rooms/${this.credential.code}/ws`),
      window.location.href
    );
    url.protocol = url.protocol === "https:" ? "wss:" : "ws:";
    url.searchParams.set("participantId", this.credential.participantId);
    url.searchParams.set("token", this.credential.reconnectToken);
    const socket = new WebSocket(url);
    this.socket = socket;

    socket.addEventListener("open", () => {
      if (socket !== this.socket) return;
      this.reconnectAttempt = 0;
      this.hooks.onConnection("connected");
    });
    socket.addEventListener("message", (event) => {
      if (socket !== this.socket) return;
      this.handleMessage(String(event.data));
    });
    socket.addEventListener("close", (event) => {
      if (socket !== this.socket) return;
      this.socket = null;
      if (event.code === 4003 || event.code === 4004) {
        this.intentionallyClosed = true;
        this.hooks.onError(event.reason || "This room is no longer available.");
      }
      if (!this.intentionallyClosed) this.scheduleReconnect();
    });
    socket.addEventListener("error", () => {
      if (socket === this.socket) socket.close();
    });
  }

  private handleMessage(raw: string) {
    let message: ServerMessage;
    try {
      message = JSON.parse(raw) as ServerMessage;
    } catch {
      this.hooks.onError("The server sent an unreadable response.");
      return;
    }
    if (message.type === "error") {
      this.hooks.onError(message.message);
      return;
    }
    if (message.type === "pong") return;

    const { snapshot, events } = message;
    if (snapshot.revision < (this.snapshot?.revision ?? 0)) return;
    const ordered = events
      .filter((event) => event.sequence > this.lastEventSequence)
      .sort((left, right) => left.sequence - right.sequence);
    if (ordered.length > 0 && ordered[0].sequence > this.lastEventSequence + 1) {
      this.send({ type: "request-snapshot" });
      ordered.length = 0;
    }
    for (const event of ordered) this.lastEventSequence = event.sequence;
    this.lastEventSequence = Math.max(this.lastEventSequence, snapshot.eventSequence - ordered.length);
    this.snapshot = snapshot;
    if (message.type === "welcome") {
      this.credential = { ...this.credential, reconnectToken: message.reconnectToken };
      saveRoomCredential(this.credential);
      this.lastEventSequence = snapshot.eventSequence;
    }
    this.hooks.onSnapshot(snapshot, ordered);
  }

  private scheduleReconnect() {
    this.reconnectAttempt += 1;
    if (this.reconnectAttempt > 8) {
      this.intentionallyClosed = true;
      this.hooks.onConnection("closed");
      this.hooks.onError("This room is unavailable or has expired.");
      return;
    }
    const delay = Math.min(10_000, 500 * 2 ** Math.min(this.reconnectAttempt - 1, 5));
    this.hooks.onConnection("reconnecting");
    this.reconnectTimer = window.setTimeout(() => {
      this.reconnectTimer = null;
      if (!this.intentionallyClosed) this.openSocket();
    }, delay);
  }
}

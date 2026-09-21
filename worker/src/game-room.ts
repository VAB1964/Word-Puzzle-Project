import { DurableObject } from "cloudflare:workers";
import wordsCsv from "../../words_processed.csv";
import {
  createPuzzleRuntime,
  puzzleIsComplete,
  submitGuess,
  useHint
} from "../../shared/multiplayer/rules";
import { emptyScore } from "../../shared/multiplayer/scoring";
import {
  generateMultiplayerPuzzle,
  parseMultiplayerWordData
} from "../../shared/multiplayer/puzzles";
import {
  MULTIPLAYER_PROTOCOL_VERSION,
  type AiLevel,
  type ClientCommand,
  type Participant,
  type PresentationEvent,
  type ReconnectRecord,
  type RoomSettings,
  type RoomSnapshot,
  type RoomState,
  type ScoreBreakdown,
  type ServerMessage,
  type StoredCommandResult,
  type GemType,
  type TurnState,
  type TurnTimeLimit
} from "../../shared/multiplayer/types";
import { parseCommand, ProtocolError, validateName, validateSettings } from "./protocol";

const WORD_DATA = parseMultiplayerWordData(wordsCsv);
const DICTIONARY_WORDS = new Set(WORD_DATA.map((word) => word.text));
const COLORS = ["#2563eb", "#dc2626", "#16a34a", "#9333ea"];
const AI_NAMES = [
  "Ada",
  "Theo",
  "Mira",
  "Jules",
  "Iris",
  "Niko",
  "Sage",
  "Ember"
];
const ACTIVE_EXPIRY_MS = 24 * 60 * 60 * 1_000;
const COMPLETED_EXPIRY_MS = 60 * 60 * 1_000;
const HOST_GRACE_MS = 30_000;
const COMMAND_RETENTION_MS = 60 * 60 * 1_000;

interface SocketAttachment {
  participantId: string;
  connectionId: string;
  commandTimes: number[];
}

interface CredentialStore {
  [participantId: string]: ReconnectRecord;
}

interface CommandStore {
  [key: string]: StoredCommandResult;
}

interface RoomJoinResult {
  participantId: string;
  reconnectToken: string;
}

type RoomJoinOutcome =
  | ({ ok: true } & RoomJoinResult)
  | { ok: false; code: string; message: string; status: number };

const puzzleCountFor = (settings: RoomSettings) => settings.puzzlesPerRound;

const randomToken = () => {
  const bytes = new Uint8Array(32);
  crypto.getRandomValues(bytes);
  return Array.from(bytes, (value) => value.toString(16).padStart(2, "0")).join("");
};

const randomId = (prefix: string) => `${prefix}_${crypto.randomUUID()}`;

const hashToken = async (token: string) => {
  const digest = await crypto.subtle.digest("SHA-256", new TextEncoder().encode(token));
  return Array.from(new Uint8Array(digest), (value) => value.toString(16).padStart(2, "0")).join("");
};

const constantTimeEqual = (left: string, right: string) => {
  if (left.length !== right.length) return false;
  let difference = 0;
  for (let index = 0; index < left.length; index += 1) {
    difference |= left.charCodeAt(index) ^ right.charCodeAt(index);
  }
  return difference === 0;
};

const aiDelay = (level: AiLevel) => {
  const ranges: Record<AiLevel, [number, number]> = {
    "High School": [12_000, 24_000],
    College: [5_000, 11_000],
    Professional: [3_500, 8_000]
  };
  const [minimum, maximum] = ranges[level];
  return minimum + Math.floor(Math.random() * (maximum - minimum));
};

const turnDurationMs = (turnTimeLimit: TurnTimeLimit) =>
  turnTimeLimit === "Not Timed" ? null : turnTimeLimit * 1_000;

const defaultEnabledPowerUps = () => ({
  letter: true,
  random: true,
  "full-word": true,
  "first-of-each": true
});

export class WordPuzzleRoom extends DurableObject<Env> {
  private state: RoomState | null = null;
  private credentials: CredentialStore = {};
  private commandResults: CommandStore = {};
  private loaded = false;

  private async load() {
    if (this.loaded) return;
    const [state, credentials, commandResults] = await Promise.all([
      this.ctx.storage.get<RoomState>("state"),
      this.ctx.storage.get<CredentialStore>("credentials"),
      this.ctx.storage.get<CommandStore>("commandResults")
    ]);
    this.state = state
      ? {
          ...state,
          settings: {
            ...state.settings,
            puzzlesPerRound: state.settings.puzzlesPerRound ?? 3,
            enabledPowerUps: state.settings.enabledPowerUps ?? defaultEnabledPowerUps()
          },
          aiIntents: state.aiIntents ?? {},
          turnState: state.turnState ?? null
        }
      : null;
    this.credentials = credentials ?? {};
    this.commandResults = commandResults ?? {};
    this.loaded = true;
  }

  async createRoom(code: string, name: string, settings: RoomSettings): Promise<RoomJoinResult | null> {
    await this.load();
    if (this.state) return null;
    const now = Date.now();
    const participantId = randomId("p");
    const token = randomToken();
    const participant = this.createHuman(participantId, validateName(name), 0, now);
    const state: RoomState = {
      code,
      revision: 1,
      eventSequence: 0,
      status: "lobby",
      createdAt: now,
      updatedAt: now,
      hostId: participantId,
      settings: validateSettings(settings),
      rosterLocked: false,
      sessionId: null,
      puzzleIndex: 0,
      puzzleCount: puzzleCountFor(settings),
      participants: [participant],
      puzzle: null,
      runtime: null,
      turnState: null,
      usedBaseWords: [],
      aiDeadlines: {},
      aiIntents: {},
      events: [],
      paused: false,
      pauseReason: null,
      disconnectedParticipantId: null,
      hostTransferAt: null,
      skipVote: null
    };
    this.credentials[participantId] = {
      participantId,
      tokenHash: await hashToken(token),
      revoked: false
    };
    await this.ctx.storage.put({
      state,
      credentials: this.credentials,
      commandResults: this.commandResults
    });
    this.state = state;
    await this.scheduleAlarm();
    return { participantId, reconnectToken: token };
  }

  async joinRoom(name: string): Promise<RoomJoinOutcome> {
    await this.load();
    const state = this.state;
    if (!state) {
      return { ok: false, code: "ROOM_NOT_FOUND", message: "This room does not exist or expired.", status: 404 };
    }
    if (this.isExpired(state)) {
      return { ok: false, code: "ROOM_EXPIRED", message: "This room has expired.", status: 410 };
    }
    if (state.status !== "lobby" || state.rosterLocked) {
      return { ok: false, code: "SESSION_STARTED", message: "This session has already started.", status: 409 };
    }
    if (state.participants.length >= state.settings.capacity) {
      return { ok: false, code: "ROOM_FULL", message: "This room is full.", status: 409 };
    }
    const participantId = randomId("p");
    const token = randomToken();
    const next = structuredClone(state);
    next.participants.push(this.createHuman(participantId, validateName(name), this.openSeat(next), Date.now()));
    this.touch(next);
    const nextCredentials = structuredClone(this.credentials);
    nextCredentials[participantId] = {
      participantId,
      tokenHash: await hashToken(token),
      revoked: false
    };
    await this.ctx.storage.put({ state: next, credentials: nextCredentials });
    this.credentials = nextCredentials;
    this.state = next;
    await this.broadcast([]);
    return { ok: true, participantId, reconnectToken: token };
  }

  // WebSocket upgrade responses cannot cross the RPC serialization boundary,
  // so upgrades use the Durable Object fetch transport while room mutations use RPC.
  async fetch(request: Request): Promise<Response> {
    try {
      const url = new URL(request.url);
      const participantId = url.searchParams.get("participantId") ?? "";
      const token = url.searchParams.get("token") ?? "";
      return await this.openWebSocket(participantId, token);
    } catch (error) {
      const known =
        error instanceof ProtocolError
          ? error
          : new ProtocolError("SERVER_ERROR", "The room could not open the connection.", 500);
      return Response.json(
        { code: known.code, message: known.message },
        { status: known.status, headers: { "cache-control": "no-store" } }
      );
    }
  }

  private async openWebSocket(participantId: string, token: string): Promise<Response> {
    await this.load();
    const state = this.requireState();
    this.assertNotExpired(state);
    await this.authenticate(participantId, token);
    const participant = state.participants.find((candidate) => candidate.id === participantId);
    if (!participant || participant.kind !== "human") {
      throw new ProtocolError("PARTICIPANT_NOT_FOUND", "That participant is no longer available.", 404);
    }

    const pair = new WebSocketPair();
    const client = pair[0];
    const server = pair[1];
    const connectionId = crypto.randomUUID();
    for (const socket of this.ctx.getWebSockets(participantId)) {
      socket.close(4001, "Replaced by a newer connection");
    }
    server.serializeAttachment({ participantId, connectionId, commandTimes: [] } satisfies SocketAttachment);
    this.ctx.acceptWebSocket(server, [participantId]);

    const next = structuredClone(state);
    const nextParticipant = next.participants.find((candidate) => candidate.id === participantId)!;
    const returned = !nextParticipant.connected;
    nextParticipant.connected = true;
    nextParticipant.ready = next.status === "lobby" ? nextParticipant.ready : true;
    if (next.disconnectedParticipantId === participantId) {
      next.disconnectedParticipantId = null;
      next.hostTransferAt = null;
    }
    this.touch(next);
    const events = returned && state.updatedAt !== state.createdAt
      ? [this.event(next, "participant-returned", `${nextParticipant.name} reconnected.`, participantId)]
      : [];
    await this.persist(next);

    const welcome: ServerMessage = {
      type: "welcome",
      snapshot: this.snapshotFor(next, participantId),
      reconnectToken: token,
      events: []
    };
    server.send(JSON.stringify(welcome));
    await this.broadcast(events, connectionId);
    return new Response(null, { status: 101, webSocket: client });
  }

  async webSocketMessage(socket: WebSocket, rawMessage: string | ArrayBuffer) {
    const attachment = socket.deserializeAttachment() as SocketAttachment | null;
    if (!attachment) {
      socket.close(4003, "Missing connection identity");
      return;
    }
    try {
      const now = Date.now();
      attachment.commandTimes = attachment.commandTimes.filter((at) => now - at < 10_000);
      if (attachment.commandTimes.length >= 30) {
        throw new ProtocolError("RATE_LIMITED", "Too many actions. Please slow down.", 429);
      }
      attachment.commandTimes.push(now);
      socket.serializeAttachment(attachment);
      const command = parseCommand(rawMessage);
      await this.handleCommand(socket, attachment.participantId, command);
    } catch (error) {
      this.sendError(socket, error);
    }
  }

  async webSocketClose(socket: WebSocket) {
    await this.handleSocketGone(socket);
  }

  async webSocketError(socket: WebSocket) {
    await this.handleSocketGone(socket);
  }

  private async handleSocketGone(socket: WebSocket) {
    await this.load();
    const attachment = socket.deserializeAttachment() as SocketAttachment | null;
    const state = this.state;
    if (!attachment || !state) return;
    const replacementExists = this.ctx.getWebSockets(attachment.participantId).some((candidate) => {
      if (candidate === socket) return false;
      const other = candidate.deserializeAttachment() as SocketAttachment | null;
      return other?.connectionId !== attachment.connectionId;
    });
    if (replacementExists) return;
    const participant = state.participants.find((candidate) => candidate.id === attachment.participantId);
    if (!participant || !participant.connected) return;

    const next = structuredClone(state);
    const nextParticipant = next.participants.find((candidate) => candidate.id === attachment.participantId)!;
    nextParticipant.connected = false;
    if (next.status === "lobby") nextParticipant.ready = false;
    if (next.status === "playing" || next.status === "puzzle-summary") {
      next.paused = true;
      next.pauseReason =
        attachment.participantId === next.hostId ? "host-disconnected" : "participant-disconnected";
      next.disconnectedParticipantId = attachment.participantId;
    }
    if (attachment.participantId === next.hostId) {
      next.hostTransferAt = Date.now() + HOST_GRACE_MS;
    }
    this.touch(next);
    const events = [
      this.event(
        next,
        "participant-disconnected",
        `${nextParticipant.name} disconnected. The game is paused.`,
        nextParticipant.id
      )
    ];
    await this.persist(next);
    await this.scheduleAlarm();
    await this.broadcast(events);
  }

  private async handleCommand(socket: WebSocket, participantId: string, command: ClientCommand) {
    await this.load();
    const state = this.requireState();
    const commandKey = `${participantId}:${command.commandId}`;
    const previous = this.commandResults[commandKey];
    if (previous) {
      socket.send(JSON.stringify(previous.message));
      return;
    }
    if (command.type === "ping") {
      socket.send(JSON.stringify({ type: "pong", at: Date.now() } satisfies ServerMessage));
      return;
    }
    if (command.type === "request-snapshot") {
      socket.send(
        JSON.stringify({
          type: "snapshot",
          snapshot: this.snapshotFor(state, participantId),
          events: []
        } satisfies ServerMessage)
      );
      return;
    }
    this.assertCurrentCommand(state, command);
    const next = structuredClone(state);
    const credentialsBefore = this.credentials;
    const commandResultsBefore = this.commandResults;
    if (command.type === "replace-with-ai") this.credentials = structuredClone(this.credentials);
    this.commandResults = structuredClone(this.commandResults);
    const events = this.applyCommand(next, participantId, command);
    this.touch(next);
    const response: ServerMessage = {
      type: "snapshot",
      snapshot: this.snapshotFor(next, participantId),
      events
    };
    this.commandResults[commandKey] = {
      participantId,
      commandId: command.commandId,
      revision: next.revision,
      message: response,
      at: Date.now()
    };
    this.pruneCommandResults();
    const attachment = socket.deserializeAttachment() as SocketAttachment | null;
    const excludedConnectionId = attachment?.connectionId;
    try {
      await this.persist(next, command.type === "replace-with-ai", true);
    } catch (error) {
      this.credentials = credentialsBefore;
      this.commandResults = commandResultsBefore;
      throw error;
    }
    await this.scheduleAlarm();
    try {
      socket.send(JSON.stringify(response));
    } catch {
      // If this send fails, broadcast may still deliver updates to replacement sockets.
    }
    await this.broadcast(events, excludedConnectionId);
  }

  private applyCommand(
    state: RoomState,
    participantId: string,
    command: ClientCommand
  ): PresentationEvent[] {
    const actor = state.participants.find((participant) => participant.id === participantId);
    if (!actor || actor.kind !== "human" || !actor.connected) {
      throw new ProtocolError("NOT_CONNECTED", "Participant is not connected.", 403);
    }
    const hostOnly = () => {
      if (state.hostId !== participantId) throw new ProtocolError("HOST_ONLY", "Only the host can do that.", 403);
    };
    const lobbyOnly = () => {
      if (state.status !== "lobby") throw new ProtocolError("LOBBY_ONLY", "That setting is locked.");
    };
    const activeOnly = () => {
      if (state.status !== "playing" || !state.puzzle || !state.runtime) {
        throw new ProtocolError("NOT_PLAYING", "There is no active puzzle.");
      }
      if (state.paused) throw new ProtocolError("GAME_PAUSED", "The game is paused.");
    };
    const currentTurnOnly = () => {
      if (!this.isTurnBased(state)) return;
      if (!state.turnState || state.turnState.activeParticipantId !== participantId) {
        throw new ProtocolError("NOT_YOUR_TURN", "Wait for your turn.");
      }
    };

    if (command.type === "set-ready") {
      lobbyOnly();
      actor.ready = command.ready;
      return [];
    }
    if (command.type === "update-settings") {
      hostOnly();
      lobbyOnly();
      if (command.expectedRevision !== state.revision) {
        throw new ProtocolError("STALE_LOBBY", "The lobby changed. Review the latest settings and try again.");
      }
      const settings = validateSettings(command.settings);
      if (settings.capacity < state.participants.length) {
        throw new ProtocolError("CAPACITY_TOO_SMALL", "Remove seats before lowering capacity.");
      }
      state.settings = settings;
      state.puzzleCount = puzzleCountFor(settings);
      state.participants.forEach((participant) => {
        if (participant.kind === "human") participant.ready = false;
      });
      return [];
    }
    if (command.type === "add-ai") {
      hostOnly();
      lobbyOnly();
      if (state.participants.length >= state.settings.capacity) {
        throw new ProtocolError("ROOM_FULL", "This room is full.");
      }
      state.participants.push(this.createAi(state, command.level, this.openSeat(state)));
      this.clearHumanReadiness(state);
      return [];
    }
    if (command.type === "remove-ai") {
      hostOnly();
      lobbyOnly();
      const target = state.participants.find((participant) => participant.id === command.participantId);
      if (!target || target.kind !== "ai") throw new ProtocolError("INVALID_SEAT", "Choose an AI seat.");
      state.participants = state.participants.filter((participant) => participant.id !== target.id);
      this.clearHumanReadiness(state);
      return [];
    }
    if (command.type === "start-session") {
      hostOnly();
      lobbyOnly();
      if (state.participants.length < 1) throw new ProtocolError("EMPTY_ROOM", "Add at least one participant.");
      const unavailable = state.participants.some(
        (participant) => participant.kind === "human" && (!participant.connected || !participant.ready)
      );
      if (unavailable) throw new ProtocolError("NOT_READY", "Every connected human must be ready.");
      state.rosterLocked = true;
      state.sessionId = randomId("session");
      state.puzzleIndex = 0;
      state.usedBaseWords = [];
      state.participants.forEach((participant) => {
        participant.score = emptyScore();
        participant.hintCredits = 0;
        participant.continued = false;
      });
      return this.startPuzzle(state);
    }
    if (command.type === "submit-guess") {
      activeOnly();
      currentTurnOnly();
      const result = submitGuess(
        state.puzzle!,
        state.runtime!,
        state.participants,
        participantId,
        command.guess,
        DICTIONARY_WORDS
      );
      const events: PresentationEvent[] = [];
      if (!result.changed) {
        if (result.needsPuzzleReview) {
          console.warn(JSON.stringify({
            event: "word_puzzle_membership_issue",
            word: command.guess.trim().toLowerCase(),
            puzzle_word: state.puzzle!.baseWord ?? state.puzzle!.baseLetters.toLowerCase(),
            puzzle_id: state.puzzle!.id,
            room_code: state.code
          }));
        }
        events.push(
          this.event(
            state,
            "guess-rejected",
            result.error ?? `The word "${command.guess.trim().toUpperCase()}" is not in the puzzle and is not a bonus word.`,
            actor.id
          )
        );
        if (this.shouldEndTurnOnGuessAttempt(state)) {
          events.push(...this.advanceTurn(state, "attempt-ended"));
        }
        return events;
      }
      if (result.kind === "bonus") {
        events.push(
          this.event(
            state,
            "bonus-claimed",
            `${actor.name} claimed bonus word ${result.claimedBonusWord?.toUpperCase()} (+${result.hintCreditsAwarded} hint credits).`,
            actor.id
          )
        );
      } else {
        const solved = result.solvedWords
          .map((id) => state.puzzle!.words.find((word) => word.id === id)?.answer.toUpperCase())
          .filter(Boolean)
          .join(", ");
        const summary = `${actor.name} solved ${solved || "a word"} +${result.pointsAwarded}.`;
        const text = `${summary}\n${this.formatAwardBreakdown(result.scoreAwarded)}`;
        events.push(
          this.event(
            state,
            "word-solved",
            text,
            actor.id,
            result.pointsAwarded
          )
        );
      }
      this.finishPuzzleIfNeeded(state);
      if (this.shouldEndTurnOnGuessAttempt(state) && state.status === "playing") {
        events.push(...this.advanceTurn(state, "word-complete"));
      }
      return events;
    }
    if (command.type === "use-hint") {
      activeOnly();
      currentTurnOnly();
      const result = useHint(
        state.puzzle!,
        state.runtime!,
        state.participants,
        participantId,
        command,
        Math.random,
        state.settings.enabledPowerUps
      );
      if (!result.changed) throw new ProtocolError("HINT_REJECTED", result.error ?? "Hint rejected.");
      const solved = result.solvedWords
        .map((id) => state.puzzle!.words.find((word) => word.id === id)?.answer.toUpperCase())
        .filter(Boolean);
      this.finishPuzzleIfNeeded(state);
      const events = [
        this.event(
          state,
          "hint-used",
          `${actor.name} used ${command.hint}, scored +${result.pointsAwarded}${
            solved.length > 0 ? `, and completed ${solved.join(", ")}` : ""
          }.`,
          actor.id,
          result.pointsAwarded
        )
      ];
      const shouldEndTurnForHint =
        command.hint === "full-word" ||
        (command.hint === "random" && result.solvedWords.length > 0) ||
        (command.hint === "first-of-each" && result.solvedWords.length > 0);
      if (shouldEndTurnForHint && this.shouldEndTurnOnGuessAttempt(state) && state.status === "playing") {
        events.push(...this.advanceTurn(state, "attempt-ended"));
      }
      return events;
    }
    if (command.type === "continue") {
      if (state.status !== "puzzle-summary") {
        throw new ProtocolError("NOT_AT_SUMMARY", "The puzzle is not awaiting continuation.");
      }
      actor.continued = true;
      const waiting = state.participants.some(
        (participant) => participant.kind === "human" && participant.connected && !participant.continued
      );
      if (!waiting) {
        if (state.puzzleIndex + 1 >= state.puzzleCount) {
          state.status = "completed";
          state.puzzle = null;
          state.runtime = null;
          state.turnState = null;
          state.aiDeadlines = {};
          state.aiIntents = {};
        } else {
          state.puzzleIndex += 1;
          return this.startPuzzle(state);
        }
      }
      return [];
    }
    if (command.type === "request-skip" || command.type === "accept-skip") {
      activeOnly();
      if (!state.skipVote) state.skipVote = { requestedBy: participantId, acceptedBy: [] };
      if (!state.skipVote.acceptedBy.includes(participantId)) state.skipVote.acceptedBy.push(participantId);
      const voters = state.participants.filter(
        (participant) => participant.kind === "human" && participant.connected
      );
      if (voters.every((participant) => state.skipVote!.acceptedBy.includes(participant.id))) {
        state.runtime!.skipped = true;
        state.status = "puzzle-summary";
        state.participants.forEach((participant) => {
          participant.continued = participant.kind === "ai";
        });
        state.turnState = null;
        state.aiDeadlines = {};
        state.aiIntents = {};
        state.skipVote = null;
        return [this.event(state, "puzzle-skipped", "The puzzle was skipped by unanimous vote.")];
      }
      return [];
    }
    if (command.type === "replace-with-ai") {
      hostOnly();
      const target = state.participants.find((participant) => participant.id === command.participantId);
      if (!target || target.kind !== "human" || target.connected) {
        throw new ProtocolError("INVALID_REPLACEMENT", "Choose a disconnected human participant.");
      }
      target.kind = "ai";
      target.aiLevel = command.level;
      target.name = this.nextAiName(state, command.level);
      target.connected = true;
      target.ready = true;
      target.replaced = true;
      target.continued = state.status === "puzzle-summary";
      const credential = this.credentials[target.id];
      if (credential) credential.revoked = true;
      if (state.disconnectedParticipantId === target.id) state.disconnectedParticipantId = null;
      state.hostTransferAt = null;
      this.scheduleAi(state);
      return [
        this.event(state, "participant-replaced", `${target.name} is now computer-controlled.`, target.id)
      ];
    }
    if (command.type === "resume") {
      hostOnly();
      const missing = state.participants.some(
        (participant) => participant.kind === "human" && !participant.connected
      );
      if (missing) throw new ProtocolError("PARTICIPANT_MISSING", "A participant is still disconnected.");
      state.paused = false;
      state.pauseReason = null;
      state.disconnectedParticipantId = null;
      state.hostTransferAt = null;
      state.aiDeadlines = {};
      state.aiIntents = {};
      this.scheduleAi(state);
      return [];
    }
    if (command.type === "end-session") {
      hostOnly();
      state.status = "ended";
      state.puzzle = null;
      state.runtime = null;
      state.turnState = null;
      state.aiDeadlines = {};
      state.aiIntents = {};
      return [];
    }
    if (command.type === "rematch") {
      hostOnly();
      if (state.status !== "completed" && state.status !== "ended") {
        throw new ProtocolError("REMATCH_UNAVAILABLE", "Finish the session before rematching.");
      }
      state.status = "lobby";
      state.rosterLocked = false;
      state.sessionId = null;
      state.puzzleIndex = 0;
      state.puzzleCount = puzzleCountFor(state.settings);
      state.puzzle = null;
      state.runtime = null;
      state.turnState = null;
      state.usedBaseWords = [];
      state.aiDeadlines = {};
      state.aiIntents = {};
      state.participants.forEach((participant) => {
        participant.score = emptyScore();
        participant.hintCredits = 0;
        participant.ready = participant.kind === "ai";
        participant.continued = false;
      });
      return [];
    }
    throw new ProtocolError("UNKNOWN_COMMAND", "Unknown command.");
  }

  async alarm() {
    await this.load();
    const state = this.state;
    if (!state) return;
    const now = Date.now();
    if (this.isExpired(state, now)) {
      for (const socket of this.ctx.getWebSockets()) socket.close(4004, "Room expired");
      await this.ctx.storage.deleteAll();
      this.state = null;
      this.credentials = {};
      this.commandResults = {};
      return;
    }

    const next = structuredClone(state);
    const events: PresentationEvent[] = [];
    if (next.hostTransferAt && now >= next.hostTransferAt) {
      const successor = next.participants
        .filter((participant) => participant.kind === "human" && participant.connected)
        .sort((left, right) => left.joinedAt - right.joinedAt)[0];
      if (successor && successor.id !== next.hostId) {
        next.hostId = successor.id;
        next.hostTransferAt = null;
        events.push(this.event(next, "host-changed", `${successor.name} is now the host.`, successor.id));
      }
    }
    if (next.status === "playing" && !next.paused && next.puzzle && next.runtime) {
      if (
        this.isTurnBased(next) &&
        next.turnState?.turnEndsAt &&
        next.turnState.turnEndsAt <= now
      ) {
        events.push(...this.advanceTurn(next, "timer-expired"));
      }
      const due = next.participants.filter(
        (participant) =>
          participant.kind === "ai" &&
          (next.aiDeadlines[participant.id] ?? Number.POSITIVE_INFINITY) <= now
      );
      for (const ai of due) {
        const intent = next.aiIntents[ai.id];
        delete next.aiDeadlines[ai.id];
        delete next.aiIntents[ai.id];
        const word =
          intent?.puzzleId === next.puzzle.id &&
          !next.runtime.completedWordIds.includes(intent.wordId)
            ? next.puzzle.words.find((candidate) => candidate.id === intent.wordId)
            : undefined;
        if (!word) continue;
        if (this.isTurnBased(next) && next.turnState?.activeParticipantId !== ai.id) continue;
        const result = submitGuess(next.puzzle, next.runtime, next.participants, ai.id, word.answer);
        if (result.changed) {
          const solved = result.solvedWords
            .map((id) => next.puzzle!.words.find((candidate) => candidate.id === id)?.answer.toUpperCase())
            .filter(Boolean)
            .join(", ");
          const summary = `${ai.name} solved ${solved || word.answer.toUpperCase()} +${result.pointsAwarded}.`;
          const text = `${summary}\n${this.formatAwardBreakdown(result.scoreAwarded)}`;
          events.push(
            this.event(
              next,
              "word-solved",
              text,
              ai.id,
              result.pointsAwarded
            )
          );
        }
        if (this.shouldEndTurnOnGuessAttempt(next) && next.status === "playing") {
          events.push(...this.advanceTurn(next, "word-complete"));
        }
        this.finishPuzzleIfNeeded(next);
      }
      this.scheduleAi(next);
    }
    this.touch(next);
    await this.persist(next);
    await this.scheduleAlarm();
    await this.broadcast(events);
  }

  private startPuzzle(state: RoomState): PresentationEvent[] {
    const seed = `${state.sessionId}:${state.puzzleIndex}:${crypto.randomUUID()}`;
    const used = new Set(state.usedBaseWords);
    state.puzzle = generateMultiplayerPuzzle(
      WORD_DATA,
      state.settings.mode,
      state.settings.difficulty,
      seed,
      state.puzzleIndex,
      state.puzzleCount,
      used
    );
    state.usedBaseWords = [...used];
    state.runtime = createPuzzleRuntime(state.puzzle);
    state.status = "playing";
    state.paused = false;
    state.pauseReason = null;
    state.disconnectedParticipantId = null;
    state.skipVote = null;
    state.participants.forEach((participant) => {
      participant.continued = participant.kind === "ai";
    });
    const events: PresentationEvent[] = [];
    if (this.isTurnBased(state)) {
      state.turnState = this.createInitialTurnState(state);
      events.push(this.createTurnOrderEvent(state, `Round ${state.puzzleIndex + 1} turn order`));
    } else {
      state.turnState = null;
    }
    this.scheduleAi(state);
    return events;
  }

  private finishPuzzleIfNeeded(state: RoomState) {
    if (!state.puzzle || !state.runtime || !puzzleIsComplete(state.puzzle, state.runtime)) return;
    state.status = "puzzle-summary";
    state.turnState = null;
    state.aiDeadlines = {};
    state.aiIntents = {};
    state.participants.forEach((participant) => {
      participant.continued = participant.kind === "ai";
    });
  }

  private isTurnBased(state: RoomState) {
    return state.settings.playMode === "Turn Based";
  }

  private shouldEndTurnOnGuessAttempt(state: RoomState) {
    return this.isTurnBased(state);
  }

  private createInitialTurnState(state: RoomState): TurnState {
    const orderedBySeat = [...state.participants]
      .sort((left, right) => left.seat - right.seat)
      .map((participant) => participant.id);
    const starter =
      state.puzzleIndex === 0
        ? orderedBySeat[Math.floor(Math.random() * orderedBySeat.length)]
        : this.pickLowestScoreStarter(state);
    const turnOrder = this.turnOrderStartingFrom(orderedBySeat, starter);
    return this.buildTurnState(turnOrder, state.puzzleIndex + 1, 0, Date.now(), state.settings.turnTimeLimit);
  }

  private turnOrderStartingFrom(orderBySeat: string[], starterId: string) {
    const startIndex = Math.max(0, orderBySeat.indexOf(starterId));
    return [...orderBySeat.slice(startIndex), ...orderBySeat.slice(0, startIndex)];
  }

  private pickLowestScoreStarter(state: RoomState) {
    const bySeat = [...state.participants].sort((left, right) => left.seat - right.seat);
    return bySeat.reduce((lowest, candidate) => {
      if (!lowest) return candidate;
      if (candidate.score.total !== lowest.score.total) {
        return candidate.score.total < lowest.score.total ? candidate : lowest;
      }
      return candidate.seat < lowest.seat ? candidate : lowest;
    }, bySeat[0])?.id ?? bySeat[0]?.id ?? "";
  }

  private buildTurnState(
    turnOrder: string[],
    roundNumber: number,
    currentTurnIndex: number,
    startedAt: number,
    turnTimeLimit: TurnTimeLimit
  ): TurnState {
    const duration = turnDurationMs(turnTimeLimit);
    const idx = Math.max(0, Math.min(currentTurnIndex, Math.max(0, turnOrder.length - 1)));
    return {
      roundNumber,
      turnOrder,
      currentTurnIndex: idx,
      turnsTakenInRound: 0,
      activeParticipantId: turnOrder[idx],
      turnStartedAt: startedAt,
      turnEndsAt: duration ? startedAt + duration : null
    };
  }

  private createTurnOrderEvent(state: RoomState, prefix: string) {
    const turnState = state.turnState;
    if (!turnState) return this.event(state, "turn-order", `${prefix}: unavailable.`);
    const names = turnState.turnOrder
      .map((id) => state.participants.find((participant) => participant.id === id)?.name ?? "Unknown")
      .join(" -> ");
    return this.event(state, "turn-order", `${prefix}: ${names}.`);
  }

  private formatAwardBreakdown(score: ScoreBreakdown) {
    return [
      `Letters: ${score.letters}`,
      `💚: ${score.emerald}`,
      `♦️: ${score.ruby}`,
      `💎: ${score.diamond}`
    ].join("\n");
  }

  private advanceTurn(state: RoomState, reason: "timer-expired" | "word-complete" | "attempt-ended") {
    if (!this.isTurnBased(state) || !state.turnState) return [] as PresentationEvent[];
    const current = state.turnState;
    const currentParticipant =
      state.participants.find((participant) => participant.id === current.activeParticipantId)?.name ?? "Player";
    const turnOrder = current.turnOrder;
    const roundNumber = current.roundNumber;
    let turnsTaken = current.turnsTakenInRound + 1;
    const nextIndex = (current.currentTurnIndex + 1) % turnOrder.length;
    const events: PresentationEvent[] = [];
    if (nextIndex === 0) turnsTaken = 0;

    const startedAt = Date.now();
    const duration = turnDurationMs(state.settings.turnTimeLimit);
    const activeParticipantId = turnOrder[nextIndex];
    state.turnState = {
      roundNumber,
      turnOrder,
      currentTurnIndex: nextIndex,
      turnsTakenInRound: turnsTaken,
      activeParticipantId,
      turnStartedAt: startedAt,
      turnEndsAt: duration ? startedAt + duration : null
    };
    const nextParticipant =
      state.participants.find((participant) => participant.id === activeParticipantId)?.name ??
      "Player";
    const reasonText =
      reason === "timer-expired"
        ? `${currentParticipant}'s timer expired.`
        : reason === "attempt-ended"
          ? `${currentParticipant}'s turn ended after their attempt.`
          : `${currentParticipant}'s turn ended.`;
    events.push(this.event(state, "turn-advanced", `${reasonText} ${nextParticipant} is up next.`));
    state.aiDeadlines = {};
    state.aiIntents = {};
    this.scheduleAi(state);
    return events;
  }

  private scheduleAi(state: RoomState) {
    if (state.status !== "playing" || state.paused || !state.puzzle || !state.runtime) return;
    const now = Date.now();
    if (this.isTurnBased(state) && state.turnState) {
      state.aiDeadlines = {};
      state.aiIntents = {};
      const active = state.participants.find((participant) => participant.id === state.turnState!.activeParticipantId);
      if (!active || active.kind !== "ai") return;
      const incomplete = state.puzzle.words.filter((word) => !state.runtime!.completedWordIds.includes(word.id));
      if (incomplete.length === 0) return;
      const level = active.aiLevel ?? "College";
      const preferred =
        level === "High School"
          ? incomplete.filter((word) => word.rarity <= 2 && word.answer.length <= 5)
          : level === "College"
            ? incomplete.filter((word) => word.rarity <= 3)
            : incomplete;
      const candidates = preferred.length > 0 ? preferred : incomplete;
      const word = candidates[Math.floor(Math.random() * candidates.length)];
      const softDelay = aiDelay(level);
      const turnRemaining = state.turnState.turnEndsAt
        ? Math.max(500, state.turnState.turnEndsAt - now - 500)
        : softDelay;
      const at = now + Math.min(softDelay, turnRemaining);
      state.aiDeadlines[active.id] = at;
      state.aiIntents[active.id] = { at, puzzleId: state.puzzle.id, type: "submit-guess", wordId: word.id };
      return;
    }
    for (const participant of state.participants) {
      if (participant.kind === "ai" && !state.aiDeadlines[participant.id]) {
        const incomplete = state.puzzle.words.filter(
          (word) => !state.runtime!.completedWordIds.includes(word.id)
        );
        if (incomplete.length === 0) continue;
        const level = participant.aiLevel ?? "College";
        const preferred =
          level === "High School"
            ? incomplete.filter((word) => word.rarity <= 2 && word.answer.length <= 5)
            : level === "College"
              ? incomplete.filter((word) => word.rarity <= 3)
              : incomplete;
        const candidates = preferred.length > 0 ? preferred : incomplete;
        const word = candidates[Math.floor(Math.random() * candidates.length)];
        const at = now + aiDelay(level);
        state.aiDeadlines[participant.id] = at;
        state.aiIntents[participant.id] = {
          at,
          puzzleId: state.puzzle.id,
          type: "submit-guess",
          wordId: word.id
        };
      }
    }
  }

  private async scheduleAlarm() {
    const state = this.state;
    if (!state) return;
    const expiry =
      state.updatedAt +
      (state.status === "completed" || state.status === "ended" ? COMPLETED_EXPIRY_MS : ACTIVE_EXPIRY_MS);
    const deadlines = [
      expiry,
      state.hostTransferAt ?? Number.POSITIVE_INFINITY,
      state.turnState?.turnEndsAt ?? Number.POSITIVE_INFINITY,
      ...Object.values(state.aiDeadlines)
    ].filter((deadline) => Number.isFinite(deadline) && deadline > Date.now());
    if (deadlines.length > 0) await this.ctx.storage.setAlarm(Math.min(...deadlines));
  }

  private snapshotFor(state: RoomState, localParticipantId: string): RoomSnapshot {
    const completed = new Set(state.runtime?.completedWordIds ?? []);
    const rarityGem = (rarity: number): GemType =>
      rarity >= 4 ? "diamond" : rarity === 3 ? "ruby" : rarity === 2 ? "emerald" : "none";
    const normalizeWordGems = (word: { answer: string; rarity: number; gems?: GemType[] }): GemType[] =>
      Array.isArray(word.gems) && word.gems.length > 0
        ? word.gems
        : Array.from({ length: word.answer.length }, () => rarityGem(word.rarity));
    return {
      protocolVersion: MULTIPLAYER_PROTOCOL_VERSION,
      roomCode: state.code,
      revision: state.revision,
      eventSequence: state.eventSequence,
      status: state.status,
      hostId: state.hostId,
      localParticipantId,
      settings: state.settings,
      rosterLocked: state.rosterLocked,
      sessionId: state.sessionId,
      puzzleIndex: state.puzzleIndex,
      puzzleCount: state.puzzleCount,
      participants: structuredClone(state.participants),
      puzzle:
        state.puzzle && state.runtime
          ? {
              id: state.puzzle.id,
              mode: state.puzzle.mode,
              baseLetters: state.puzzle.baseLetters,
              rows: state.puzzle.rows,
              cols: state.puzzle.cols,
              words: state.puzzle.words.map((word) => ({
                id: word.id,
                length: word.answer.length,
                rarity: word.rarity,
                gems: normalizeWordGems(word),
                cells: word.cells,
                completed: completed.has(word.id)
              })),
              visibleCells: structuredClone(state.runtime.visibleCells),
              bonusWordCount: state.puzzle.bonusWords.length,
              claimedBonusCount: Object.keys(state.runtime.claimedBonusWords).length,
              claimedBonusWords: Object.keys(state.runtime.claimedBonusWords).sort(
                (left, right) => left.localeCompare(right)
              ),
              failedWords: [...(state.runtime.failedGuesses ?? [])]
                .map((word) => word.toUpperCase())
                .sort((left, right) => left.localeCompare(right)),
              skipped: state.runtime.skipped
            }
          : null,
      turnState: state.turnState ? structuredClone(state.turnState) : null,
      paused: state.paused,
      pauseReason: state.pauseReason,
      disconnectedParticipantId: state.disconnectedParticipantId,
      skipVote: state.skipVote
    };
  }

  private event(
    state: RoomState,
    type: PresentationEvent["type"],
    text: string,
    actorId?: string,
    points?: number
  ): PresentationEvent {
    const event: PresentationEvent = {
      sequence: ++state.eventSequence,
      type,
      actorId,
      text,
      points,
      at: Date.now()
    };
    state.events.push(event);
    if (state.events.length > 50) state.events.splice(0, state.events.length - 50);
    return event;
  }

  private async broadcast(events: PresentationEvent[], excludedConnectionId?: string) {
    const state = this.state;
    if (!state) return;
    for (const socket of this.ctx.getWebSockets()) {
      const attachment = socket.deserializeAttachment() as SocketAttachment | null;
      if (!attachment || attachment.connectionId === excludedConnectionId) continue;
      try {
        socket.send(
          JSON.stringify({
            type: "snapshot",
            snapshot: this.snapshotFor(state, attachment.participantId),
            events
          } satisfies ServerMessage)
        );
      } catch {
        // The close/error callback updates authoritative connection state.
      }
    }
  }

  private async authenticate(participantId: string, token: string) {
    const credential = this.credentials[participantId];
    if (!credential || credential.revoked) {
      throw new ProtocolError("INVALID_CREDENTIAL", "Your room credential is no longer valid.", 403);
    }
    const suppliedHash = await hashToken(token);
    if (!constantTimeEqual(credential.tokenHash, suppliedHash)) {
      throw new ProtocolError("INVALID_CREDENTIAL", "Your room credential is invalid.", 403);
    }
  }

  private assertCurrentCommand(state: RoomState, command: ClientCommand) {
    const sessionCommands = new Set([
      "submit-guess",
      "use-hint",
      "continue",
      "request-skip",
      "accept-skip"
    ]);
    if (sessionCommands.has(command.type) && command.sessionId !== state.sessionId) {
      throw new ProtocolError("STALE_SESSION", "This command belongs to an older session.");
    }
    if (
      (command.type === "submit-guess" || command.type === "use-hint") &&
      command.puzzleId !== state.puzzle?.id
    ) {
      throw new ProtocolError("STALE_PUZZLE", "This command belongs to an older puzzle.");
    }
  }

  private createHuman(id: string, name: string, seat: number, joinedAt: number): Participant {
    return {
      id,
      name,
      color: COLORS[seat],
      seat,
      kind: "human",
      connected: false,
      ready: false,
      replaced: false,
      joinedAt,
      hintCredits: 0,
      score: emptyScore(),
      continued: false
    };
  }

  private createAi(state: RoomState, level: AiLevel, seat: number): Participant {
    return {
      id: randomId("ai"),
      name: this.nextAiName(state, level),
      color: COLORS[seat],
      seat,
      kind: "ai",
      aiLevel: level,
      connected: true,
      ready: true,
      replaced: false,
      joinedAt: Date.now(),
      hintCredits: 0,
      score: emptyScore(),
      continued: true
    };
  }

  private nextAiName(state: RoomState, level: AiLevel) {
    const used = new Set(state.participants.map((participant) => participant.name.toLowerCase()));
    for (const baseName of AI_NAMES) {
      const candidate = `${baseName} AI`;
      if (!used.has(candidate.toLowerCase())) return candidate;
    }
    let suffix = 2;
    while (suffix < 100) {
      for (const baseName of AI_NAMES) {
        const candidate = `${baseName} AI ${suffix}`;
        if (!used.has(candidate.toLowerCase())) return candidate;
      }
      suffix += 1;
    }
    return `${level} AI`;
  }

  private clearHumanReadiness(state: RoomState) {
    state.participants.forEach((participant) => {
      if (participant.kind === "human") participant.ready = false;
    });
  }

  private openSeat(state: RoomState) {
    for (let seat = 0; seat < state.settings.capacity; seat += 1) {
      if (!state.participants.some((participant) => participant.seat === seat)) return seat;
    }
    throw new ProtocolError("ROOM_FULL", "This room is full.");
  }

  private requireState() {
    if (!this.state) throw new ProtocolError("ROOM_NOT_FOUND", "This room does not exist or expired.", 404);
    return this.state;
  }

  private touch(state: RoomState) {
    state.revision += 1;
    state.updatedAt = Date.now();
  }

  private isExpired(state: RoomState, now = Date.now()) {
    const lifetime =
      state.status === "completed" || state.status === "ended" ? COMPLETED_EXPIRY_MS : ACTIVE_EXPIRY_MS;
    return now - state.updatedAt > lifetime;
  }

  private assertNotExpired(state: RoomState) {
    if (this.isExpired(state)) throw new ProtocolError("ROOM_EXPIRED", "This room has expired.", 410);
  }

  private pruneCommandResults() {
    const cutoff = Date.now() - COMMAND_RETENTION_MS;
    for (const [key, result] of Object.entries(this.commandResults)) {
      if (result.at < cutoff) delete this.commandResults[key];
    }
  }

  private async persist(state: RoomState, credentialsChanged = false, commandsChanged = false) {
    const values: Record<string, unknown> = { state };
    if (credentialsChanged) values.credentials = this.credentials;
    if (commandsChanged) values.commandResults = this.commandResults;
    await this.ctx.storage.put(values);
    this.state = state;
  }

  private sendError(socket: WebSocket, error: unknown) {
    const protocolError =
      error instanceof ProtocolError
        ? error
        : new ProtocolError("SERVER_ERROR", "The room could not process that command.", 500);
    socket.send(
      JSON.stringify({
        type: "error",
        code: protocolError.code,
        message: protocolError.message,
        recoverable: protocolError.status < 500
      } satisfies ServerMessage)
    );
  }
}

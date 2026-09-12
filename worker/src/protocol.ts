import {
  MULTIPLAYER_PROTOCOL_VERSION,
  type AiLevel,
  type ClientCommand,
  type CreateRoomRequest,
  type Difficulty,
  type GameMode,
  type JoinRoomRequest,
  type RoomSettings
} from "../../shared/multiplayer/types";

const MAX_MESSAGE_BYTES = 4_096;
const AI_LEVELS: AiLevel[] = ["High School", "College", "Professional"];
const MODES: GameMode[] = ["Casual", "Crossword"];
const DIFFICULTIES: Difficulty[] = ["Easy", "Medium", "Hard"];

export class ProtocolError extends Error {
  constructor(
    readonly code: string,
    message: string,
    readonly status = 400
  ) {
    super(message);
  }
}

export const normalizeRoomCode = (value: string) => value.trim().toUpperCase().replace(/[^A-Z2-9]/g, "");

export const validateName = (value: unknown) => {
  if (typeof value !== "string") throw new ProtocolError("INVALID_NAME", "Enter a player name.");
  const normalized = value.trim().replace(/\s+/g, " ");
  if (normalized.length < 1 || normalized.length > 24) {
    throw new ProtocolError("INVALID_NAME", "Player names must be 1–24 characters.");
  }
  return normalized;
};

export const validateSettings = (value: unknown): RoomSettings => {
  if (!value || typeof value !== "object") {
    throw new ProtocolError("INVALID_SETTINGS", "Room settings are required.");
  }
  const candidate = value as Partial<RoomSettings>;
  if (!MODES.includes(candidate.mode as GameMode)) {
    throw new ProtocolError("INVALID_SETTINGS", "Choose Casual or Crossword.");
  }
  if (!DIFFICULTIES.includes(candidate.difficulty as Difficulty)) {
    throw new ProtocolError("INVALID_SETTINGS", "Choose Easy, Medium, or Hard.");
  }
  if (![1, 2, 3, 4].includes(candidate.capacity ?? 0)) {
    throw new ProtocolError("INVALID_SETTINGS", "Capacity must be from one to four.");
  }
  return candidate as RoomSettings;
};

export const validateCreateRequest = (value: unknown): CreateRoomRequest => {
  if (!value || typeof value !== "object") throw new ProtocolError("INVALID_REQUEST", "Invalid request.");
  const candidate = value as Partial<CreateRoomRequest>;
  return { name: validateName(candidate.name), settings: validateSettings(candidate.settings) };
};

export const validateJoinRequest = (value: unknown): JoinRoomRequest => {
  if (!value || typeof value !== "object") throw new ProtocolError("INVALID_REQUEST", "Invalid request.");
  return { name: validateName((value as Partial<JoinRoomRequest>).name) };
};

export const parseCommand = (message: string | ArrayBuffer): ClientCommand => {
  const bytes = typeof message === "string" ? new TextEncoder().encode(message).byteLength : message.byteLength;
  if (bytes > MAX_MESSAGE_BYTES) throw new ProtocolError("MESSAGE_TOO_LARGE", "Command is too large.");
  let value: unknown;
  try {
    value = JSON.parse(typeof message === "string" ? message : new TextDecoder().decode(message));
  } catch {
    throw new ProtocolError("INVALID_JSON", "Command must be valid JSON.");
  }
  if (!value || typeof value !== "object") throw new ProtocolError("INVALID_COMMAND", "Invalid command.");
  const command = value as Partial<ClientCommand>;
  if (command.protocolVersion !== MULTIPLAYER_PROTOCOL_VERSION) {
    throw new ProtocolError("PROTOCOL_MISMATCH", "Please refresh to update the game.");
  }
  if (typeof command.commandId !== "string" || command.commandId.length < 8 || command.commandId.length > 80) {
    throw new ProtocolError("INVALID_COMMAND_ID", "Invalid command identifier.");
  }
  if (typeof command.type !== "string") throw new ProtocolError("INVALID_COMMAND", "Command type is required.");
  if ("level" in command && !AI_LEVELS.includes(command.level as AiLevel)) {
    throw new ProtocolError("INVALID_AI_LEVEL", "Unknown AI level.");
  }
  return command as ClientCommand;
};

export const parseJsonBody = async (request: Request) => {
  const length = Number(request.headers.get("content-length") ?? "0");
  if (length > MAX_MESSAGE_BYTES) throw new ProtocolError("REQUEST_TOO_LARGE", "Request is too large.", 413);
  return request.json() as Promise<unknown>;
};

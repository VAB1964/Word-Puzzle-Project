export const MULTIPLAYER_PROTOCOL_VERSION = 1;

export type GameMode = "Casual" | "Crossword";
export type PlayMode = "Free for All" | "Turn Based";
export type TurnTimeLimit = "Not Timed" | 30 | 25 | 20 | 15 | 10;
export type Difficulty = "Easy" | "Medium" | "Hard";
export type AiLevel = "High School" | "College" | "Professional";
export type GemType = "none" | "diamond" | "ruby" | "emerald";
export type RoomStatus = "lobby" | "playing" | "puzzle-summary" | "completed" | "ended";
export type PauseReason = "participant-disconnected" | "host-disconnected" | null;
export type HintKind = "letter" | "random" | "full-word" | "first-of-each";

export interface ScoreBreakdown {
  letters: number;
  emerald: number;
  diamond: number;
  ruby: number;
  total: number;
}

export interface Participant {
  id: string;
  name: string;
  color: string;
  seat: number;
  kind: "human" | "ai";
  aiLevel?: AiLevel;
  connected: boolean;
  ready: boolean;
  replaced: boolean;
  joinedAt: number;
  hintCredits: number;
  score: ScoreBreakdown;
  continued: boolean;
}

export interface RoomSettings {
  mode: GameMode;
  playMode: PlayMode;
  turnTimeLimit: TurnTimeLimit;
  difficulty: Difficulty;
  capacity: 1 | 2 | 3 | 4;
}

export interface TurnState {
  roundNumber: number;
  turnOrder: string[];
  currentTurnIndex: number;
  turnsTakenInRound: number;
  activeParticipantId: string;
  turnStartedAt: number;
  turnEndsAt: number | null;
}

export interface PuzzleCellRef {
  row: number;
  col: number;
}

export interface PuzzleWordDefinition {
  id: string;
  answer: string;
  rarity: number;
  gems: GemType[];
  cells: PuzzleCellRef[];
}

export interface PuzzleDefinition {
  id: string;
  mode: GameMode;
  baseLetters: string;
  rows: number;
  cols: number;
  words: PuzzleWordDefinition[];
  bonusWords: string[];
}

export interface VisibleCell {
  letter: string;
  ownerId: string;
}

export interface PositionCredit {
  ownerId: string;
  base: number;
  gem: GemType;
  bonus: number;
}

export interface PuzzleRuntime {
  visibleCells: Record<string, VisibleCell>;
  completedWordIds: string[];
  credits: Record<string, Array<PositionCredit | null>>;
  claimedBonusWords: Record<string, string>;
  skipped: boolean;
}

export interface PublicPuzzleWord {
  id: string;
  length: number;
  rarity: number;
  gems: GemType[];
  cells: PuzzleCellRef[];
  completed: boolean;
}

export interface PublicPuzzle {
  id: string;
  mode: GameMode;
  baseLetters: string;
  rows: number;
  cols: number;
  words: PublicPuzzleWord[];
  visibleCells: Record<string, VisibleCell>;
  bonusWordCount: number;
  claimedBonusCount: number;
  claimedBonusWords: string[];
  skipped: boolean;
}

export interface PresentationEvent {
  sequence: number;
  type:
    | "word-solved"
    | "bonus-claimed"
    | "hint-used"
    | "participant-disconnected"
    | "participant-returned"
    | "participant-replaced"
    | "host-changed"
    | "puzzle-skipped"
    | "guess-rejected"
    | "turn-order"
    | "turn-advanced";
  actorId?: string;
  text: string;
  points?: number;
  at: number;
}

export interface SkipVote {
  requestedBy: string;
  acceptedBy: string[];
}

export interface AiIntent {
  at: number;
  puzzleId: string;
  type: "submit-guess";
  wordId: string;
}

export interface RoomState {
  code: string;
  revision: number;
  eventSequence: number;
  status: RoomStatus;
  createdAt: number;
  updatedAt: number;
  hostId: string;
  settings: RoomSettings;
  rosterLocked: boolean;
  sessionId: string | null;
  puzzleIndex: number;
  puzzleCount: number;
  participants: Participant[];
  puzzle: PuzzleDefinition | null;
  runtime: PuzzleRuntime | null;
  turnState: TurnState | null;
  usedBaseWords: string[];
  aiDeadlines: Record<string, number>;
  aiIntents: Record<string, AiIntent>;
  events: PresentationEvent[];
  paused: boolean;
  pauseReason: PauseReason;
  disconnectedParticipantId: string | null;
  hostTransferAt: number | null;
  skipVote: SkipVote | null;
}

export interface RoomSnapshot {
  protocolVersion: number;
  roomCode: string;
  revision: number;
  eventSequence: number;
  status: RoomStatus;
  hostId: string;
  localParticipantId: string;
  settings: RoomSettings;
  rosterLocked: boolean;
  sessionId: string | null;
  puzzleIndex: number;
  puzzleCount: number;
  participants: Participant[];
  puzzle: PublicPuzzle | null;
  turnState: TurnState | null;
  paused: boolean;
  pauseReason: PauseReason;
  disconnectedParticipantId: string | null;
  skipVote: SkipVote | null;
}

export interface CommandBase {
  protocolVersion: number;
  commandId: string;
  sessionId?: string | null;
  puzzleId?: string | null;
}

export type ClientCommand =
  | (CommandBase & { type: "set-ready"; ready: boolean })
  | (CommandBase & { type: "update-settings"; settings: RoomSettings; expectedRevision: number })
  | (CommandBase & { type: "add-ai"; level: AiLevel })
  | (CommandBase & { type: "remove-ai"; participantId: string })
  | (CommandBase & { type: "start-session" })
  | (CommandBase & { type: "submit-guess"; guess: string })
  | (CommandBase & { type: "use-hint"; hint: HintKind; wordId?: string; position?: number })
  | (CommandBase & { type: "continue" })
  | (CommandBase & { type: "request-skip" })
  | (CommandBase & { type: "accept-skip" })
  | (CommandBase & { type: "replace-with-ai"; participantId: string; level: AiLevel })
  | (CommandBase & { type: "resume" })
  | (CommandBase & { type: "end-session" })
  | (CommandBase & { type: "rematch" })
  | (CommandBase & { type: "request-snapshot" })
  | (CommandBase & { type: "ping" });

export type ServerMessage =
  | { type: "welcome"; snapshot: RoomSnapshot; reconnectToken: string; events: PresentationEvent[] }
  | { type: "snapshot"; snapshot: RoomSnapshot; events: PresentationEvent[] }
  | { type: "pong"; at: number }
  | {
      type: "error";
      code: string;
      message: string;
      commandId?: string;
      recoverable: boolean;
    };

export interface CreateRoomRequest {
  name: string;
  settings: RoomSettings;
}

export interface CreateRoomResponse {
  code: string;
  participantId: string;
  reconnectToken: string;
  wsUrl: string;
}

export interface JoinRoomRequest {
  name: string;
}

export interface JoinRoomResponse extends CreateRoomResponse {}

export interface ReconnectRecord {
  participantId: string;
  tokenHash: string;
  revoked: boolean;
}

export interface StoredCommandResult {
  participantId: string;
  commandId: string;
  revision: number;
  message: ServerMessage;
  at: number;
}

import { useEffect, useMemo, useRef, useState } from "react";
import type { ReactNode } from "react";
import { configureGameAudio, playGameSound, unlockGameAudio } from "../audio/gameAudio";
import { playScriptedDialogue } from "../audio/scriptedDialogue";
import { AVATARS } from "../identity/preferences";
import type { PlayerPreferences } from "../identity/preferences";
import { cardValue, isCard, type Card } from "../rules/cards";
import { scoreHand } from "../rules/handScoring";
import type { MultiplayerSnapshot, RoomMessage } from "../controllers/MultiplayerController";
import type { CommandType, ConnectionState } from "../multiplayer/protocol";

type Player = {
  id: string; name: string; avatarId?: string; seat: number | null; teamId?: string | null;
  connected?: boolean; isAI?: boolean; ready?: boolean; score?: number; hand?: Card[]; handCount?: number;
};
type Event = { id?: string; type: string; data: Record<string, unknown> };
type PegColor = "red" | "green" | "blue";
type Props = {
  view: MultiplayerSnapshot; playerId: string; preferences: PlayerPreferences; connection: ConnectionState; message?: RoomMessage;
  send: (type: CommandType, payload: unknown) => void; onLeave: () => void;
};

const object = (value: unknown): Record<string, unknown> | null => value && typeof value === "object" && !Array.isArray(value) ? value as Record<string, unknown> : null;
const number = (value: unknown, fallback = 0) => typeof value === "number" && Number.isFinite(value) ? value : fallback;
const text = (value: unknown, fallback = "") => typeof value === "string" ? value : fallback;
const parseCard = (value: unknown): Card | null => {
  if (isCard(value)) return value;
  if (typeof value !== "string") return null;
  const match = /^(A|[2-9]|10|J|Q|K)([CDHS])$/.exec(value);
  if (!match) return null;
  const rank = ({ A: 1, J: 11, Q: 12, K: 13 } as Record<string, number>)[match[1]] ?? Number(match[1]);
  const suit = ({ C: "clubs", D: "diamonds", H: "hearts", S: "spades" } as const)[match[2] as "C" | "D" | "H" | "S"];
  return { rank: rank as Card["rank"], suit, id: `${rank}-${suit}` };
};
const encodeCard = (card: Card): string => `${rank(card.rank)}${({ clubs: "C", diamonds: "D", hearts: "H", spades: "S" } as const)[card.suit]}`;
const cards = (value: unknown): Card[] => Array.isArray(value) ? value.flatMap(item => {
  const candidate = object(item)?.card ?? item;
  const parsed = parseCard(candidate);
  return parsed ? [parsed] : [];
}) : [];
const phaseName = (view: MultiplayerSnapshot) => text(view.phase ?? object(view.game)?.phase, "lobby").toLowerCase();
const game = (view: MultiplayerSnapshot) => object(view.game) ?? view;
const playersFrom = (view: MultiplayerSnapshot): Player[] => {
  const raw = view.players ?? game(view).players;
  if (!Array.isArray(raw)) return [];
  return raw.flatMap(value => {
    const item = object(value);
    if (!item || typeof item.id !== "string") return [];
    const hand = cards(item.hand ?? item.cards);
    return [{ id: item.id, name: text(item.name, "Player"), avatarId: text(item.avatarId), seat: typeof item.seat === "number" ? item.seat : null,
      teamId: typeof item.teamId === "string" ? item.teamId : null, connected: item.connected !== false, isAI: item.isAI === true,
      ready: item.ready === true, score: number(item.score), hand, handCount: number(item.handCount ?? item.cardCount, hand.length) }];
  });
};
const eventKey = (event: Event, revision: number) => event.id ?? `${revision}:${event.type}:${JSON.stringify(event.data)}`;
const titlePhase = (phase: string) => ({ cut: "Cut for deal", cutting: "Cut for deal", discard: "Choose the crib", pegging: "Pegging", counting: "Counting", dealcomplete: "Deal complete", result: "Game result", complete: "Game result", session_summary: "Session summary", summary: "Session summary" }[phase] ?? phase.replaceAll("_", " "));
const rank = (value: number) => value === 1 ? "A" : value === 11 ? "J" : value === 12 ? "Q" : value === 13 ? "K" : String(value);
const suit = (value: Card["suit"]) => ({ clubs: "♣", diamonds: "♦", hearts: "♥", spades: "♠" }[value]);

function PlayingCard({ card, hidden, selected, disabled, onClick }: { card?: Card; hidden?: boolean; selected?: boolean; disabled?: boolean; onClick?: () => void }) {
  if (hidden || !card) return <span className="mp-card back" aria-label="Hidden card" />;
  const warm = card.suit === "diamonds" || card.suit === "hearts";
  return <button type="button" className={`mp-card ${warm ? "warm" : ""} ${selected ? "selected" : ""}`} disabled={disabled} onClick={onClick} aria-pressed={selected}>
    <b>{rank(card.rank)}</b><span>{suit(card.suit)}</span>
  </button>;
}

function CardNotation({ value }: { value: string }) {
  const parts: ReactNode[] = [];
  const pattern = /(10|[2-9AJQK])([CDHS])/g;
  let cursor = 0;
  for (const match of value.matchAll(pattern)) {
    const index = match.index;
    if (index > cursor) parts.push(value.slice(cursor, index));
    const suitCode = match[2] as "C" | "D" | "H" | "S";
    const suitGlyph = ({ C: "♣", D: "♦", H: "♥", S: "♠" } as const)[suitCode];
    parts.push(<span className={`mp-card-notation ${suitCode === "D" || suitCode === "H" ? "warm" : ""}`} key={`${index}-${match[0]}`}>{match[1]}{suitGlyph}</span>);
    cursor = index + match[0].length;
  }
  if (cursor < value.length) parts.push(value.slice(cursor));
  return <>{parts}</>;
}

function CribbageBoard({ lanes, moves }: {
  lanes: Array<{ id: string; label: string; score: number; color: PegColor }>;
  moves: Record<string, { from: number; to: number; amount: number }>;
}) {
  const winner = lanes.find(lane => lane.score >= 121);
  return <section className="mp-cribbage-board" aria-label="Cribbage scoreboard">
    {lanes.map(lane => <div className={`mp-board-lane ${lane.color}`} key={lane.id} aria-label={`${lane.label}: ${lane.score} points`}>
      <strong>{lane.label}</strong>
      <i className={`mp-end-hole ${lane.score === 0 ? `pegged ${lane.color}` : ""} ${moves[lane.id]?.from === 0 ? "has-ghost" : ""}`} aria-label="Start hole"><span /></i>
      <div className="mp-hole-track" aria-hidden="true">{Array.from({ length: 24 }, (_, groupIndex) => {
        const groupEnd = (groupIndex + 1) * 5;
        return <div className={`mp-hole-group ${groupEnd === 60 ? "double-skunk-line" : ""} ${groupEnd === 90 ? "skunk-line" : ""}`} key={groupIndex}>
          {Array.from({ length: 5 }, (_, offset) => {
            const hole = groupIndex * 5 + offset + 1;
            const move = moves[lane.id];
            const inTrail = Boolean(move && hole > move.from && hole <= move.to);
            return <i key={hole} className={`${hole === lane.score ? `pegged ${lane.color}` : ""} ${inTrail ? `score-trail ${lane.color}` : ""} ${move?.from === hole ? "has-ghost" : ""}`}>
              <span />{hole === lane.score && move?.amount ? <b className="mp-score-jump">+{move.amount}</b> : null}
            </i>;
          })}
        </div>;
      })}</div>
      <b>{lane.score}</b>
    </div>)}
    <div className={`mp-finish ${winner?.color ?? ""}`}><small>Finish</small><i className={`mp-end-hole ${winner ? `pegged ${winner.color}` : ""}`}><span /></i></div>
  </section>;
}

function CountReveal({ title, hand, starter, isCrib, points, canContinue, waiting, onContinue }: {
  title: string; hand: Card[]; starter?: Card; isCrib: boolean; points: number;
  canContinue: boolean; waiting: boolean; onContinue: () => void;
}) {
  const score = hand.length === 4 && starter ? scoreHand(hand, starter, isCrib) : null;
  const describe = (event: NonNullable<typeof score>["events"][number]) =>
    `${event.category === "fifteen" ? "Fifteen" : event.category[0].toUpperCase() + event.category.slice(1)}: ${event.cards.map(card => `${rank(card.rank)}${suit(card.suit)}`).join(" + ")} — ${event.points}`;
  return <div className="mp-count-modal" role="dialog" aria-modal="true" aria-labelledby="mp-count-title">
    <section>
      <span className="eyebrow">{isCrib ? "Crib count" : "Hand count"}</span>
      <h2 id="mp-count-title">{title}</h2>
      <div className="mp-count-cards">{hand.map(card => <PlayingCard card={card} key={card.id} disabled />)}
        {starter && <div className="mp-starter-count"><small>Starter</small><PlayingCard card={starter} disabled /></div>}
      </div>
      <div className="mp-count-breakdown">
        <h3>{points} points</h3>
        {score?.events.length ? <ul>{score.events.map((event, index) => <li key={`${event.category}-${index}`}>{describe(event)}</li>)}</ul> : <p>No scoring combinations.</p>}
      </div>
      <button className="primary" disabled={!canContinue || waiting} onClick={onContinue}>
        {waiting ? "Waiting for other players…" : canContinue ? "Accept count" : "Reviewing count…"}
      </button>
    </section>
  </div>;
}

export default function MultiplayerTable({ view, playerId, preferences, connection, message, send, onLeave }: Props) {
  const phase = phaseName(view);
  const state = game(view);
  const players = playersFrom(view).sort((a, b) => (a.seat ?? 99) - (b.seat ?? 99));
  const me = players.find(player => player.id === playerId);
  const [selected, setSelected] = useState<string[]>([]);
  const [countingEntryReady, setCountingEntryReady] = useState(true);
  const previousPresentationPhase = useRef(phase);
  const lastPeggingPile = useRef<Card[]>([]);
  const lastPeggingCount = useRef(0);
  const [history, setHistory] = useState<Array<{ key: string; text: string; dialogue: boolean }>>([]);
  const [playNotices, setPlayNotices] = useState<Array<{ id: string; kind: "play" | "go" | "last"; name: string; isAI: boolean; card: string; reason: string; points: number; score: number; count: number }>>([]);
  const seen = useRef(new Set<string>());
  const playNoticeSeen = useRef(new Set<string>());
  const playNoticesInitialized = useRef(false);
  const hand = cards(state.hand ?? state.localHand ?? me?.hand);
  const handCounts = object(state.handCounts);
  const teamScores = object(state.teamScores);
  for (const player of players) {
    player.handCount = number(handCounts?.[player.id], player.handCount);
    player.score = number(teamScores?.[players.length === 4 ? (player.teamId ?? "") : player.id], player.score);
  }
  const authoritativeRunningCount = number(state.runningCount ?? state.count);
  const authoritativePile = cards(state.pile ?? state.sequence ?? state.playedCards);
  if (phase === "pegging") {
    lastPeggingPile.current = authoritativePile;
    lastPeggingCount.current = authoritativeRunningCount;
  }
  const holdingFinalPeg = phase === "counting" && (!countingEntryReady || previousPresentationPhase.current === "pegging");
  const runningCount = holdingFinalPeg ? lastPeggingCount.current : phase === "counting" ? 0 : authoritativeRunningCount;
  const turnSeat = number(state.turnSeat, -1);
  const dealerSeat = number(state.dealerSeat, -1);
  const needed = number(state.discardCount ?? state.requiredDiscards, players.length === 2 ? 2 : 1);
  const legalIds = new Set(Array.isArray(state.legalCardIds) ? state.legalCardIds.filter((id): id is string => typeof id === "string") : hand.filter(card => cardValue(card) + runningCount <= 31).map(card => card.id));
  const myTurn = me?.seat === turnSeat;
  const canGo = phase === "pegging" && myTurn && hand.length > 0 && legalIds.size === 0;
  const hostId = text(view.hostPlayerId ?? state.hostPlayerId);
  const isHost = playerId === hostId;
  const host = players.find(player => player.id === hostId);
  const cut = cards(state.starterCard ? [state.starterCard] : state.cutCard ? [state.cutCard] : state.starter ? [state.starter] : [])[0];
  const pile = holdingFinalPeg ? lastPeggingPile.current : phase === "counting" ? [] : authoritativePile;
  const ledger = object(view.sessionLedger ?? state.sessionLedger ?? view.ledger);
  const winner = text(state.winnerName ?? state.winnerTeamName ?? state.winnerTeamId);
  const cutCards = object(state.cutCards);
  const events = useMemo(() => {
    const dialogue = Array.isArray(view.dialogue) ? view.dialogue.flatMap(value => {
      const item = object(value);
      if (!item || typeof item.type !== "string") return [];
      const speaker = players.find(player => player.id === item.playerId);
      const details = object(item.data);
      return [{ id: text(item.id), type: item.type, data: { ...details, playerName: speaker?.name ?? "", message: text(details?.message, item.type.replaceAll("_", " ").toLowerCase()) } as Record<string, unknown> }];
    }) : [];
    return [...dialogue, ...(message?.events ?? []), ...(message?.event ? [message.event] : [])];
  }, [message, players, view.dialogue]);

  useEffect(() => setSelected(ids => ids.filter(id => hand.some(card => card.id === id))), [view.revision]); // authoritative hand clears accepted choices
  useEffect(() => {
    const previous = previousPresentationPhase.current;
    previousPresentationPhase.current = phase;
    if (phase === "counting" && previous === "pegging") {
      setCountingEntryReady(false);
      const timer = window.setTimeout(() => setCountingEntryReady(true), 3000);
      return () => window.clearTimeout(timer);
    }
    if (phase !== "counting") setCountingEntryReady(true);
  }, [phase]);
  useEffect(() => {
    const additions: Array<{ key: string; text: string; dialogue: boolean }> = [];
    for (const event of events) {
      const key = eventKey(event, view.revision);
      if (seen.current.has(key)) continue;
      seen.current.add(key);
      const speaker = text(event.data.speakerName ?? event.data.playerName);
      const body = text(event.data.text ?? event.data.message ?? event.data.summary, event.type.replaceAll("_", " ").toLowerCase());
      const diagnostic = event.type === "PEG_PLAY" || event.type === "PEG_GO" || event.type === "PEG_LAST" || event.type === "COUNT_AWARDED";
      additions.push({ key, text: diagnostic ? body : speaker ? `${speaker}: “${body}”` : body, dialogue: !diagnostic && Boolean(speaker || event.type.includes("DIALOGUE")) });
    }
    if (additions.length) setHistory(old => [...old, ...additions].slice(-80));
  }, [events, view.revision]);
  useEffect(() => {
    const dialogue = Array.isArray(view.dialogue) ? view.dialogue : [];
    if (!playNoticesInitialized.current) {
      for (const value of dialogue) {
        const item = object(value);
        if (item?.type === "PEG_PLAY" || item?.type === "PEG_GO" || item?.type === "PEG_LAST") playNoticeSeen.current.add(text(item.id));
      }
      playNoticesInitialized.current = true;
      return;
    }
    const additions = dialogue.flatMap(value => {
      const item = object(value);
      if (!item || (item.type !== "PEG_PLAY" && item.type !== "PEG_GO" && item.type !== "PEG_LAST")) return [];
      const id = text(item.id);
      if (!id || playNoticeSeen.current.has(id)) return [];
      playNoticeSeen.current.add(id);
      const details = object(item.data);
      const player = players.find(candidate => candidate.id === item.playerId);
      return [{ id, kind: item.type === "PEG_GO" ? "go" as const : item.type === "PEG_LAST" ? "last" as const : "play" as const, name: player?.name ?? "Player", isAI: player?.isAI === true,
        card: text(details?.card), reason: text(details?.reason, "no points"),
        points: number(details?.points), score: number(details?.score), count: number(details?.runningCount) }];
    });
    if (additions.length) setPlayNotices(current => [...current, ...additions]);
  }, [view.dialogue, view.revision]);
  useEffect(() => {
    if (!playNotices.length) return;
    const notice = playNotices[0];
    if (notice.isAI && preferences.soundEnabled) {
      const key = notice.kind === "go" ? "go_declared"
        : notice.kind === "last" ? "self_last_card"
        : notice.reason.includes("makes 31") ? "self_thirty_one"
          : notice.reason.includes("makes 15") ? "self_fifteen"
            : notice.reason.includes("four of a kind") ? "self_double_pair_royal"
              : notice.reason.includes("three of a kind") ? "self_pair_royal"
                : notice.reason.includes("pairs") ? "self_pair"
                  : notice.reason.includes("run") ? "self_pegging_run" : null;
      if (key) {
        const characterIds = ["mabel", "arthur", "clara"] as const;
        const characterId = characterIds[Math.max(0, players.findIndex(player => player.name === notice.name)) % characterIds.length]!;
        void playScriptedDialogue(characterId, key, preferences.volume);
      }
    }
    const timer = window.setTimeout(() => setPlayNotices(current => current.slice(1)), 2600);
    return () => window.clearTimeout(timer);
  }, [playNotices[0]?.id]);

  const toggle = (id: string) => setSelected(old => {
    if (old.includes(id)) return old.filter(item => item !== id);
    const limit = phase === "pegging" ? 1 : needed;
    return old.length < limit ? [...old, id] : old;
  });
  const active = players.find(player => player.seat === turnSeat);
  const dealerPlayer = players.find(player => player.seat === dealerSeat);
  const opponentsInTurnOrder = players
    .filter(player => player.id !== playerId)
    .sort((left, right) => {
      const localSeat = me?.seat ?? 0;
      const leftDistance = ((left.seat ?? 0) - localSeat + players.length) % players.length;
      const rightDistance = ((right.seat ?? 0) - localSeat + players.length) % players.length;
      return leftDistance - rightDistance;
    });
  const hasDiscarded = phase === "discard" && hand.length === 4;
  const cribOwner = dealerPlayer?.id === playerId ? "your crib" : `${dealerPlayer?.name ?? "the dealer"}'s crib`;
  const turnMessage = phase === "discard"
    ? `Your turn. ${hasDiscarded ? `Waiting for the other players to discard for ${cribOwner}.` : `Discard ${needed} card${needed === 1 ? "" : "s"} for ${cribOwner}.`}`
    : phase === "pegging" && active
      ? `${active.id === playerId ? "Your" : `${active.name}'s`} turn. ${active.id === playerId ? "Play a card or say Go." : "Waiting for their play."}`
      : active ? `${active.id === playerId ? "Your" : `${active.name}'s`} turn.` : titlePhase(phase);
  const scoreLanes = players.length === 4
    ? (["gold", "green"] as const).map(teamId => ({
      id: teamId,
      label: `${teamId === "gold" ? "Red" : "Green"} · ${players.filter(player => player.teamId === teamId).map(player => player.name).join(" & ")}`,
      score: number(teamScores?.[teamId]),
      color: teamId === "gold" ? "red" as const : "green" as const,
    }))
    : players.map((player, index) => ({ id: player.id, label: player.name, score: number(teamScores?.[player.id]), color: (index === 2 ? "blue" : index === 1 ? "green" : "red") as PegColor }));
  const [scoreMoves, setScoreMoves] = useState<Record<string, { from: number; to: number; amount: number }>>({});
  const previousLaneScores = useRef<Record<string, number>>(Object.fromEntries(scoreLanes.map(lane => [lane.id, lane.score])));
  useEffect(() => {
    const nextScores = Object.fromEntries(scoreLanes.map(lane => [lane.id, lane.score]));
    const moves = Object.fromEntries(scoreLanes.flatMap(lane => {
      const from = previousLaneScores.current[lane.id] ?? lane.score;
      return lane.score > from ? [[lane.id, { from, to: lane.score, amount: lane.score - from }]] : [];
    }));
    previousLaneScores.current = nextScores;
    if (!Object.keys(moves).length) return;
    setScoreMoves(moves);
    const timer = window.setTimeout(() => setScoreMoves({}), 2400);
    return () => window.clearTimeout(timer);
  }, [view.revision]);
  const currentCount = object(state.currentCount);
  const alreadyAcknowledged = Array.isArray(state.acknowledgements) && state.acknowledgements.includes(playerId);
  const countedPlayer = players.find(player => player.id === currentCount?.playerId);
  const [aiReviewReady, setAiReviewReady] = useState(true);
  useEffect(() => {
    if (!countedPlayer?.isAI) { setAiReviewReady(true); return; }
    setAiReviewReady(false);
    const timer = window.setTimeout(() => setAiReviewReady(true), 2500);
    return () => window.clearTimeout(timer);
  }, [countedPlayer?.isAI, currentCount?.eventId]);
  const countedCards = cards(currentCount?.cards);
  const countStarter = parseCard(currentCount?.starterCard);
  const [countRevealReady, setCountRevealReady] = useState(true);
  const [countPegging, setCountPegging] = useState<{ name: string; points: number; score: number; color: PegColor } | null>(null);
  const previousCountForPeg = useRef<{ eventId: string; name: string; teamId: string; points: number; color: PegColor } | null>(null);
  const playerPegColor = (targetId: unknown): PegColor => {
    const index = players.findIndex(player => player.id === targetId);
    if (players.length === 4) return players[index]?.teamId === "green" ? "green" : "red";
    return index === 2 ? "blue" : index === 1 ? "green" : "red";
  };
  const [showCutResult, setShowCutResult] = useState(false);
  const previousPhase = useRef<string | undefined>(undefined);
  const previousPlayed = useRef(JSON.stringify(state.playedCards ?? []));
  const previousScores = useRef(JSON.stringify(teamScores ?? {}));
  const previousCountEvent = useRef("");
  const previousDealNumber = useRef(number(state.dealNumber));
  useEffect(() => configureGameAudio(preferences.soundEnabled, preferences.volume), [preferences.soundEnabled, preferences.volume]);
  useEffect(() => {
    const unlock = () => unlockGameAudio();
    window.addEventListener("pointerdown", unlock, { capture: true });
    window.addEventListener("keydown", unlock, { capture: true });
    return () => {
      window.removeEventListener("pointerdown", unlock, { capture: true });
      window.removeEventListener("keydown", unlock, { capture: true });
    };
  }, []);
  useEffect(() => {
    const oldPhase = previousPhase.current;
    if (phase === "cut" && oldPhase !== "cut") playGameSound("shuffle");
    if (phase === "discard" && oldPhase === "cut") {
      playGameSound("deal");
      setShowCutResult(true);
      const timer = window.setTimeout(() => setShowCutResult(false), 3000);
      previousPhase.current = phase;
      return () => window.clearTimeout(timer);
    }
    previousPhase.current = phase;
  }, [phase]);
  useEffect(() => {
    const played = JSON.stringify(state.playedCards ?? []);
    if (phase === "pegging" && played !== previousPlayed.current) playGameSound("card");
    previousPlayed.current = played;
  }, [phase, state.playedCards]);
  useEffect(() => {
    const scores = JSON.stringify(teamScores ?? {});
    if (phase === "pegging" && scores !== previousScores.current) playGameSound("peg");
    previousScores.current = scores;
  }, [phase, teamScores]);
  useEffect(() => {
    const eventId = text(currentCount?.eventId);
    if (phase === "counting" && eventId && eventId !== previousCountEvent.current && !previousCountForPeg.current) playGameSound("count");
    previousCountEvent.current = eventId;
  }, [currentCount?.eventId, phase]);
  useEffect(() => {
    const eventId = text(currentCount?.eventId);
    const previous = previousCountForPeg.current;
    if (previous && eventId !== previous.eventId) {
      setCountRevealReady(false);
      setCountPegging({
        name: previous.name,
        points: previous.points,
        score: number(teamScores?.[previous.teamId]),
        color: previous.color,
      });
      playGameSound("peg");
      const timer = window.setTimeout(() => {
        setCountPegging(null);
        setCountRevealReady(true);
        if (eventId) playGameSound("count");
      }, 2400);
      previousCountForPeg.current = eventId ? {
        eventId,
        name: currentCount?.kind === "crib" ? `${countedPlayer?.name ?? "Dealer"}'s crib` : countedPlayer?.name ?? "Player",
        teamId: text(currentCount?.teamId),
        points: number(currentCount?.points),
        color: playerPegColor(currentCount?.playerId),
      } : null;
      return () => window.clearTimeout(timer);
    }
    if (!previous && eventId) {
      previousCountForPeg.current = {
        eventId,
        name: currentCount?.kind === "crib" ? `${countedPlayer?.name ?? "Dealer"}'s crib` : countedPlayer?.name ?? "Player",
        teamId: text(currentCount?.teamId),
        points: number(currentCount?.points),
        color: playerPegColor(currentCount?.playerId),
      };
      setCountRevealReady(true);
    }
  }, [currentCount?.eventId, phase]);
  useEffect(() => {
    const dealNumber = number(state.dealNumber);
    if (dealNumber > previousDealNumber.current && dealNumber > 1) {
      playGameSound("shuffle");
      const timer = window.setTimeout(() => playGameSound("deal"), 420);
      previousDealNumber.current = dealNumber;
      return () => window.clearTimeout(timer);
    }
    previousDealNumber.current = dealNumber;
  }, [state.dealNumber]);

  return <main className="mp-table">
    <header className="mp-board">
      <div><span className="eyebrow">Private table · {titlePhase(phase)}</span><h1>Cribbage</h1></div>
      <div className={`mp-connection ${connection}`}>{connection === "connected" ? "Live" : "Reconnecting…"}</div>
      <CribbageBoard lanes={scoreLanes} moves={scoreMoves} />
    </header>
    {countPegging && <div className={`mp-count-pegging ${countPegging.color}`} role="status" aria-live="assertive">
      <strong>{countPegging.name} pegs {countPegging.points}</strong>
      <span>Score: {countPegging.score}</span>
    </div>}
    {playNotices[0] && <div className="mp-play-notice" role="status" aria-live="polite">
      {playNotices[0].kind === "go" ? <><strong>{playNotices[0].name} says Go!</strong><small>Running count: {playNotices[0].count}</small></>
        : playNotices[0].kind === "last" ? <><strong>{playNotices[0].name} pegs 1 for last card</strong><small>Score: {playNotices[0].score}</small></> : <>
        <strong>{playNotices[0].name} played <CardNotation value={playNotices[0].card} /></strong>
        <span><CardNotation value={playNotices[0].reason} /> · {playNotices[0].points} point{playNotices[0].points === 1 ? "" : "s"}</span>
        <small>Running count: {playNotices[0].count} · Score: {playNotices[0].score}</small>
      </>}
    </div>}

    <section className="mp-tabletop">
      <div className="mp-opponents">{opponentsInTurnOrder.map(player => <article key={player.id} className={`mp-player ${player.seat === turnSeat ? "active" : ""}`}>
        <span className="mp-avatar">{AVATARS.find(avatar => avatar.id === player.avatarId)?.glyph ?? "●"}</span>
        <div><strong>{player.name}{player.seat === dealerSeat ? " · Dealer" : ""}</strong><small>{player.connected === false ? "Disconnected" : player.isAI ? "Computer" : player.seat === turnSeat ? "Playing" : "Waiting"}</small></div>
        <div className="mp-hidden-hand">{Array.from({ length: player.handCount ?? 0 }, (_, index) => <PlayingCard hidden key={index} />)}</div>
      </article>)}</div>

      <div className="mp-center">
        {(phase === "cut" || showCutResult) && cutCards && Object.keys(cutCards).length > 0 && <div className={`mp-cut-reveal ${showCutResult ? "complete" : ""}`}>
          <strong>{showCutResult ? `${dealerPlayer?.name ?? "Low card"} cut low and deals` : "Low card deals"}</strong>
          <div>{players.map(player => {
            const card = parseCard(cutCards[player.id]);
            return <article className={showCutResult && player.seat === dealerSeat ? "dealer-cut" : ""} key={player.id}><span>{player.name}{showCutResult && player.seat === dealerSeat ? " · Dealer" : ""}</span>{card ? <PlayingCard card={card} disabled /> : <span className="mp-card back" />}</article>;
          })}</div>
        </div>}
        <div><small>Running count</small><strong className="mp-count">{runningCount}</strong></div>
        <div><small>Played</small><div className="mp-pile">{pile.map(card => <PlayingCard card={card} key={card.id} disabled />)}</div></div>
        <div><small>Starter</small>{cut ? <PlayingCard card={cut} disabled /> : <span className="mp-card-slot" />}</div>
        <div className="mp-prompt"><strong>{turnMessage}</strong>
          {phase === "cut" && !object(state.cutCards)?.[playerId] && <button className="primary" onClick={() => { unlockGameAudio(); playGameSound("shuffle"); send("CUT_CARD", {}); }}>Cut card</button>}
          {phase === "discard" && !hasDiscarded && <button className="primary" disabled={selected.length !== needed} onClick={() => send("DISCARD", { cards: selected.map(id => encodeCard(hand.find(card => card.id === id)!)) })}>Send exactly {needed}</button>}
          {phase === "pegging" && myTurn && selected.length === 1 && <button className="primary" disabled={!legalIds.has(selected[0])} onClick={() => send("PLAY_CARD", { card: encodeCard(hand.find(card => card.id === selected[0])!) })}>Play card</button>}
          {canGo && <button className="primary" onClick={() => send("SAY_GO", {})}>Say Go</button>}
          {phase === "dealcomplete" && isHost && <button className="primary" onClick={() => send("NEXT_DEAL", { eventId: state.pendingEventId })}>Next deal</button>}
          {phase === "complete" && <button className="primary" onClick={() => send(isHost ? "REMATCH" : "REQUEST_REMATCH", {})}>Rematch</button>}
          {["session_summary", "summary"].includes(phase) && <button className="primary" onClick={() => send("REQUEST_REMATCH", {})}>Rematch</button>}
        </div>
      </div>

      {["result", "complete", "session_summary", "summary"].includes(phase) && <section className="mp-result"><h2>{winner ? `${winner} wins` : titlePhase(phase)}</h2>
        {ledger && <><p>{number(ledger.gameCount ?? ledger.games)} games recorded · friendly recordkeeping only</p><div className="mp-ledger">{Array.isArray(ledger.entries) ? ledger.entries.map((entry, index) => <span key={index}>{text(object(entry)?.label ?? object(entry)?.playerName, `Entry ${index + 1}`)} <b>{number(object(entry)?.amount ?? object(entry)?.total)}¢</b></span>) : null}</div></>}
      </section>}

      <article className={`mp-player local ${myTurn ? "active" : ""}`}>
        <span className="mp-avatar">{AVATARS.find(avatar => avatar.id === me?.avatarId)?.glyph ?? "●"}</span>
        <div><strong>{me?.name ?? "You"}{me?.seat === dealerSeat ? " · Dealer" : ""}</strong><small>{myTurn ? "Your turn" : "Your hand"}</small></div>
        <div className="mp-local-hand">{hand.map(card => <PlayingCard card={card} key={card.id} selected={selected.includes(card.id)} disabled={phase !== "discard" && !(phase === "pegging" && myTurn && legalIds.has(card.id))} onClick={() => toggle(card.id)} />)}</div>
      </article>
    </section>

    <footer className="mp-footer"><div role="log" aria-live="polite">{history.length ? history.map(item => <p className={item.dialogue ? "dialogue" : ""} key={item.key}>{item.text}</p>) : <p>Waiting for the deal…</p>}</div>
      <button className="quiet" onClick={onLeave}>Leave table</button></footer>
    {connection !== "connected" && <div className="mp-reconnect" role="status"><strong>Reconnecting to the table…</strong><span>Your table is preserved.</span></div>}
    {host?.connected === false && <div className="mp-host-warning">Host disconnected. Host controls resume when they reconnect.</div>}
    {isHost && typeof state.pausedForPlayerId === "string" && <div className="mp-host-warning"><strong>Player disconnected</strong>
      <button onClick={() => send("WAIT_FOR_PLAYER", { playerId: state.pausedForPlayerId })}>Wait</button>
      <button onClick={() => send("REPLACE_WITH_AI", { playerId: state.pausedForPlayerId, difficulty: "medium" })}>Replace with AI</button>
      <button onClick={() => send("END_GAME", {})}>End game</button>
    </div>}
    {phase === "counting" && countingEntryReady && countRevealReady && currentCount && countStarter && <CountReveal
      title={currentCount.kind === "crib" ? `${countedPlayer?.name ?? "Dealer"}'s crib` : `${countedPlayer?.name ?? "Player"}'s hand`}
      hand={countedCards} starter={countStarter} isCrib={currentCount.kind === "crib"} points={number(currentCount.points)}
      canContinue={aiReviewReady} waiting={alreadyAcknowledged}
      onContinue={() => send("ACK_COUNT", { eventId: state.pendingEventId })}
    />}
  </main>;
}

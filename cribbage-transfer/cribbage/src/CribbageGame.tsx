"use client";

import { useEffect, useMemo, useRef, useState } from "react";

type Suit = "♠" | "♥" | "♦" | "♣";
type Card = { rank: number; suit: Suit; id: string };
type Player = { name: string; color: string; hand: Card[]; score: number; team: number };
type Phase = "menu" | "cutting" | "discard" | "pegging" | "counting" | "gameover";
type Difficulty = "easy" | "medium" | "hard";
type ScoreEvent = { label: string; points: number; cards: Card[] };
type HandCount = { label: string; color: string; kind: "hand" | "crib"; cards: Card[]; cut: Card; result: ReturnType<typeof scoreCards> };
type OpeningDraw = { player: number; card: Card; round: number };

const SUITS: Suit[] = ["♠", "♥", "♦", "♣"];
const COLORS = ["red", "blue", "green"];
const NAMES = ["You", "Mabel", "Arthur", "Clara"];
const RANK = (n: number) => n === 1 ? "A" : n === 11 ? "J" : n === 12 ? "Q" : n === 13 ? "K" : String(n);
const value = (c: Card) => Math.min(c.rank, 10);

function combinations<T>(items: T[], count: number): T[][] {
  if (count === 0) return [[]];
  if (items.length < count) return [];
  return items.flatMap((item, index) => combinations(items.slice(index + 1), count - 1).map(rest => [item, ...rest]));
}

function cribCardValue(card: Card) {
  return card.rank === 5 ? 5.5 : value(card) === 10 ? 1.2 : card.rank === 1 ? 1 : .35;
}

function discardSynergy(cards: Card[]) {
  let score = cards.reduce((sum, card) => sum + cribCardValue(card), 0);
  if (cards.length === 2) {
    if (cards[0].rank === cards[1].rank) score += 3;
    if (value(cards[0]) + value(cards[1]) === 15) score += 4;
    if (cards[0].suit === cards[1].suit) score += .6;
    const gap = Math.abs(cards[0].rank - cards[1].rank);
    if (gap === 1) score += 1.4;
    if (gap === 2) score += .8;
  }
  return score;
}

function deck(): Card[] {
  return SUITS.flatMap(suit => Array.from({ length: 13 }, (_, i) => ({ rank: i + 1, suit, id: `${suit}-${i + 1}` })))
    .sort(() => Math.random() - .5);
}

function scoreCards(cards: Card[], cut?: Card, isCrib = false) {
  const all = cut ? [...cards, cut] : cards;
  let fifteens = 0, pairs = 0, runs = 0, flush = 0, nobs = 0;
  const events: ScoreEvent[] = [];
  for (let mask = 1; mask < (1 << all.length); mask++) {
    const chosen = all.filter((_, i) => mask & (1 << i));
    if (chosen.reduce((s, c) => s + value(c), 0) === 15) {
      fifteens += 2;
      events.push({ label: "Fifteen", points: 2, cards: chosen });
    }
  }
  for (let i = 0; i < all.length; i++) for (let j = i + 1; j < all.length; j++) if (all[i].rank === all[j].rank) {
    pairs += 2;
    events.push({ label: "Pair", points: 2, cards: [all[i], all[j]] });
  }
  const counts = Array(14).fill(0); all.forEach(c => counts[c.rank]++);
  for (let len = 5; len >= 3 && !runs; len--) {
    const runEvents: ScoreEvent[] = [];
    for (let mask = 1; mask < (1 << all.length); mask++) {
      const chosen = all.filter((_, i) => mask & (1 << i));
      if (chosen.length !== len) continue;
      const ranks = chosen.map(c => c.rank).sort((a, b) => a - b);
      if (new Set(ranks).size === len && ranks[len - 1] - ranks[0] === len - 1) runEvents.push({ label: `Run of ${len}`, points: len, cards: chosen });
    }
    if (runEvents.length) {
      runs = runEvents.reduce((sum, event) => sum + event.points, 0);
      events.push(...runEvents);
    }
  }
  if (cut && cards.every(c => c.suit === cards[0].suit)) {
    if (cut.suit === cards[0].suit) { flush = 5; events.push({ label: "Five-card flush", points: 5, cards: all }); }
    else if (!isCrib) { flush = 4; events.push({ label: "Four-card flush", points: 4, cards }); }
  }
  const nob = cut && cards.find(c => c.rank === 11 && c.suit === cut.suit);
  if (cut && nob) { nobs = 1; events.push({ label: "His nobs", points: 1, cards: [nob, cut] }); }
  return { fifteens, pairs, runs, flush, nobs, total: fifteens + pairs + runs + flush + nobs, events };
}

function CountCard({ card, tableCard = false }: { card: Card; tableCard?: boolean }) {
  const warm = card.suit === "♥" || card.suit === "♦";
  return <span className={`count-card ${warm ? "warm" : ""} ${tableCard ? "table-card" : ""}`} title={tableCard ? "Table card" : undefined}>{RANK(card.rank)}{card.suit}</span>;
}

function CardView({ card, selected, hidden, onClick, small }: { card: Card; selected?: boolean; hidden?: boolean; onClick?: () => void; small?: boolean }) {
  const warm = !hidden && (card.suit === "♥" || card.suit === "♦");
  return <button className={`card ${hidden ? "back" : ""} ${warm ? "warm" : ""} ${selected ? "selected" : ""} ${small ? "small" : ""}`} onClick={onClick} disabled={!onClick} aria-label={hidden ? "Face-down card" : `${RANK(card.rank)} of ${card.suit}`}>
    {!hidden && <><span>{RANK(card.rank)}</span><b>{card.suit}</b><em>{card.suit}</em></>}
  </button>;
}

function Board({ players, playerCount }: { players: Player[]; playerCount: number }) {
  const laneInfo = [
    { name: playerCount === 4 ? "You + Arthur" : "You", score: players[0]?.score ?? 0, color: "red" },
    { name: playerCount === 4 ? "Mabel + Clara" : "Mabel", score: players[1]?.score ?? 0, color: "blue" },
    { name: playerCount >= 3 && playerCount !== 4 ? "Arthur" : "Open lane", score: playerCount === 3 ? players[2]?.score ?? 0 : 0, color: "green" },
  ];
  return <section className="board" aria-label="Three lane cribbage board">
    <div className="board-title"><span>♣</span><h1>Cribbage</h1><span>♣</span></div>
    <div className="route-key"><strong>START</strong><span>Score travels left to right</span><b>FINISH · 121</b></div>
    {laneInfo.map((lane, idx) => {
      const active = idx < (playerCount === 4 ? 2 : playerCount);
      return <div className={`lane ${lane.color} ${active ? "" : "inactive"}`} key={lane.color}>
        <div className="lane-name"><span className="color-dot" />{lane.name}<strong>{active ? lane.score : "—"}</strong></div>
        <div className="track">
          {Array.from({ length: 121 }, (_, i) => {
            const point = i + 1;
            const marker = active && Math.min(121, lane.score) === point;
            return <i key={point} className={`${point % 5 === 0 ? "fifth" : ""} ${marker ? "has-peg" : ""}`} title={`${point} points`}><span /></i>;
          })}
        </div>
        <div className="milestones"><span>0</span><span>30</span><span>60</span><span>90</span><span>121</span></div>
      </div>;
    })}
    <div className="start-plate">START<br/><small>0 points</small></div>
    <div className="finish-plate">★ FINISH ★<br/><small>First to 121</small></div>
  </section>;
}

export default function Home() {
  const [phase, setPhase] = useState<Phase>("menu");
  const [playerCount, setPlayerCount] = useState(3);
  const [difficulty, setDifficulty] = useState<Difficulty>("medium");
  const [players, setPlayers] = useState<Player[]>([]);
  const [scoringHands, setScoringHands] = useState<Card[][]>([]);
  const [dealer, setDealer] = useState(1);
  const [crib, setCrib] = useState<Card[]>([]);
  const [cut, setCut] = useState<Card | null>(null);
  const [selected, setSelected] = useState<string[]>([]);
  const [pile, setPile] = useState<Card[]>([]);
  const [running, setRunning] = useState(0);
  const [turn, setTurn] = useState(0);
  const [lastPegger, setLastPegger] = useState<number | null>(null);
  const [messageHistory, setMessageHistory] = useState<string[]>(["Choose the number of players, then start a game."]);
  const [breakdown, setBreakdown] = useState<ReturnType<typeof scoreCards> | null>(null);
  const [handCounts, setHandCounts] = useState<HandCount[]>([]);
  const [winner, setWinner] = useState("");
  const [openingDraws, setOpeningDraws] = useState<OpeningDraw[]>([]);
  const [muted, setMuted] = useState(false);
  const [volume, setVolume] = useState(.55);
  const deckRef = useRef<Card[]>([]);
  const messageWindowRef = useRef<HTMLDivElement>(null);
  const audioRef = useRef<AudioContext | null>(null);
  const mutedRef = useRef(muted);
  const volumeRef = useRef(volume);
  const pendingScoreByTeamRef = useRef<Record<number, number>>({});

  const needDiscard = playerCount === 2 ? 2 : playerCount === 3 ? 1 : 1;
  const handSize = playerCount === 2 ? 6 : 5;
  function announce(text: string) {
    setMessageHistory(old => [...old, text]);
  }
  function audioContext() {
    const AudioCtx = window.AudioContext || (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext;
    if (!AudioCtx) return null;
    if (!audioRef.current) audioRef.current = new AudioCtx();
    if (audioRef.current.state === "suspended") void audioRef.current.resume();
    return audioRef.current;
  }
  function tone(frequency: number, duration: number, delay = 0, type: OscillatorType = "sine", level = .13) {
    if (mutedRef.current) return;
    const ctx = audioContext(); if (!ctx) return;
    const osc = ctx.createOscillator(), gain = ctx.createGain();
    const start = ctx.currentTime + delay;
    osc.type = type; osc.frequency.setValueAtTime(frequency, start);
    gain.gain.setValueAtTime(.0001, start);
    gain.gain.exponentialRampToValueAtTime(Math.max(.0001, level * volumeRef.current), start + .008);
    gain.gain.exponentialRampToValueAtTime(.0001, start + duration);
    osc.connect(gain).connect(ctx.destination); osc.start(start); osc.stop(start + duration + .02);
  }
  function noise(duration = .08, delay = 0, level = .045) {
    if (mutedRef.current) return;
    const ctx = audioContext(); if (!ctx) return;
    const length = Math.max(1, Math.floor(ctx.sampleRate * duration));
    const buffer = ctx.createBuffer(1, length, ctx.sampleRate), data = buffer.getChannelData(0);
    for (let i = 0; i < length; i++) data[i] = (Math.random() * 2 - 1) * (1 - i / length);
    const source = ctx.createBufferSource(), filter = ctx.createBiquadFilter(), gain = ctx.createGain();
    source.buffer = buffer; filter.type = "bandpass"; filter.frequency.value = 1250;
    gain.gain.value = level * volumeRef.current;
    source.connect(filter).connect(gain).connect(ctx.destination); source.start(ctx.currentTime + delay);
  }
  function sound(kind: "click" | "deal" | "slide" | "card" | "go" | "count" | "win") {
    if (kind === "click") tone(520, .045, 0, "triangle", .07);
    if (kind === "deal") { [0,1,2,3].forEach(i => noise(.055, i * .045, .05)); tone(190, .12, .17, "triangle", .035); }
    if (kind === "slide") { noise(.14, 0, .055); tone(230, .09, .06, "triangle", .04); }
    if (kind === "card") { noise(.065, 0, .075); tone(150, .07, 0, "triangle", .05); }
    if (kind === "go") { tone(280, .1, 0, "triangle", .08); tone(220, .14, .09, "triangle", .07); }
    if (kind === "count") { tone(523, .11, 0, "sine", .08); tone(659, .14, .09, "sine", .09); }
    if (kind === "win") [523,659,784,1047].forEach((f,i) => tone(f, .28, i * .12, "triangle", .13));
  }
  function chime(player: number, steps = 1) {
    if (mutedRef.current) return;
    const ctx = audioContext(); if (!ctx) return;
    const base = [330, 440, 550][players[player]?.team ?? player % 3];
    for (let i = 0; i < Math.min(steps, 8); i++) {
      const osc = ctx.createOscillator(), gain = ctx.createGain();
      osc.type = "sine"; osc.frequency.value = base * Math.pow(2, i / 12);
      gain.gain.setValueAtTime(.0001, ctx.currentTime + i * .08); gain.gain.exponentialRampToValueAtTime(.11 * volumeRef.current, ctx.currentTime + i * .08 + .01); gain.gain.exponentialRampToValueAtTime(.0001, ctx.currentTime + i * .08 + .18);
      osc.connect(gain).connect(ctx.destination); osc.start(ctx.currentTime + i * .08); osc.stop(ctx.currentTime + i * .08 + .2);
    }
  }

  function addScore(index: number, amount: number, reason: string) {
    if (!amount) return false;
    const team = players[index].team;
    const pending = pendingScoreByTeamRef.current[team] ?? 0;
    const baseScore = (players.find(p => p.team === team)?.score ?? 0) + pending;
    const final = Math.min(121, baseScore + amount);
    pendingScoreByTeamRef.current[team] = pending + (final - baseScore);
    setPlayers(old => old.map((p, i) => {
      if (p.team !== team) return p;
      return { ...p, score: Math.min(121, p.score + amount) };
    }));
    chime(index, amount);
    const pegVerb = players[index].name === "You" ? "peg" : "pegs";
    announce(`${players[index].name} ${pegVerb} ${amount} for ${reason}. New score: ${final}.`);
    if (final >= 121) {
      const winnerName = playerCount === 4 ? (team === 0 ? "You and Arthur" : "Mabel and Clara") : players[index].name;
      const winVerb = playerCount === 4 || winnerName === "You" ? "win" : "wins";
      setWinner(winnerName);
      announce(`${winnerName} ${winVerb} the game! The finish peg reached 121.`);
      sound("win");
      setPhase("gameover");
      return true;
    }
    return false;
  }

  function dealRound(basePlayers = players, dealerIndex = dealer) {
    const d = deck(); deckRef.current = d;
    sound("deal");
    const dealt = basePlayers.slice(0, playerCount).map(p => ({ ...p, hand: d.splice(0, handSize).sort((a,b)=>a.rank-b.rank) }));
    setPlayers(old => old.map((p, i) => dealt[i] ?? p)); setScoringHands([]); setCrib([]); setCut(null); setSelected([]); setPile([]); setRunning(0); setLastPegger(null); setBreakdown(null); setHandCounts([]); setTurn((dealerIndex + 1) % playerCount); setPhase("discard");
    announce(`Round ${Math.floor(dealerIndex / playerCount) + 1}: choose ${needDiscard} card${needDiscard > 1 ? "s" : ""} for ${NAMES[dealerIndex]}’s crib.`);
  }

  function startGame() {
    const ps = NAMES.map((name, i) => ({ name, color: COLORS[i % 3], hand: [] as Card[], score: 0, team: playerCount === 4 ? i % 2 : i }));
    const drawDeck = deck();
    const draws: OpeningDraw[] = [];
    let contenders = Array.from({ length: playerCount }, (_, i) => i);
    let round = 1;
    while (contenders.length > 1) {
      const roundDraws = contenders.map(player => ({ player, card: drawDeck.shift()!, round }));
      draws.push(...roundDraws);
      const lowest = Math.min(...roundDraws.map(draw => draw.card.rank));
      contenders = roundDraws.filter(draw => draw.card.rank === lowest).map(draw => draw.player);
      round++;
    }
    const firstDealer = contenders[0];
    const history: string[] = [];
    for (let drawRound = 1; drawRound < round; drawRound++) {
      const roundDraws = draws.filter(draw => draw.round === drawRound);
      history.push(roundDraws.map(draw => `${NAMES[draw.player]} ${NAMES[draw.player] === "You" ? "draw" : "draws"} ${RANK(draw.card.rank)}${draw.card.suit}`).join(" · "));
      const lowRank = Math.min(...roundDraws.map(draw => draw.card.rank));
      const tied = roundDraws.filter(draw => draw.card.rank === lowRank);
      if (tied.length > 1) history.push(`${tied.map(draw => NAMES[draw.player]).join(" and ")} tie for low card and draw again.`);
    }
    history.push(`${NAMES[firstDealer]} ${NAMES[firstDealer] === "You" ? "win" : "wins"} the draw and get${NAMES[firstDealer] === "You" ? "" : "s"} the first deal and crib.`);
    setPlayers(ps); setDealer(firstDealer); setWinner(""); setMessageHistory(history); setOpeningDraws(draws); setCut(null); setPhase("cutting");
  }

  function beginFirstDeal() {
    setOpeningDraws([]);
    dealRound(players, dealer);
  }

  function toggleCard(id: string) { setSelected(s => s.includes(id) ? s.filter(x => x !== id) : s.length < needDiscard ? [...s, id] : s); }

  function finishDiscard() {
    if (selected.length !== needDiscard) return;
    sound("slide");
    const mine = players[0].hand.filter(c => selected.includes(c.id));
    const cribCards: Card[] = [];
    const updated = players.map((p, i) => {
      if (i >= playerCount) return p;
      const chosen = i === 0 ? mine : chooseAiDiscard(p.hand, i);
      cribCards.push(...chosen);
      return { ...p, hand: p.hand.filter(c => !chosen.some(x => x.id === c.id)) };
    });
    if (playerCount === 3) cribCards.push(deckRef.current.shift()!);
    const starter = deckRef.current.shift()!; setPlayers(updated); setCrib(cribCards); setScoringHands(updated.slice(0, playerCount).map(p => [...p.hand])); setCut(starter); setSelected([]); setPhase("pegging"); setTurn((dealer + 1) % playerCount); announce(`${RANK(starter.rank)}${starter.suit} is cut. Select a card to begin pegging.`);
    if (starter.rank === 11) addScore(dealer, 2, "his heels");
  }

  function chooseAiDiscard(hand: Card[], playerIndex: number) {
    if (difficulty === "easy") return [...hand].sort(() => Math.random() - .5).slice(0, needDiscard);
    const choices = combinations(hand, needDiscard);
    const possibleCuts = deckRef.current.filter(card => !hand.some(held => held.id === card.id));
    const ownsCrib = players[playerIndex]?.team === players[dealer]?.team;
    return choices.map(discarded => {
      const kept = hand.filter(card => !discarded.some(drop => drop.id === card.id));
      const sampleCuts = difficulty === "hard" ? possibleCuts : possibleCuts.filter((_, index) => index % 4 === 0);
      const handAverage = sampleCuts.reduce((sum, starter) => sum + scoreCards(kept, starter).total, 0) / Math.max(1, sampleCuts.length);
      const cribEffect = discardSynergy(discarded) * (ownsCrib ? 1 : -1);
      const danger = !ownsCrib && discarded.some(card => card.rank === 5) ? -3.5 : 0;
      return { discarded, rating: handAverage + cribEffect * (difficulty === "hard" ? .72 : .35) + danger };
    }).sort((a, b) => b.rating - a.rating)[0].discarded;
  }

  function pegPoints(next: Card, currentPile: Card[], total: number) {
    const cards = [...currentPile, next]; let pts = total + value(next) === 15 || total + value(next) === 31 ? 2 : 0;
    let same = 1; for (let i = cards.length - 2; i >= 0 && cards[i].rank === next.rank; i--) same++;
    if (same === 2) pts += 2; if (same === 3) pts += 6; if (same === 4) pts += 12;
    for (let len = Math.min(cards.length, 7); len >= 3; len--) { const ranks = cards.slice(-len).map(c=>c.rank).sort((a,b)=>a-b); if (new Set(ranks).size === len && ranks[len-1]-ranks[0] === len-1) { pts += len; break; } }
    return pts;
  }

  function playCard(index: number, card: Card) {
    if (value(card) + running > 31) return;
    sound("card");
    const pts = pegPoints(card, pile, running); const nextTotal = running + value(card);
    const remaining = players.slice(0, playerCount).reduce((n,p,i)=>n+p.hand.length-(i===index?1:0),0);
    const lastCardPoint = remaining === 0 && nextTotal !== 31 ? 1 : 0;
    setPlayers(old => old.map((p,i)=>i===index?{...p,hand:p.hand.filter(c=>c.id!==card.id)}:p)); setPile(old => [...old, card]); setRunning(nextTotal);
    setLastPegger(index);
    if (pts + lastCardPoint) {
      const won = addScore(index, pts + lastCardPoint, lastCardPoint ? (pts ? "pegging and last card" : "last card") : nextTotal === 31 ? "31" : "pegging");
      if (won) return;
    }
    else announce(`${players[index].name} plays ${RANK(card.rank)}${card.suit}. Count: ${nextTotal}.`);
    if (nextTotal === 31) { setPile([]); setRunning(0); setLastPegger(null); }
    if (!remaining) { setTimeout(() => beginCounting(), 500); return; }
    setTurn((index + 1) % playerCount);
  }

  function chooseAiPeg(index: number, playable: Card[]) {
    if (difficulty === "easy") return playable[Math.floor(Math.random() * playable.length)];
    const opponents = players.slice(0, playerCount).filter((_, i) => i !== index && players[i].team !== players[index].team);
    return playable.map(card => {
      const nextTotal = running + value(card);
      let rating = pegPoints(card, pile, running) * 8;
      if (nextTotal === 5 || nextTotal === 10 || nextTotal === 21) rating -= difficulty === "hard" ? 4 : 1.5;
      if (nextTotal === 15 || nextTotal === 31) rating += 4;
      if (pile.length === 0 && card.rank === 5) rating -= 7;
      rating -= value(card) * .05;
      if (difficulty === "hard") {
        const replyScores = opponents.flatMap(opponent => opponent.hand
          .filter(reply => value(reply) + nextTotal <= 31)
          .map(reply => pegPoints(reply, [...pile, card], nextTotal)));
        rating -= (replyScores.length ? Math.max(...replyScores) : 0) * 3.2;
        const matchingReplies = opponents.reduce((sum, opponent) => sum + opponent.hand.filter(reply => reply.rank === card.rank).length, 0);
        rating -= matchingReplies * 1.5;
      }
      return { card, rating };
    }).sort((a, b) => b.rating - a.rating)[0].card;
  }

  function beginCounting() { setPhase("counting"); setTurn((dealer + 1) % playerCount); setPile([]); setRunning(0); setLastPegger(null); announce("Pegging complete. Count each hand, then the dealer’s crib."); }

  function sayGo(index: number) {
    sound("go");
    const followingPlayers = Array.from({ length: playerCount - 1 }, (_, offset) => (index + offset + 1) % playerCount);
    const nextPlayablePosition = followingPlayers.findIndex(i => players[i].hand.some(c => value(c) + running <= 31));
    const nextPlayable = nextPlayablePosition >= 0 ? followingPlayers[nextPlayablePosition] : undefined;

    // Record every player passed over because they cannot legally play, not
    // only the player whose turn happened to trigger the search.
    const goPlayers = nextPlayablePosition >= 0
      ? [index, ...followingPlayers.slice(0, nextPlayablePosition)]
      : [index, ...followingPlayers];
    goPlayers.forEach(i => announce(`${players[i].name} says Go.`));

    if (nextPlayable !== undefined) {
      setTurn(nextPlayable);
      return;
    }

    if (lastPegger !== null && addScore(lastPegger, 1, "go")) return;
    const cardsRemain = players.slice(0, playerCount).some(p => p.hand.length > 0);
    if (!cardsRemain) { setTimeout(() => beginCounting(), 500); return; }

    const startAfter = lastPegger ?? index;
    const nextLeader = Array.from({ length: playerCount }, (_, offset) => (startAfter + offset + 1) % playerCount)
      .find(i => players[i].hand.length > 0);
    setPile([]); setRunning(0); setLastPegger(null);
    setTurn(nextLeader ?? 0);
  }

  function countCurrent() {
    if (!cut || turn < 0 || phase !== "counting") return;
    sound("count");
    const countingPlayer = turn;
    const result = scoreCards(scoringHands[countingPlayer] ?? [], cut);
    setBreakdown(result);
    const countedCards = scoringHands[countingPlayer] ?? [];
    setHandCounts(old => [...old, { label: players[countingPlayer].name, color: players[countingPlayer].color, kind: "hand", cards: countedCards, cut, result }]);
    if (addScore(countingPlayer, result.total, "the hand")) return;

    // Counting begins to the dealer's left and continues in play order.
    // The dealer is therefore always the final hand counted.
    const next = (turn + 1) % playerCount;
    if (next === (dealer + 1) % playerCount) {
      const cribResult = scoreCards(crib.slice(0,4), cut, true);
      setBreakdown(cribResult);
      setHandCounts(old => [...old, { label: `${players[dealer].name}’s crib`, color: players[dealer].color, kind: "crib", cards: crib.slice(0,4), cut, result: cribResult }]);
      if (addScore(dealer, cribResult.total, "the crib")) return;
      announce(`${players[dealer].name} ${players[dealer].name === "You" ? "peg" : "pegs"} ${cribResult.total} for the crib. New score: ${Math.min(121, players[dealer].score + cribResult.total)}. Start the next deal when ready.`); setTurn(-1);
    } else setTurn(next);
  }

  function nextRound() { const nextDealer = (dealer + 1) % playerCount; setDealer(nextDealer); dealRound(players, nextDealer); }

  useEffect(() => {
    mutedRef.current = muted;
  }, [muted]);

  useEffect(() => {
    pendingScoreByTeamRef.current = {};
  }, [players]);

  useEffect(() => {
    volumeRef.current = volume;
  }, [volume]);

  useEffect(() => {
    if (phase !== "pegging" || turn <= 0 || turn >= playerCount) return;
    const timer = setTimeout(() => {
      const playable = players[turn].hand.filter(c => value(c) + running <= 31);
      if (playable.length) playCard(turn, chooseAiPeg(turn, playable));
      else sayGo(turn);
    }, 650); return () => clearTimeout(timer);
  }, [phase, turn, running, players, playerCount]);

  useEffect(() => {
    if (phase !== "counting" || turn <= 0) return;
    const timer = setTimeout(countCurrent, 800); return () => clearTimeout(timer);
  }, [phase, turn]);

  useEffect(() => {
    const window = messageWindowRef.current;
    if (window) window.scrollTop = window.scrollHeight;
  }, [messageHistory]);

  const shownPlayers = useMemo(() => players.slice(0, playerCount), [players, playerCount]);

  return <main className="tabletop">
    <div className="game-shell" onClickCapture={event => {
      const button = (event.target as HTMLElement).closest("button");
      if (button && !button.classList.contains("card") && !button.classList.contains("sound-toggle")) sound("click");
    }}>
      <Board players={players} playerCount={playerCount} />
      <section className="play-area">
        <div className="status-bar"><span className="phase-tag">{phase === "menu" ? "Welcome" : phase}</span><div className="history-column"><strong className="history-title">Pegging history</strong><div className="message-window" ref={messageWindowRef} role="log" aria-live="polite" aria-label="Game and pegging history">{messageHistory.map((entry, i) => <p className={i === messageHistory.length - 1 ? "latest" : ""} key={`${i}-${entry}`}>{entry}</p>)}</div></div><div className="sound-controls"><button className="quiet sound-toggle" onClick={() => { setMuted(value => !value); if (muted) setTimeout(() => sound("click"), 0); }} aria-pressed={muted}>{muted ? "Sound off" : "Sound on"}</button><label>Volume<input aria-label="Sound volume" type="range" min="0" max="1" step="0.05" value={volume} onChange={event => setVolume(Number(event.target.value))} /></label><button className="quiet" onClick={() => setPhase("menu")}>Menu</button><a className="quiet home-link" href="https://vabgames.com" onClick={() => sound("click")}>Back to VABGames.com</a></div></div>
        {phase === "menu" ? <div className="menu-panel">
          <div><span className="eyebrow">A classic card-room game</span><h2>Pull up a chair.</h2><p>Play against up to three computer opponents. Four-player games use traditional partnerships.</p></div>
          <div className="menu-controls"><label>Number of players</label><div className="player-picks">{[2,3,4].map(n=><button key={n} className={playerCount===n?"active":""} onClick={()=>setPlayerCount(n)}>{n}</button>)}</div><label>Computer challenge</label><div className="difficulty-picks">{(["easy","medium","hard"] as Difficulty[]).map(level=><button key={level} className={difficulty===level?"active":""} onClick={()=>setDifficulty(level)}>{level}</button>)}</div><p className="difficulty-note">{difficulty === "easy" ? "Relaxed play with occasional computer mistakes." : difficulty === "medium" ? "Balanced opponents that make sensible choices." : "Skilled opponents that evaluate hands, cribs, and pegging replies."}</p><button className="primary" onClick={startGame}>Start game</button><button className="quit" onClick={()=>announce("Thanks for playing. You can close this tab whenever you’re ready.")}>Quit</button></div>
        </div> : phase === "cutting" ? <div className="opening-draw">
          <span className="eyebrow">Cut for first crib</span>
          <h2>Low card deals first</h2>
          {Array.from(new Set(openingDraws.map(draw => draw.round))).map(round => <div className="draw-round" key={round}>
            {round > 1 && <strong>Tie-break draw</strong>}
            <div>{openingDraws.filter(draw => draw.round === round).map(draw => <article key={`${round}-${draw.player}`}><span className={`player-token ${players[draw.player]?.color}`} /><b>{players[draw.player]?.name}</b><CardView card={draw.card} /></article>)}</div>
          </div>)}
          <p><strong>{players[dealer]?.name}</strong> {dealer === 0 ? "win" : "wins"} the first deal and crib.</p>
          <button className="primary" onClick={beginFirstDeal}>Deal first hand</button>
        </div> : <>
          <div className="table-center">
            <div className="pile-zone"><span>PEGGING COUNT</span><strong>{running}</strong><div className="mini-pile" aria-label="Cards played in the current pegging sequence">{pile.map(c=><CardView key={c.id} card={c} small />)}</div></div>
            <div className="cut-zone"><span>STARTER CARD</span>{cut ? <CardView card={cut} small /> : <div className="card-placeholder" />}</div>
            <div className="count-box"><span>HAND COUNT</span>{handCounts.length ? <div className="hand-count-list">{handCounts.map((count, i) => <div className="hand-count-row" key={`${count.kind}-${count.label}-${i}`}><div className="hand-count-heading"><span className={`count-token ${count.color}`} /><strong>{count.label}</strong><div className="counted-cards">{[...count.cards, count.cut].map(card => <CountCard key={card.id} card={card} tableCard={card.id === count.cut.id} />)}</div><b>{count.result.total}</b></div><div className="score-events">{count.result.events.length ? count.result.events.map((event, eventIndex) => <div className="score-event" key={`${event.label}-${eventIndex}`}><span>{event.label}</span><div>{event.cards.map(card => <CountCard key={card.id} card={card} tableCard={card.id === count.cut.id} />)}</div><b>+{event.points}</b></div>) : <div className="score-event zero"><span>No scoring combinations</span><b>+0</b></div>}</div></div>)}</div> : breakdown ? <p>Counting hands…</p> : <p>Scores will appear here.</p>}{phase === "counting" && turn === -1 && <div className="crib-reveal"><strong>{dealer === 0 ? "Your crib" : `${players[dealer]?.name}’s crib`}</strong><div>{crib.slice(0,4).map(card => <CardView key={card.id} card={card} small />)}</div></div>}</div>
            <div className="action-zone">{phase === "discard" && <button className="primary" disabled={selected.length!==needDiscard} onClick={finishDiscard}>Send {needDiscard} to crib</button>}{phase === "pegging" && turn===0 && !players[0]?.hand.some(c=>value(c)+running<=31) && <button className="primary" onClick={()=>sayGo(0)}>Say Go</button>}{phase === "counting" && turn===0 && <button className="primary" onClick={countCurrent}>Count my hand</button>}{phase === "counting" && turn===-1 && <button className="primary" onClick={nextRound}>Next deal</button>}</div>
          </div>
          <div className="players">{shownPlayers.map((p,i)=>{ const shownHand = phase === "counting" ? (scoringHands[i] ?? []) : p.hand; return <article className={`player ${turn===i?"turn":""}`} key={p.name}><header><span className={`player-token ${p.color}`}/><h3>{p.name}</h3><strong>{p.score}</strong>{i>0&&<small>AI</small>}</header><div className="hand">{shownHand.map(c=><CardView key={c.id} card={c} hidden={i>0 && phase!=="counting"} selected={selected.includes(c.id)} onClick={i===0 && phase==="discard"?()=>toggleCard(c.id):i===0&&phase==="pegging"&&turn===0?()=>playCard(0,c):undefined} />)}</div></article>})}</div>
        </>}
      </section>
      {phase === "gameover" && <div className="modal"><div><span>★ GAME ★</span><h2>{winner} {playerCount === 4 || winner === "You" ? "win" : "wins"}!</h2><p>The finish peg has reached 121.</p><button className="primary" onClick={()=>setPhase("menu")}>Play again</button></div></div>}
    </div>
  </main>;
}

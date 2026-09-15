import type {
  AiLevel,
  HintKind,
  PresentationEvent,
  PublicPuzzle,
  RoomSnapshot
} from "../../../shared/multiplayer/types";
import { Assets } from "../assets";
import { MultiplayerClient } from "./client";

const AI_LEVELS: AiLevel[] = ["High School", "College", "Professional"];
const HINTS: Array<{ kind: HintKind; label: string; cost: number }> = [
  { kind: "letter", label: "Letter", cost: 2 },
  { kind: "random", label: "Random", cost: 3 },
  { kind: "full-word", label: "Full Word", cost: 5 },
  { kind: "first-of-each", label: "1st of Each", cost: 7 }
];

const escapeHtml = (value: string | number) =>
  String(value)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&#039;");

const cellKey = (row: number, col: number) => `${row},${col}`;
const GEM_RANK: Record<string, number> = { none: 0, emerald: 1, ruby: 2, diamond: 3 };
const GEM_BONUS: Record<string, number> = { none: 0, emerald: 5, ruby: 10, diamond: 15 };

type GemName = "none" | "emerald" | "ruby" | "diamond";

interface ScoreFlightComponents {
  letters: number;
  emerald: number;
  ruby: number;
  diamond: number;
  strongestGem: GemName;
}

export class MultiplayerController {
  private snapshot: RoomSnapshot | null = null;
  private feedback = "Connected to the library table.";
  private connection = "connecting";
  private selectedIndices: number[] = [];
  private wheelLetters: string[] = [];
  private awaitingLetterHint = false;
  private showBonusList = false;
  private sounds: Record<string, HTMLAudioElement> = {};
  private audioUnlocked = false;
  private processedEventSequences = new Set<number>();
  private animationLayer: HTMLDivElement | null = null;
  private turnOrderPopup: { roundNumber: number; names: string[]; hideAt: number } | null = null;
  private uiTicker: number | null = null;
  private shownTurnOrderKey: string | null = null;
  private startSessionTimeout: number | null = null;

  constructor(
    private readonly root: HTMLElement,
    readonly client: MultiplayerClient,
    private readonly onLeave: () => void
  ) {
    for (const [name, url] of Object.entries(Assets.sounds)) {
      const audio = new Audio(url);
      audio.preload = "auto";
      this.sounds[name] = audio;
    }
    root.addEventListener("click", (event) => this.handleClick(event));
    root.addEventListener("change", (event) => this.handleChange(event));
    window.addEventListener("keydown", this.handleKeyDown);
    this.uiTicker = window.setInterval(() => {
      if (!this.snapshot) return;
      const activeCountdown =
        this.snapshot.turnState &&
        this.snapshot.settings.playMode === "Turn Based" &&
        this.snapshot.turnState.turnEndsAt !== null;
      // Keep ticking while popup object exists so expired popups get one final render pass and disappear.
      const showingTurnPopup = this.turnOrderPopup !== null;
      if (activeCountdown || showingTurnPopup) this.render();
    }, 250);
  }

  destroy() {
    window.removeEventListener("keydown", this.handleKeyDown);
    if (this.uiTicker !== null) {
      window.clearInterval(this.uiTicker);
      this.uiTicker = null;
    }
    if (this.animationLayer) {
      this.animationLayer.remove();
      this.animationLayer = null;
    }
    if (this.startSessionTimeout !== null) {
      window.clearTimeout(this.startSessionTimeout);
      this.startSessionTimeout = null;
    }
    this.client.close();
  }

  setConnection(status: string) {
    this.connection = status;
    this.render();
  }

  setError(message: string) {
    this.feedback = message;
    if (this.startSessionTimeout !== null) {
      window.clearTimeout(this.startSessionTimeout);
      this.startSessionTimeout = null;
    }
    this.render();
  }

  setSnapshot(snapshot: RoomSnapshot, events: PresentationEvent[]) {
    const previousStatus = this.snapshot?.status ?? null;
    const previousSnapshot = this.snapshot;
    const puzzleChanged = snapshot.puzzle?.id !== this.snapshot?.puzzle?.id;
    this.snapshot = snapshot;
    if (snapshot.status !== "lobby" && this.startSessionTimeout !== null) {
      window.clearTimeout(this.startSessionTimeout);
      this.startSessionTimeout = null;
    }
    if (puzzleChanged) {
      this.wheelLetters = snapshot.puzzle?.baseLetters.split("") ?? [];
      this.clearGuess();
      this.showBonusList = false;
      this.shownTurnOrderKey = null;
      this.feedback = "";
    }
    this.maybeShowTurnOrderFromSnapshot(snapshot, previousSnapshot);
    const latest = [...events]
      .reverse()
      .find((event) => event.type !== "turn-order" && event.type !== "turn-advanced");
    if (latest) this.feedback = latest.text;
    this.render();
    this.ensureAnimationLayer();
    const uiChanged = this.processPresentationEvents(events, snapshot, previousSnapshot);
    if (
      previousStatus === "playing" &&
      (snapshot.status === "puzzle-summary" || snapshot.status === "completed")
    ) {
      this.playSound("win");
    }
    if (uiChanged) this.render();
  }

  private render() {
    const snapshot = this.snapshot;
    if (!snapshot) {
      this.root.innerHTML = `
        <section class="mp-shell mp-loading">
          <div class="mp-paper"><h1>Joining room…</h1><p>${escapeHtml(this.connection)}</p></div>
        </section>`;
      return;
    }
    const local = snapshot.participants.find((participant) => participant.id === snapshot.localParticipantId);
    const host = snapshot.hostId === snapshot.localParticipantId;
    const connectionBanner =
      this.connection === "connected"
        ? ""
        : `<div class="mp-connection-banner">${escapeHtml(this.connection)} — competitive actions are unavailable.</div>`;
    this.root.innerHTML = `
      <section class="mp-shell">
        ${connectionBanner}
        <header class="mp-room-header">
          <button class="mp-link-button" data-action="leave">Leave</button>
          <strong>Room ${escapeHtml(snapshot.roomCode)}</strong>
          <span>${escapeHtml(snapshot.settings.mode)} · ${escapeHtml(snapshot.settings.playMode)} · ${escapeHtml(String(snapshot.settings.turnTimeLimit))} · ${escapeHtml(snapshot.settings.difficulty)}</span>
          <button class="mp-link-button" data-action="copy-invite">Copy Invite</button>
        </header>
        ${
          snapshot.status === "lobby"
            ? this.renderLobby(snapshot, host)
            : snapshot.status === "playing"
              ? this.renderPlay(snapshot, local?.hintCredits ?? 0, host)
              : snapshot.status === "puzzle-summary"
                ? this.renderSummary(snapshot, Boolean(local?.continued))
                : this.renderResults(snapshot, host)
        }
      </section>`;
  }

  private renderLobby(snapshot: RoomSnapshot, host: boolean) {
    const seats = Array.from({ length: snapshot.settings.capacity }, (_, seat) =>
      snapshot.participants.find((participant) => participant.seat === seat)
    );
    return `
      <main class="mp-lobby mp-paper">
        <h1>Multiplayer Library</h1>
        <p>Invite friends, add AI readers, then ready up.</p>
        <div class="mp-lobby-layout">
          <div class="mp-seat-list">
            ${seats
              .map((participant, seat) =>
                participant
                  ? `<article class="mp-seat" style="--player-color:${participant.color}">
                      <span class="mp-color-dot"></span>
                      <div><strong>${escapeHtml(participant.name)}</strong>
                      <small>${participant.kind === "ai" ? `AI · ${escapeHtml(participant.aiLevel ?? "College")}` : participant.connected ? "Human · connected" : "Human · disconnected"}</small></div>
                      <span>${participant.ready ? "Ready" : "Not ready"}</span>
                      ${
                        host && participant.kind === "ai"
                          ? `<button data-action="remove-ai" data-participant="${participant.id}">Remove</button>`
                          : ""
                      }
                    </article>`
                  : `<article class="mp-seat mp-seat-open"><span>Seat ${seat + 1}</span><small>Open</small></article>`
              )
              .join("")}
          </div>
          <div class="mp-settings">
            <label>Style
              <select data-setting="mode" ${host ? "" : "disabled"}>
                <option ${snapshot.settings.mode === "Casual" ? "selected" : ""}>Casual</option>
                <option ${snapshot.settings.mode === "Crossword" ? "selected" : ""}>Crossword</option>
              </select>
            </label>
            <label>Mode
              <select data-setting="playMode" ${host ? "" : "disabled"}>
                <option ${snapshot.settings.playMode === "Free for All" ? "selected" : ""}>Free for All</option>
                <option ${snapshot.settings.playMode === "Turn Based" ? "selected" : ""}>Turn Based</option>
              </select>
            </label>
            <label>Time
              <select data-setting="turnTimeLimit" ${
                host && snapshot.settings.playMode === "Turn Based" ? "" : "disabled"
              }>
                ${["Not Timed", 30, 25, 20, 15, 10]
                  .map((value) => `<option ${snapshot.settings.turnTimeLimit === value ? "selected" : ""}>${value}</option>`)
                  .join("")}
              </select>
            </label>
            <label>Difficulty
              <select data-setting="difficulty" ${host ? "" : "disabled"}>
                ${["Easy", "Medium", "Hard"].map((value) => `<option ${snapshot.settings.difficulty === value ? "selected" : ""}>${value}</option>`).join("")}
              </select>
            </label>
            <label>Seats
              <select data-setting="capacity" ${host ? "" : "disabled"}>
                ${[1, 2, 3, 4].map((value) => `<option ${snapshot.settings.capacity === value ? "selected" : ""}>${value}</option>`).join("")}
              </select>
            </label>
            ${
              host && snapshot.participants.length < snapshot.settings.capacity
                ? `<div class="mp-ai-controls">
                    <select id="mp-ai-level">${AI_LEVELS.map((level) => `<option>${level}</option>`).join("")}</select>
                    <button data-action="add-ai">Add AI</button>
                  </div>`
                : ""
            }
          </div>
        </div>
        <div class="mp-actions">
          <button data-action="toggle-ready">${
            snapshot.participants.find((participant) => participant.id === snapshot.localParticipantId)?.ready
              ? "Not Ready"
              : "Ready"
          }</button>
          ${host ? `<button class="mp-primary" data-action="start">Start Session</button>` : "<span>Waiting for the host to start.</span>"}
        </div>
        <p class="mp-feedback" role="status">${escapeHtml(this.feedback)}</p>
      </main>`;
  }

  private renderPlay(snapshot: RoomSnapshot, hintCredits: number, host: boolean) {
    const puzzle = snapshot.puzzle;
    if (!puzzle) return `<main class="mp-paper"><h1>Preparing puzzle…</h1></main>`;
    const localTurn = this.isLocalTurn(snapshot);
    const controlsDisabled =
      snapshot.settings.playMode === "Turn Based" &&
      (!localTurn || this.isTurnOrderPopupActive());
    return `
      ${this.renderScores(snapshot)}
      <div class="mp-play-status">
        <span>Puzzle ${snapshot.puzzleIndex + 1} of ${snapshot.puzzleCount}</span>
        <span style="color:${escapeHtml(this.activeTurnColor(snapshot))};font-weight:800;">${escapeHtml(this.roundTurnText(snapshot))}</span>
        <span>Hint Credits: <strong>${hintCredits}</strong></span>
        <span>${escapeHtml(this.timeStatusText(snapshot))}</span>
      </div>
      ${this.renderTurnOrderPopup()}
      ${
        snapshot.paused
          ? this.renderPause(snapshot, host)
          : `<main class="mp-game-layout">
              <section class="mp-board-panel mp-paper">
                ${this.renderBoard(puzzle, snapshot)}
                <p class="mp-feedback" role="status">${escapeHtml(this.feedback)}</p>
              </section>
              <aside class="mp-controls mp-paper">
                <div class="mp-guess">${escapeHtml(this.currentGuess()) || "Choose letters"}</div>
                <div class="mp-wheel">${this.wheelLetters
                  .map(
                    (letter, index) =>
                      `<button style="--wheel-index:${index};--wheel-count:${this.wheelLetters.length}" data-action="letter" data-index="${index}" class="${this.selectedIndices.includes(index) ? "selected" : ""}" ${controlsDisabled ? "disabled" : ""}>${escapeHtml(letter)}</button>`
                  )
                  .join("")}</div>
                <div class="mp-control-row">
                  <button data-action="clear" ${controlsDisabled ? "disabled" : ""}>Clear</button>
                  <button data-action="shuffle" ${controlsDisabled ? "disabled" : ""}>Shuffle</button>
                  <button class="mp-submit" data-action="submit" ${controlsDisabled ? "disabled" : ""}>Submit</button>
                </div>
                <div class="mp-hints">
                  ${HINTS.map(
                    (hint) =>
                      `<button data-action="hint" data-hint="${hint.kind}" ${
                        hintCredits < hint.cost || controlsDisabled ? "disabled" : ""
                      }>${hint.label}<small>${hint.cost}</small></button>`
                  ).join("")}
                </div>
                <button data-action="skip" ${controlsDisabled ? "disabled" : ""}>Request Skip</button>
              </aside>
            </main>`
      }
      ${this.renderBonusInfo(snapshot)}
      ${this.showBonusList ? this.renderBonusListPopup(snapshot) : ""}`;
  }

  private renderTurnOrderPopup() {
    if (!this.turnOrderPopup || this.turnOrderPopup.hideAt <= Date.now()) {
      this.turnOrderPopup = null;
      return "";
    }
    return `<div class="mp-turn-order-popup" role="alert" aria-live="assertive">
      <h3>Round ${this.turnOrderPopup.roundNumber}</h3>
      <ol>
        ${this.turnOrderPopup.names.map((name) => `<li>${escapeHtml(name)}</li>`).join("")}
      </ol>
    </div>`;
  }

  private isLocalTurn(snapshot: RoomSnapshot) {
    if (snapshot.settings.playMode !== "Turn Based") return true;
    return snapshot.turnState?.activeParticipantId === snapshot.localParticipantId;
  }

  private roundTurnText(snapshot: RoomSnapshot) {
    if (snapshot.settings.playMode !== "Turn Based" || !snapshot.turnState) {
      return "Free for All";
    }
    const active = snapshot.participants.find(
      (participant) => participant.id === snapshot.turnState?.activeParticipantId
    );
    return `Round ${snapshot.turnState.roundNumber}: ${(active?.name ?? "Player")}'s Turn`;
  }

  private activeTurnColor(snapshot: RoomSnapshot) {
    if (snapshot.settings.playMode !== "Turn Based" || !snapshot.turnState) return "#fff5dc";
    return (
      snapshot.participants.find((participant) => participant.id === snapshot.turnState?.activeParticipantId)?.color ??
      "#fff5dc"
    );
  }

  private timeStatusText(snapshot: RoomSnapshot) {
    if (snapshot.settings.turnTimeLimit === "Not Timed") return "Not Timed";
    if (snapshot.settings.playMode !== "Turn Based" || !snapshot.turnState || snapshot.turnState.turnEndsAt === null) {
      return `${snapshot.settings.turnTimeLimit}s`;
    }
    const secondsRemaining = Math.max(0, Math.ceil((snapshot.turnState.turnEndsAt - Date.now()) / 1000));
    return `${secondsRemaining}s`;
  }

  private turnOrderText(snapshot: RoomSnapshot) {
    if (snapshot.settings.playMode !== "Turn Based" || !snapshot.turnState) return "";
    const names = snapshot.turnState.turnOrder
      .map((id) => snapshot.participants.find((participant) => participant.id === id)?.name ?? "Player")
      .join(" -> ");
    return `Turn order (Round ${snapshot.turnState.roundNumber}): ${names}`;
  }

  private maybeShowTurnOrderFromSnapshot(snapshot: RoomSnapshot, previousSnapshot: RoomSnapshot | null) {
    if (snapshot.settings.playMode !== "Turn Based" || !snapshot.turnState) return;
    const key = `${snapshot.puzzle?.id ?? "none"}`;
    if (this.shownTurnOrderKey === key) return;
    const shouldShow =
      previousSnapshot?.status !== "playing" ||
      previousSnapshot?.puzzle?.id !== snapshot.puzzle?.id;
    if (!shouldShow) return;
    const names = snapshot.turnState.turnOrder.map(
      (id) => snapshot.participants.find((participant) => participant.id === id)?.name ?? "Player"
    );
    this.turnOrderPopup = {
      roundNumber: snapshot.turnState.roundNumber,
      names,
      hideAt: Date.now() + 3000
    };
    this.shownTurnOrderKey = key;
  }

  private isTurnOrderPopupActive() {
    return Boolean(this.turnOrderPopup && this.turnOrderPopup.hideAt > Date.now());
  }

  private renderScores(snapshot: RoomSnapshot) {
    const activeTurnId = snapshot.turnState?.activeParticipantId ?? "";
    return `<div class="mp-score-strip">${[...snapshot.participants]
      .sort((left, right) => left.seat - right.seat)
      .map((participant) => {
        const activeClass =
          snapshot.settings.playMode === "Turn Based" && participant.id === activeTurnId ? "mp-active-turn" : "";
        return `<article class="${activeClass}" style="--player-color:${participant.color}" data-participant-id="${participant.id}">
          <div class="mp-player-identity">
            <span class="mp-color-dot"></span>
            <strong>${escapeHtml(participant.name)}</strong>
            <span class="mp-player-level">${
              participant.kind === "ai" ? `AI · ${escapeHtml(participant.aiLevel ?? "College")}` : "Human"
            }</span>
          </div>
          <b class="mp-score-total" data-score-field="total">${participant.score.total}</b>
          <small class="mp-score-breakdown">
            <span data-score-field="letters">L ${participant.score.letters}</span> ·
            <span data-score-field="emerald">E ${participant.score.emerald}</span> ·
            <span data-score-field="diamond">D ${participant.score.diamond}</span> ·
            <span data-score-field="ruby">R ${participant.score.ruby}</span>
          </small>
        </article>`;
      })
      .join("")}</div>`;
  }

  private renderBoard(puzzle: PublicPuzzle, snapshot: RoomSnapshot) {
    const cells = new Map<string, { row: number; col: number; refs: Array<{ wordId: string; position: number }> }>();
    for (const word of puzzle.words) {
      word.cells.forEach((cell, position) => {
        const key = cellKey(cell.row, cell.col);
        const entry = cells.get(key) ?? { ...cell, refs: [] };
        entry.refs.push({ wordId: word.id, position });
        cells.set(key, entry);
      });
    }
    return `<div class="mp-board-scroll" role="region" aria-label="Puzzle board" tabindex="0"><div class="mp-board" style="--rows:${puzzle.rows};--cols:${puzzle.cols}">
      ${[...cells.entries()]
        .map(([key, cell]) => {
          const visible = puzzle.visibleCells[key];
          const owner = snapshot.participants.find((participant) => participant.id === visible?.ownerId);
          const ref = cell.refs.find(
            (candidate) => !puzzle.words.find((word) => word.id === candidate.wordId)?.completed
          ) ?? cell.refs[0];
          const gem = cell.refs
            .map((candidate) => {
              const word = puzzle.words.find((entry) => entry.id === candidate.wordId);
              return word?.gems[candidate.position] ?? "none";
            })
            .reduce((best, current) => (GEM_RANK[current] > GEM_RANK[best] ? current : best), "none");
          return `<button class="mp-cell ${visible ? "filled" : ""}" style="grid-row:${cell.row + 1};grid-column:${cell.col + 1};--owner-color:${owner?.color ?? "#5b4631"}"
            data-action="board-cell" data-word="${ref.wordId}" data-position="${ref.position}"
            data-row="${cell.row}" data-col="${cell.col}"
            aria-label="${visible ? `${visible.letter}, owned by ${owner?.name ?? "player"}` : "Unrevealed letter"}">
            ${!visible && gem !== "none" ? `<span class="mp-cell-gem ${gem}" title="${gem} word"></span>` : ""}
            ${visible ? `<span class="mp-cell-letter">${escapeHtml(visible.letter)}</span>` : ""}
          </button>`;
        })
        .join("")}
    </div></div>`;
  }

  private renderBonusInfo(snapshot: RoomSnapshot) {
    const puzzle = snapshot.puzzle;
    const found = puzzle?.claimedBonusCount ?? 0;
    const total = puzzle?.bonusWordCount ?? 0;
    return `<section class="mp-bonus-info mp-paper">
      <span>Bonus words found: <strong>${found}/${total}</strong></span>
      <button data-action="toggle-bonus-list" ${total === 0 ? "disabled" : ""}>List</button>
    </section>`;
  }

  private renderBonusListPopup(snapshot: RoomSnapshot) {
    const words = snapshot.puzzle?.claimedBonusWords ?? [];
    return `<section class="mp-bonus-popup mp-paper" role="dialog" aria-label="Bonus words list">
      <header>
        <h2>Bonus Words</h2>
        <button data-action="close-bonus-list" aria-label="Close bonus list">Close</button>
      </header>
      ${
        words.length > 0
          ? `<div class="mp-bonus-word-list">${words
              .map((word) => `<span>${escapeHtml(word.toUpperCase())}</span>`)
              .join("")}</div>`
          : `<p>No bonus words found yet.</p>`
      }
    </section>`;
  }

  private renderPause(snapshot: RoomSnapshot, host: boolean) {
    const missing = snapshot.participants.find(
      (participant) => participant.id === snapshot.disconnectedParticipantId
    );
    return `<main class="mp-paper mp-pause">
      <h1>Game Paused</h1>
      <p>${missing ? `${escapeHtml(missing.name)} disconnected.` : "Everyone is connected. The host can resume."}</p>
      ${
        host
          ? `<div class="mp-actions">
              <button data-action="resume">Resume</button>
              ${
                missing
                  ? `<select id="mp-replace-level">${AI_LEVELS.map((level) => `<option>${level}</option>`).join("")}</select>
                     <button data-action="replace" data-participant="${missing.id}">Replace with AI</button>`
                  : ""
              }
              <button data-action="end">End Session</button>
            </div>`
          : "<p>Waiting for the host.</p>"
      }
    </main>`;
  }

  private renderSummary(snapshot: RoomSnapshot, continued: boolean) {
    return `${this.renderScores(snapshot)}
      <main class="mp-game-layout mp-summary-layout">
        <section class="mp-board-panel mp-paper">
          ${snapshot.puzzle ? this.renderBoard(snapshot.puzzle, snapshot) : ""}
          <p class="mp-feedback" role="status">${escapeHtml(this.feedback)}</p>
        </section>
        <aside class="mp-paper mp-summary-card">
          <h1>${snapshot.puzzle?.skipped ? "Puzzle Skipped" : "Puzzle Complete"}</h1>
          <p>Puzzle ${snapshot.puzzleIndex + 1} of ${snapshot.puzzleCount}</p>
          <p>Review the completed board, then continue when you are ready.</p>
          <button class="mp-primary" data-action="continue" ${continued ? "disabled" : ""}>${continued ? "Waiting for others…" : "Continue"}</button>
        </aside>
      </main>
      ${this.renderBonusInfo(snapshot)}
      ${this.showBonusList ? this.renderBonusListPopup(snapshot) : ""}`;
  }

  private renderResults(snapshot: RoomSnapshot, host: boolean) {
    const sorted = [...snapshot.participants].sort(
      (left, right) => right.score.total - left.score.total || left.seat - right.seat
    );
    let priorScore: number | null = null;
    let rank = 0;
    return `<main class="mp-paper mp-results">
      <h1>${snapshot.status === "completed" ? "High Scores" : "Session Ended"}</h1>
      <div class="mp-ranking">
        ${sorted
          .map((participant, index) => {
            if (participant.score.total !== priorScore) rank = index + 1;
            priorScore = participant.score.total;
            return `<article style="--player-color:${participant.color}">
              <b>#${rank}</b><span class="mp-color-dot"></span>
              <strong>${escapeHtml(participant.name)}</strong>
              <span>${participant.kind === "ai" ? `AI · ${escapeHtml(participant.aiLevel ?? "College")}` : "Human"}</span>
              <small>L ${participant.score.letters} · E ${participant.score.emerald} · D ${participant.score.diamond} · R ${participant.score.ruby}</small>
              <em>${participant.score.total}</em>
            </article>`;
          })
          .join("")}
      </div>
      <div class="mp-actions">
        ${host ? `<button class="mp-primary" data-action="rematch">Rematch</button>` : ""}
        <button data-action="leave">Leave</button>
      </div>
    </main>`;
  }

  private handleClick(event: Event) {
    this.unlockAudio();
    const button = (event.target as HTMLElement).closest<HTMLElement>("[data-action]");
    const snapshot = this.snapshot;
    if (!button || !snapshot) return;
    const action = button.dataset.action;
    this.playSound("click");
    const local = snapshot.participants.find((participant) => participant.id === snapshot.localParticipantId);
    const turnLocked =
      snapshot.settings.playMode === "Turn Based" &&
      (!this.isLocalTurn(snapshot) || this.isTurnOrderPopupActive()) &&
      ["letter", "clear", "shuffle", "submit", "hint", "board-cell", "skip"].includes(action ?? "");
    if (turnLocked) {
      this.setError("Wait for your turn.");
      this.playSound("error");
      return;
    }
    if (action === "leave") return this.onLeave();
    if (action === "copy-invite") {
      const invite = `${window.location.origin}/wordpuzzle/room/${snapshot.roomCode}`;
      void navigator.clipboard.writeText(invite).then(
        () => this.setError("Invite link copied."),
        () => this.setError(invite)
      );
    } else if (action === "toggle-ready") {
      this.client.send({ type: "set-ready", ready: !local?.ready });
    } else if (action === "start") {
      const blocker = snapshot.participants.find(
        (participant) => participant.kind === "human" && (!participant.connected || !participant.ready)
      );
      if (blocker) {
        this.setError(
          blocker.connected
            ? `${blocker.name} is not ready yet.`
            : `${blocker.name} is disconnected. Reconnect them or replace them with AI.`
        );
        this.playSound("error");
        return;
      }
      if (this.client.send({ type: "start-session" })) {
        this.feedback = "Starting session…";
        this.render();
        if (this.startSessionTimeout !== null) {
          window.clearTimeout(this.startSessionTimeout);
        }
        this.startSessionTimeout = window.setTimeout(() => {
          this.startSessionTimeout = null;
          const current = this.snapshot;
          if (!current || current.status !== "lobby") return;
          this.client.send({ type: "request-snapshot" });
          this.setError("Still waiting for session start confirmation. Try Start Session again if needed.");
        }, 3000);
      }
    } else if (action === "add-ai") {
      const level = (document.getElementById("mp-ai-level") as HTMLSelectElement)?.value as AiLevel;
      this.client.send({ type: "add-ai", level });
    } else if (action === "remove-ai") {
      this.client.send({ type: "remove-ai", participantId: button.dataset.participant ?? "" });
    } else if (action === "letter") {
      const index = Number(button.dataset.index);
      const selectedIndex = this.selectedIndices.indexOf(index);
      if (selectedIndex >= 0) this.selectedIndices.splice(selectedIndex, 1);
      else this.selectedIndices.push(index);
      this.render();
    } else if (action === "clear") {
      this.clearGuess();
      this.render();
    } else if (action === "shuffle") {
      this.wheelLetters.sort(() => Math.random() - 0.5);
      this.clearGuess();
      this.render();
    } else if (action === "submit") {
      const guess = this.currentGuess();
      if (guess.length >= 3 && this.client.send({ type: "submit-guess", guess })) {
        this.clearGuess();
      } else {
        this.playSound("error");
      }
    } else if (action === "hint") {
      const hint = button.dataset.hint as HintKind;
      if (hint === "letter") {
        this.awaitingLetterHint = true;
        this.feedback = "Choose an unrevealed board letter.";
        this.render();
      } else {
        this.client.send({ type: "use-hint", hint });
      }
    } else if (action === "board-cell" && this.awaitingLetterHint) {
      this.awaitingLetterHint = false;
      this.client.send({
        type: "use-hint",
        hint: "letter",
        wordId: button.dataset.word,
        position: Number(button.dataset.position)
      });
    } else if (action === "skip") {
      this.client.send({ type: snapshot.skipVote ? "accept-skip" : "request-skip" });
    } else if (action === "toggle-bonus-list") {
      this.showBonusList = !this.showBonusList;
      this.render();
    } else if (action === "close-bonus-list") {
      this.showBonusList = false;
      this.render();
    } else if (action === "continue") {
      this.client.send({ type: "continue" });
    } else if (action === "resume") {
      this.client.send({ type: "resume" });
    } else if (action === "replace") {
      const level = (document.getElementById("mp-replace-level") as HTMLSelectElement)?.value as AiLevel;
      this.client.send({
        type: "replace-with-ai",
        participantId: button.dataset.participant ?? "",
        level
      });
    } else if (action === "end") {
      this.client.send({ type: "end-session" });
    } else if (action === "rematch") {
      this.client.send({ type: "rematch" });
    }
  }

  private handleChange(event: Event) {
    const select = (event.target as HTMLElement).closest<HTMLSelectElement>("[data-setting]");
    const snapshot = this.snapshot;
    if (!select || !snapshot) return;
    const settings = { ...snapshot.settings };
    if (select.dataset.setting === "mode") settings.mode = select.value as typeof settings.mode;
    if (select.dataset.setting === "playMode") settings.playMode = select.value as typeof settings.playMode;
    if (select.dataset.setting === "turnTimeLimit") {
      settings.turnTimeLimit =
        select.value === "Not Timed" ? "Not Timed" : (Number(select.value) as typeof settings.turnTimeLimit);
    }
    if (select.dataset.setting === "difficulty") {
      settings.difficulty = select.value as typeof settings.difficulty;
    }
    if (select.dataset.setting === "capacity") {
      settings.capacity = Number(select.value) as typeof settings.capacity;
    }
    this.client.send({ type: "update-settings", settings, expectedRevision: snapshot.revision });
  }

  private handleKeyDown = (event: KeyboardEvent) => {
    this.unlockAudio();
    const snapshot = this.snapshot;
    if (!snapshot || snapshot.status !== "playing" || snapshot.paused) return;
    if (
      snapshot.settings.playMode === "Turn Based" &&
      (!this.isLocalTurn(snapshot) || this.isTurnOrderPopupActive())
    ) {
      return;
    }
    if (event.key === "Enter") {
      const guess = this.currentGuess();
      if (guess.length >= 3 && this.client.send({ type: "submit-guess", guess })) {
        this.playSound("click");
        this.clearGuess();
      } else {
        this.playSound("error");
      }
      return;
    }
    if (event.key === "Backspace") {
      this.playSound("click");
      this.selectedIndices.pop();
      this.render();
      return;
    }
    if (!/^[a-z]$/i.test(event.key)) return;
    const index = this.wheelLetters.findIndex(
      (letter, candidate) =>
        letter.toUpperCase() === event.key.toUpperCase() && !this.selectedIndices.includes(candidate)
    );
    if (index >= 0) {
      this.playSound("select");
      this.selectedIndices.push(index);
      this.render();
    }
  };

  private currentGuess() {
    return this.selectedIndices.map((index) => this.wheelLetters[index]).join("");
  }

  private clearGuess() {
    this.selectedIndices = [];
  }

  private ensureAnimationLayer() {
    if (this.animationLayer && document.body.contains(this.animationLayer)) return;
    this.animationLayer = document.createElement("div");
    this.animationLayer.className = "mp-score-fx-layer";
    document.body.appendChild(this.animationLayer);
  }

  private processPresentationEvents(
    events: PresentationEvent[],
    snapshot: RoomSnapshot,
    previousSnapshot: RoomSnapshot | null
  ) {
    let uiChanged = false;
    const newCompletedWordIds = this.collectNewlyCompletedWordIds(snapshot, previousSnapshot);
    const pendingWordIds = [...newCompletedWordIds];

    for (const event of events) {
      if (this.processedEventSequences.has(event.sequence)) continue;
      this.processedEventSequences.add(event.sequence);

      if (event.type === "word-solved") this.playSound("place");
      else if (event.type === "bonus-claimed") this.playSound("hintUsed");
      else if (event.type === "hint-used") this.playSound("hintUsed");
      else if (event.type === "puzzle-skipped" || event.type === "guess-rejected") this.playSound("error");
      else if (event.type === "turn-order") {
        uiChanged = true;
      } else if (event.type === "turn-advanced") {
        uiChanged = true;
      }

      if (!event.actorId || !event.points || event.points <= 0) continue;
      if (event.type !== "word-solved" && event.type !== "hint-used") continue;

      const wordIdsForEvent =
        event.type === "word-solved" ? pendingWordIds.splice(0, 1) : pendingWordIds.splice(0);
      const components = this.calculateFlightComponents(snapshot, wordIdsForEvent, event.points);
      this.spawnScoreFlight(event.actorId, event.points, components, wordIdsForEvent);
    }

    if (this.processedEventSequences.size > 500) {
      const keep = new Set<number>();
      const sorted = [...this.processedEventSequences].sort((left, right) => right - left);
      sorted.slice(0, 250).forEach((seq) => keep.add(seq));
      this.processedEventSequences = keep;
    }
    return uiChanged;
  }

  private collectNewlyCompletedWordIds(
    snapshot: RoomSnapshot,
    previousSnapshot: RoomSnapshot | null
  ): string[] {
    if (!snapshot.puzzle || !previousSnapshot?.puzzle || snapshot.puzzle.id !== previousSnapshot.puzzle.id) {
      return [];
    }
    const previousMap = new Map(previousSnapshot.puzzle.words.map((word) => [word.id, word.completed]));
    return snapshot.puzzle.words
      .filter((word) => word.completed && !previousMap.get(word.id))
      .map((word) => word.id);
  }

  private calculateFlightComponents(
    snapshot: RoomSnapshot,
    wordIds: string[],
    fallbackTotal: number
  ): ScoreFlightComponents {
    if (!snapshot.puzzle || wordIds.length === 0) {
      return { letters: fallbackTotal, emerald: 0, ruby: 0, diamond: 0, strongestGem: "none" };
    }

    let letters = 0;
    let emerald = 0;
    let ruby = 0;
    let diamond = 0;
    let strongestGem: GemName = "none";

    for (const wordId of wordIds) {
      const word = snapshot.puzzle.words.find((candidate) => candidate.id === wordId);
      if (!word) continue;
      letters += word.length;
      for (const gem of word.gems) {
        if (gem === "emerald") emerald += GEM_BONUS.emerald;
        else if (gem === "ruby") ruby += GEM_BONUS.ruby;
        else if (gem === "diamond") diamond += GEM_BONUS.diamond;
        if (GEM_RANK[gem] > GEM_RANK[strongestGem]) strongestGem = gem as GemName;
      }
    }

    const combined = letters + emerald + ruby + diamond;
    if (combined <= 0) {
      return { letters: fallbackTotal, emerald: 0, ruby: 0, diamond: 0, strongestGem: "none" };
    }
    if (combined < fallbackTotal) letters += fallbackTotal - combined;
    return { letters, emerald, ruby, diamond, strongestGem };
  }

  private spawnScoreFlight(
    participantId: string,
    points: number,
    components: ScoreFlightComponents,
    wordIds: string[]
  ) {
    const snapshot = this.snapshot;
    if (!snapshot || !this.animationLayer) return;

    const source = this.resolveFlightSource(snapshot, wordIds);
    const destination = this.resolveFlightDestination(participantId);
    if (!destination) return;

    const ownerColor =
      snapshot.participants.find((participant) => participant.id === participantId)?.color ?? "#526b3d";
    const strongestGem = components.strongestGem;
    const reducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    const travelMs = reducedMotion ? 260 : Math.max(800, Math.min(1300, 760 + Math.hypot(destination.x - source.x, destination.y - source.y) * 0.4));
    const holdMs = reducedMotion ? 120 : 180;

    const flight = document.createElement("div");
    flight.className = `mp-score-flight gem-${strongestGem}`;
    flight.style.setProperty("--owner-color", ownerColor);
    flight.innerHTML = `
      <div class="mp-score-flight-total">+${points}</div>
      <div class="mp-score-flight-breakdown">
        <span>L+${components.letters}</span>
        ${components.emerald > 0 ? `<span class="gem-emerald">E+${components.emerald}</span>` : ""}
        ${components.diamond > 0 ? `<span class="gem-diamond">D+${components.diamond}</span>` : ""}
        ${components.ruby > 0 ? `<span class="gem-ruby">R+${components.ruby}</span>` : ""}
      </div>`;
    this.animationLayer.appendChild(flight);

    if (reducedMotion) {
      flight.classList.add("reduced");
      flight.style.setProperty("--from-x", `${source.x}px`);
      flight.style.setProperty("--from-y", `${source.y}px`);
      window.setTimeout(() => {
        flight.remove();
        this.triggerScoreImpact(participantId, components, strongestGem);
      }, travelMs);
      return;
    }

    const dx = destination.x - source.x;
    const dy = destination.y - source.y;
    const control = {
      x: source.x + dx * 0.45 + Math.max(-90, Math.min(90, dx * 0.12)),
      y: Math.min(source.y, destination.y) - Math.max(56, Math.abs(dx) * 0.18)
    };

    const startAt = performance.now();
    let lastTrailAt = startAt;
    const frame = () => {
      const elapsed = performance.now() - startAt;
      if (elapsed < holdMs) {
        const pop = elapsed / holdMs;
        const scale = 0.82 + 0.32 * Math.sin(pop * Math.PI * 0.5);
        flight.style.transform = `translate(${source.x}px, ${source.y}px) scale(${scale})`;
        requestAnimationFrame(frame);
        return;
      }

      const tRaw = Math.min(1, (elapsed - holdMs) / travelMs);
      const t = tRaw * tRaw * (3 - 2 * tRaw);
      const x = (1 - t) * (1 - t) * source.x + 2 * (1 - t) * t * control.x + t * t * destination.x;
      const y = (1 - t) * (1 - t) * source.y + 2 * (1 - t) * t * control.y + t * t * destination.y;
      const scale = 1.08 - t * 0.18;
      flight.style.transform = `translate(${x}px, ${y}px) scale(${scale})`;
      flight.style.opacity = `${1 - t * 0.68}`;

      const now = performance.now();
      if (now - lastTrailAt > 40) {
        lastTrailAt = now;
        this.spawnTrailDot(x, y, ownerColor, strongestGem);
      }

      if (tRaw < 1) {
        requestAnimationFrame(frame);
      } else {
        flight.remove();
        this.triggerScoreImpact(participantId, components, strongestGem);
      }
    };

    requestAnimationFrame(frame);
  }

  private resolveFlightSource(snapshot: RoomSnapshot, wordIds: string[]) {
    const board = this.root.querySelector<HTMLElement>(".mp-board");
    if (!board || !snapshot.puzzle || wordIds.length === 0) {
      const bounds = board?.getBoundingClientRect();
      if (bounds) return { x: bounds.left + bounds.width * 0.5, y: bounds.top + bounds.height * 0.5 };
      return { x: window.innerWidth * 0.5, y: window.innerHeight * 0.5 };
    }

    const centers: Array<{ x: number; y: number }> = [];
    for (const wordId of wordIds) {
      const word = snapshot.puzzle.words.find((entry) => entry.id === wordId);
      if (!word) continue;
      for (const cell of word.cells) {
        const cellElement = this.root.querySelector<HTMLElement>(`.mp-cell[data-row="${cell.row}"][data-col="${cell.col}"]`);
        if (!cellElement) continue;
        const rect = cellElement.getBoundingClientRect();
        centers.push({ x: rect.left + rect.width * 0.5, y: rect.top + rect.height * 0.5 });
      }
    }

    if (centers.length === 0) {
      const bounds = board.getBoundingClientRect();
      return { x: bounds.left + bounds.width * 0.5, y: bounds.top + bounds.height * 0.5 };
    }

    return {
      x: centers.reduce((sum, point) => sum + point.x, 0) / centers.length,
      y: centers.reduce((sum, point) => sum + point.y, 0) / centers.length
    };
  }

  private resolveFlightDestination(participantId: string) {
    const card = this.root.querySelector<HTMLElement>(`.mp-score-strip article[data-participant-id="${participantId}"]`);
    if (!card) return null;
    const totalEl = card.querySelector<HTMLElement>(".mp-score-total");
    const target = (totalEl ?? card).getBoundingClientRect();
    return { x: target.left + target.width * 0.5, y: target.top + target.height * 0.55 };
  }

  private triggerScoreImpact(participantId: string, components: ScoreFlightComponents, gem: GemName) {
    const card = this.root.querySelector<HTMLElement>(`.mp-score-strip article[data-participant-id="${participantId}"]`);
    if (!card) return;
    const total = card.querySelector<HTMLElement>('[data-score-field="total"]');
    if (!total) return;

    card.classList.remove("mp-score-impact-panel");
    total.classList.remove("mp-score-impact-total", "gem-emerald", "gem-ruby", "gem-diamond");
    // Force restart on repeated impacts.
    // eslint-disable-next-line @typescript-eslint/no-unused-expressions
    total.offsetHeight;
    card.classList.add("mp-score-impact-panel");
    total.classList.add("mp-score-impact-total");
    if (gem !== "none") total.classList.add(`gem-${gem}`);

    const flashField = (field: "letters" | "emerald" | "diamond" | "ruby", active: boolean) => {
      if (!active) return;
      const el = card.querySelector<HTMLElement>(`[data-score-field="${field}"]`);
      if (!el) return;
      el.classList.remove("mp-score-impact-field");
      // eslint-disable-next-line @typescript-eslint/no-unused-expressions
      el.offsetHeight;
      el.classList.add("mp-score-impact-field");
      window.setTimeout(() => el.classList.remove("mp-score-impact-field"), 420);
    };

    flashField("letters", components.letters > 0);
    flashField("emerald", components.emerald > 0);
    flashField("diamond", components.diamond > 0);
    flashField("ruby", components.ruby > 0);

    const totalRect = total.getBoundingClientRect();
    for (let i = 0; i < 8; i += 1) {
      const angle = (Math.PI * 2 * i) / 8;
      const x = totalRect.left + totalRect.width * 0.5 + Math.cos(angle) * 12;
      const y = totalRect.top + totalRect.height * 0.5 + Math.sin(angle) * 7;
      this.spawnTrailDot(x, y, getComputedStyle(card).getPropertyValue("--player-color") || "#526b3d", gem, true);
    }

    this.playSound("place");
    window.setTimeout(() => {
      card.classList.remove("mp-score-impact-panel");
      total.classList.remove("mp-score-impact-total", "gem-emerald", "gem-ruby", "gem-diamond");
    }, 460);
  }

  private spawnTrailDot(x: number, y: number, ownerColor: string, gem: GemName, burst = false) {
    if (!this.animationLayer) return;
    const dot = document.createElement("span");
    dot.className = `mp-score-trail gem-${gem}${burst ? " burst" : ""}`;
    dot.style.setProperty("--owner-color", ownerColor.trim() || "#526b3d");
    dot.style.transform = `translate(${x}px, ${y}px)`;
    this.animationLayer.appendChild(dot);
    window.setTimeout(() => dot.remove(), burst ? 460 : 360);
  }

  private unlockAudio() {
    if (this.audioUnlocked) return;
    this.audioUnlocked = true;
    const firstSound = this.sounds.click;
    if (!firstSound) return;
    firstSound.muted = true;
    firstSound.currentTime = 0;
    firstSound
      .play()
      .then(() => {
        firstSound.pause();
        firstSound.currentTime = 0;
        firstSound.muted = false;
      })
      .catch(() => {
        firstSound.muted = false;
      });
  }

  private playSound(name: keyof typeof Assets.sounds) {
    const sound = this.sounds[name];
    if (!sound) return;
    sound.currentTime = 0;
    sound.play().catch(() => undefined);
  }
}

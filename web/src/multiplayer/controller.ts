import type {
  AiLevel,
  HintKind,
  PresentationEvent,
  PublicPuzzle,
  RoomSnapshot
} from "../../../shared/multiplayer/types";
import { Assets } from "../assets";
import { playSpellTone, playWordSuccess, sampleVolume, unlockGameAudio } from "../core/gameAudio";
import { MultiplayerClient } from "./client";

const AI_LEVELS: AiLevel[] = ["High School", "College", "Professional"];
const HINTS: Array<{ kind: HintKind; label: string; cost: number; description: string }> = [
  { kind: "letter", label: "Letter", cost: 5, description: "Pick one unrevealed tile and reveal exactly that letter." },
  { kind: "random", label: "Random", cost: 10, description: "Reveal one random unrevealed letter in every unsolved word." },
  { kind: "full-word", label: "Full Word", cost: 15, description: "Reveal every unrevealed letter in one word." },
  { kind: "first-of-each", label: "1st Ltr All", cost: 20, description: "Reveal the next unrevealed letter in every unsolved word." }
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
const rarityGem = (rarity: number) => (rarity >= 4 ? "diamond" : rarity === 3 ? "ruby" : rarity === 2 ? "emerald" : "none");
const safeWordGems = (word: { gems?: string[]; length: number; rarity?: number }) =>
  Array.isArray(word.gems) && word.gems.length > 0
    ? word.gems
    : Array.from({ length: word.length }, () => rarityGem(word.rarity ?? 0));
const gemArtwork = (gem: string) =>
  gem === "emerald" ? Assets.sapphire : gem === "ruby" ? Assets.ruby : gem === "diamond" ? Assets.diamond : "";

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
  private awaitingFullWordHint = false;
  private showBonusList = false;
  private sounds: Record<string, HTMLAudioElement> = {};
  private audioUnlocked = false;
  private processedEventSequences = new Set<number>();
  private animationLayer: HTMLDivElement | null = null;
  private turnOrderPopup: { roundNumber: number; names: string[]; hideAt: number } | null = null;
  private uiTicker: number | null = null;
  private shownTurnOrderKey: string | null = null;
  private startSessionTimeout: number | null = null;
  private draggingWheel = false;
  private wheelPointerId: number | null = null;
  private suppressNextLetterClick = false;
  private pinnedWordInfoId: string | null = null;
  private readonly handleViewportResize = () => this.fitBoardToViewport();

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
    root.addEventListener("pointerdown", this.handleWheelPointerDown);
    root.addEventListener("pointermove", this.handleWheelPointerMove);
    root.addEventListener("pointerup", this.handleWheelPointerUp);
    root.addEventListener("pointercancel", this.handleWheelPointerCancel);
    root.addEventListener("pointerover", this.handleWordInfoPointerOver);
    root.addEventListener("pointerout", this.handleWordInfoPointerOut);
    root.addEventListener("focusin", this.handleWordInfoFocusIn);
    root.addEventListener("focusout", this.handleWordInfoFocusOut);
    window.addEventListener("keydown", this.handleKeyDown);
    window.addEventListener("resize", this.handleViewportResize);
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
    window.removeEventListener("resize", this.handleViewportResize);
    this.root.removeEventListener("pointerdown", this.handleWheelPointerDown);
    this.root.removeEventListener("pointermove", this.handleWheelPointerMove);
    this.root.removeEventListener("pointerup", this.handleWheelPointerUp);
    this.root.removeEventListener("pointercancel", this.handleWheelPointerCancel);
    this.root.removeEventListener("pointerover", this.handleWordInfoPointerOver);
    this.root.removeEventListener("pointerout", this.handleWordInfoPointerOut);
    this.root.removeEventListener("focusin", this.handleWordInfoFocusIn);
    this.root.removeEventListener("focusout", this.handleWordInfoFocusOut);
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
      this.awaitingLetterHint = false;
      this.awaitingFullWordHint = false;
      this.showBonusList = false;
      this.pinnedWordInfoId = null;
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
    // Replacing the wheel DOM during a drag would release pointer capture and break the gesture.
    // The pointer-up handler always renders the latest snapshot after the gesture finishes.
    if (this.draggingWheel) return;
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
      <section class="mp-shell mp-shell-${snapshot.status}">
        ${connectionBanner}
        <header class="mp-room-header">
          <button class="mp-link-button" data-action="leave">Leave</button>
          <strong>Room ${escapeHtml(snapshot.roomCode)}</strong>
          <span class="mp-room-details">${escapeHtml(snapshot.settings.mode)} · ${escapeHtml(snapshot.settings.playMode)} · ${escapeHtml(String(snapshot.settings.turnTimeLimit))} · ${escapeHtml(snapshot.settings.difficulty)} · ${escapeHtml(String(snapshot.settings.puzzlesPerRound))} Puzzles</span>
          <button class="mp-link-button" data-action="copy-invite">Copy Invite</button>
        </header>
        ${
          snapshot.status === "lobby"
            ? this.renderLobby(snapshot, host)
            : snapshot.status === "playing"
              ? this.renderPlay(snapshot, local?.hintCredits ?? 0, host)
              : snapshot.status === "puzzle-summary"
                ? this.renderSummary(snapshot, local?.hintCredits ?? 0, Boolean(local?.continued))
                : this.renderResults(snapshot, host)
        }
      </section>`;
    window.requestAnimationFrame(() => this.fitBoardToViewport());
    if (this.pinnedWordInfoId) this.showWordInfo(this.pinnedWordInfoId);
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
            <label class="mp-setting-option">
              <input
                type="checkbox"
                data-setting-option="includeBonusWordsWhenPossible"
                ${snapshot.settings.includeBonusWordsWhenPossible ? "checked" : ""}
                ${host ? "" : "disabled"}
              >
              <span>Include Bonus Words when possible</span>
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
            <label>Puzzles per Round
              <select data-setting="puzzlesPerRound" ${host ? "" : "disabled"}>
                ${[3, 4, 5, 6, 7].map((value) => `<option ${snapshot.settings.puzzlesPerRound === value ? "selected" : ""}>${value}</option>`).join("")}
              </select>
            </label>
            <fieldset class="mp-powerups">
              <legend>Enabled Power Ups</legend>
              <div class="mp-powerups-scroll">
                ${HINTS.map(
                  (hint) => `
                    <label class="mp-powerup-option" title="${escapeHtml(hint.description)}">
                      <input
                        type="checkbox"
                        data-setting-hint="${hint.kind}"
                        ${snapshot.settings.enabledPowerUps[hint.kind] ? "checked" : ""}
                        ${host ? "" : "disabled"}
                      >
                      <span>${hint.label}</span>
                    </label>`
                ).join("")}
              </div>
            </fieldset>
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
        ${this.renderFeedbackText(this.feedback)}
      </main>`;
  }

  private renderPlay(snapshot: RoomSnapshot, hintCredits: number, host: boolean) {
    const puzzle = snapshot.puzzle;
    if (!puzzle) return `<main class="mp-paper"><h1>Preparing puzzle…</h1></main>`;
    const localTurn = this.isLocalTurn(snapshot);
    const localPlayer = snapshot.participants.find(
      (participant) => participant.id === snapshot.localParticipantId
    );
    const puzzleProgress = ((snapshot.puzzleIndex + 1) / Math.max(1, snapshot.puzzleCount)) * 100;
    const failedWords = [...puzzle.failedWords].sort((left, right) => left.localeCompare(right));
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
          : `<main class="mp-play-layout">
              <section class="mp-board-panel mp-paper">
                <div class="mp-board-stage">
                  ${this.renderBoard(puzzle, snapshot)}
                  ${this.renderSpellPanel()}
                </div>
                ${this.renderFeedbackText(this.feedback)}
              </section>
              <section class="mp-sp-lower" aria-label="Puzzle controls">
                <aside class="mp-hint-panel mp-paper">
                  <div class="mp-panel-heading">
                    <strong>Bonus Words: ${puzzle.claimedBonusCount}/${puzzle.bonusWordCount}</strong>
                    <button class="mp-list-button" data-action="toggle-bonus-list" ${puzzle.bonusWordCount === 0 ? "disabled" : ""}>List</button>
                  </div>
                <div class="mp-hints">
                  ${HINTS.map(
                    (hint) => {
                      const disabledBySettings = !snapshot.settings.enabledPowerUps[hint.kind];
                      const disabledByTurn = controlsDisabled;
                      const disabledByCredits = hintCredits < hint.cost;
                      const disabled = disabledBySettings || disabledByTurn || disabledByCredits;

                      let reasonClass = "is-ready";
                      let badge = `${hint.cost}`;
                      let title = `${hint.description}\nCost ${hint.cost}`;

                      if (disabledBySettings) {
                        reasonClass = "is-off";
                        badge = "OFF";
                        title = `${hint.description}\nDisabled by room settings`;
                      } else if (disabledByTurn) {
                        reasonClass = "is-turn";
                        badge = "TURN";
                        title = `${hint.description}\nWait for your turn`;
                      } else if (disabledByCredits) {
                        reasonClass = "is-credits";
                        badge = `${hintCredits}/${hint.cost}`;
                        title = `${hint.description}\nNeed ${hint.cost} credits`;
                      }

                      return `<button data-action="hint" data-hint="${hint.kind}" class="${reasonClass}" title="${escapeHtml(title)}" ${
                        disabled ? "disabled" : ""
                      }><span class="mp-hint-dot" aria-hidden="true"></span><span>${hint.label}</span><small class="mp-hint-badge ${reasonClass}">${escapeHtml(badge)}</small></button>`;
                    }
                  ).join("")}
                </div>
                </aside>
                <div class="mp-wheel-console">
                  <div class="mp-wheel">
                    <svg class="mp-wheel-path" viewBox="0 0 100 100" preserveAspectRatio="none" aria-hidden="true"><polyline points=""></polyline></svg>
                    ${this.wheelLetters
                    .map(
                      (letter, index) =>
                        `<button style="--wheel-index:${index};--wheel-count:${this.wheelLetters.length}" data-action="letter" data-index="${index}" class="${this.selectedIndices.includes(index) ? "selected" : ""}" ${controlsDisabled ? "disabled" : ""} aria-label="Choose ${escapeHtml(letter)}" aria-pressed="${this.selectedIndices.includes(index)}">${escapeHtml(letter)}</button>`
                    )
                    .join("")}</div>
                  <div class="mp-control-row">
                    <button data-action="clear" ${controlsDisabled ? "disabled" : ""}>Clear</button>
                    <button data-action="shuffle" ${controlsDisabled ? "disabled" : ""}>↝ Shuffle</button>
                    <button class="mp-submit" data-action="submit" ${controlsDisabled ? "disabled" : ""}>Submit</button>
                    <button class="mp-mobile-utility" data-action="toggle-bonus-list" ${puzzle.bonusWordCount === 0 ? "disabled" : ""} aria-label="Show bonus words">Bonus</button>
                    <button class="mp-mobile-utility" data-action="skip" ${controlsDisabled ? "disabled" : ""}>Skip</button>
                  </div>
                  <button class="mp-skip-button" data-action="skip" ${controlsDisabled ? "disabled" : ""}>Request Skip</button>
                </div>
                <aside class="mp-progress-panel mp-paper">
                  ${this.renderCompactScores(snapshot)}
                  <div class="mp-progress-overview">
                    <strong class="mp-score-label">${escapeHtml(localPlayer?.name ?? "Your")} Score</strong>
                    <b class="mp-local-score">${localPlayer?.score.total ?? 0}</b>
                    <span class="mp-puzzle-label">Puzzle ${snapshot.puzzleIndex + 1} of ${snapshot.puzzleCount}</span>
                    <div class="mp-puzzle-meter" role="progressbar" aria-valuemin="0" aria-valuemax="${snapshot.puzzleCount}" aria-valuenow="${snapshot.puzzleIndex + 1}">
                      <span style="width:${puzzleProgress}%"></span>
                    </div>
                    <strong class="mp-hint-points">Hint Points: ${hintCredits}</strong>
                    <div class="mp-failed-summary">
                      <span>Failed words: <strong>${failedWords.length}</strong></span>
                      ${failedWords.length > 0 ? `<small>${failedWords.map((word) => escapeHtml(word)).join(", ")}</small>` : ""}
                    </div>
                  </div>
                </aside>
              </section>
            </main>`
      }
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
            <span data-score-field="ruby">R ${participant.score.ruby}</span> ·
            <span data-score-field="diamond">D ${participant.score.diamond}</span>
          </small>
        </article>`;
      })
      .join("")}</div>`;
  }

  private renderCompactScores(snapshot: RoomSnapshot) {
    const activeTurnId = snapshot.turnState?.activeParticipantId ?? "";
    return `<section class="mp-compact-scores" aria-label="Player scores">
      <strong class="mp-compact-scores-title">Players</strong>
      <div class="mp-compact-score-list">${[...snapshot.participants]
        .sort((left, right) => left.seat - right.seat)
        .map((participant) => {
          const activeClass =
            snapshot.settings.playMode === "Turn Based" && participant.id === activeTurnId ? "mp-active-turn" : "";
          const level = participant.kind === "ai" ? `AI · ${escapeHtml(participant.aiLevel ?? "College")}` : "Human";
          return `<article class="mp-compact-score-row ${activeClass}" style="--player-color:${participant.color}" data-participant-id="${participant.id}">
            <span class="mp-color-dot" aria-hidden="true"></span>
            <span class="mp-compact-player">
              <strong>${escapeHtml(participant.name)}</strong>
              <small>${level}</small>
            </span>
            <b class="mp-score-total" data-score-field="total">${participant.score.total}</b>
            <small class="mp-compact-breakdown">
              <span data-score-field="letters">L ${participant.score.letters}</span>
              <span data-score-field="emerald">E ${participant.score.emerald}</span>
              <span data-score-field="ruby">R ${participant.score.ruby}</span>
              <span data-score-field="diamond">D ${participant.score.diamond}</span>
            </small>
          </article>`;
        })
        .join("")}</div>
    </section>`;
  }

  private buildMobileCasualLayout(puzzle: PublicPuzzle) {
    if (puzzle.mode !== "Casual" || puzzle.words.length === 0) return null;

    const rows = Math.ceil(puzzle.words.length / 2);
    const columnWidths = [0, 0];
    puzzle.words.forEach((word, index) => {
      const column = Math.min(1, Math.floor(index / rows));
      columnWidths[column] = Math.max(columnWidths[column], word.cells.length);
    });

    const secondColumnOffset = columnWidths[0] + 1;
    const positions = new Map<string, { row: number; col: number }>();
    puzzle.words.forEach((word, index) => {
      const column = Math.min(1, Math.floor(index / rows));
      const row = index % rows;
      const startCol = column === 0 ? 0 : secondColumnOffset;
      word.cells.forEach((cell, position) => {
        positions.set(cellKey(cell.row, cell.col), { row, col: startCol + position });
      });
    });

    return {
      rows,
      cols: secondColumnOffset + columnWidths[1],
      dividerCols: columnWidths[1] > 0 ? [columnWidths[0]] : [],
      positions
    };
  }

  private renderBoard(puzzle: PublicPuzzle, snapshot: RoomSnapshot) {
    const cells = new Map<string, { row: number; col: number; refs: Array<{ wordId: string; position: number }> }>();
    const occupiedCols = new Set<number>();
    for (const word of puzzle.words) {
      word.cells.forEach((cell, position) => {
        const key = cellKey(cell.row, cell.col);
        const entry = cells.get(key) ?? { ...cell, refs: [] };
        entry.refs.push({ wordId: word.id, position });
        cells.set(key, entry);
        occupiedCols.add(cell.col);
      });
    }
    const dividerCols =
      puzzle.mode === "Casual"
        ? Array.from({ length: Math.max(0, puzzle.cols - 2) }, (_, index) => index + 1).filter(
            (col) => !occupiedCols.has(col)
          )
        : [];
    const mobileLayout = this.buildMobileCasualLayout(puzzle);
    const mobileRows = mobileLayout?.rows ?? puzzle.rows;
    const mobileCols = mobileLayout?.cols ?? puzzle.cols;
    return `<div class="mp-board-scroll" role="region" aria-label="Puzzle board" tabindex="0"><div class="mp-board ${mobileLayout ? "mp-mobile-reflow" : ""}" data-layout-rows="${mobileRows}" data-layout-cols="${mobileCols}" style="--rows:${puzzle.rows};--cols:${puzzle.cols};--mobile-rows:${mobileRows};--mobile-cols:${mobileCols}">
      ${dividerCols
        .map(
          (col) =>
            `<span class="mp-column-divider mp-desktop-divider" style="grid-row:1 / span ${puzzle.rows};grid-column:${col + 1};" aria-hidden="true"></span>`
        )
        .join("")}
      ${(mobileLayout?.dividerCols ?? [])
        .map(
          (col) =>
            `<span class="mp-column-divider mp-mobile-divider" style="--mobile-divider-col:${col + 1};grid-row:1 / span ${mobileRows};grid-column:${col + 1};" aria-hidden="true"></span>`
        )
        .join("")}
      ${[...cells.entries()]
        .map(([key, cell]) => {
          const visible = puzzle.visibleCells[key];
          const hintTarget = !visible && (this.awaitingLetterHint || this.awaitingFullWordHint);
          const owner = snapshot.participants.find((participant) => participant.id === visible?.ownerId);
          const ref = cell.refs.find(
            (candidate) => !puzzle.words.find((word) => word.id === candidate.wordId)?.completed
          ) ?? cell.refs[0];
          const completedWord = cell.refs
            .map((candidate) => puzzle.words.find((word) => word.id === candidate.wordId))
            .find((word) => word?.completed && word.answer);
          const gem = cell.refs
            .map((candidate) => {
              const word = puzzle.words.find((entry) => entry.id === candidate.wordId);
              if (!word) return "none";
              const gems = safeWordGems(word);
              return gems[candidate.position] ?? "none";
            })
            .reduce((best, current) => (GEM_RANK[current] > GEM_RANK[best] ? current : best), "none");
          const mobilePosition = mobileLayout?.positions.get(key) ?? { row: cell.row, col: cell.col };
          return `<button class="mp-cell ${visible ? "filled" : ""} ${hintTarget ? "mp-cell-hint-target" : ""} ${completedWord ? "has-word-info" : ""}" style="grid-row:${cell.row + 1};grid-column:${cell.col + 1};--mobile-row:${mobilePosition.row + 1};--mobile-col:${mobilePosition.col + 1};--owner-color:${owner?.color ?? "#5b4631"}"
            data-action="board-cell" data-word="${ref.wordId}" data-position="${ref.position}"
            ${completedWord ? `data-word-info="${escapeHtml(completedWord.id)}"` : ""}
            data-row="${cell.row}" data-col="${cell.col}"
            aria-label="${visible ? `${visible.letter}, owned by ${owner?.name ?? "player"}${completedWord ? "; definition available" : ""}` : "Unrevealed letter"}">
            ${
              !visible && gem !== "none"
                ? `<img class="mp-cell-gem" src="${escapeHtml(gemArtwork(gem))}" alt="" aria-hidden="true" title="${gem} word">`
                : ""
            }
            ${visible ? `<span class="mp-cell-letter">${escapeHtml(visible.letter)}</span>` : ""}
          </button>`;
        })
        .join("")}
    </div></div>
    <aside class="mp-word-info-popup mp-paper" role="tooltip" hidden>
      <button type="button" data-action="close-word-info" aria-label="Close word definition">&times;</button>
      <strong data-word-info-field="word"></strong>
      <span data-word-info-field="pos"></span>
      <p data-word-info-field="definition"></p>
      <p data-word-info-field="sentence"></p>
    </aside>`;
  }

  private fitBoardToViewport() {
    const puzzle = this.snapshot?.puzzle;
    const stage =
      this.root.querySelector<HTMLElement>(".mp-board-stage") ??
      this.root.querySelector<HTMLElement>(".mp-summary-layout .mp-board-scroll");
    const board = this.root.querySelector<HTMLElement>(".mp-board");
    if (!puzzle || !stage || !board || window.matchMedia("(min-width: 1501px)").matches) {
      board?.style.removeProperty("--cell-size");
      return;
    }

    const gap = 2;
    const horizontalSafetyInset = this.snapshot?.status === "puzzle-summary" ? 6 : 0;
    const phoneLayout = window.matchMedia("(max-width: 760px)").matches;
    const tabletGameplayShell = Boolean(
      board.closest(".mp-shell-playing, .mp-shell-puzzle-summary")
    );
    const portraitTabletLayout =
      tabletGameplayShell &&
      window.matchMedia(
        "(min-width: 761px) and (max-width: 1180px) and (orientation: portrait)"
      ).matches;
    const landscapeTabletLayout =
      tabletGameplayShell &&
      window.matchMedia(
        "(min-width: 761px) and (max-width: 1500px) and (orientation: landscape)"
      ).matches;
    const reflowedCasualLayout = phoneLayout || portraitTabletLayout;
    const layoutCols = reflowedCasualLayout ? Number(board.dataset.layoutCols) || puzzle.cols : puzzle.cols;
    const layoutRows = reflowedCasualLayout ? Number(board.dataset.layoutRows) || puzzle.rows : puzzle.rows;
    const availableWidth = Math.max(
      0,
      stage.clientWidth - horizontalSafetyInset - gap * Math.max(0, layoutCols - 1)
    );
    const availableHeight = Math.max(0, stage.clientHeight - gap * Math.max(0, layoutRows - 1));
    const contentSizedTabletLayout = portraitTabletLayout || landscapeTabletLayout;
    const maxCellSize = portraitTabletLayout ? 64 : landscapeTabletLayout ? 52 : 42;
    const cellSize = Math.max(
      1,
      contentSizedTabletLayout
        ? Math.min(maxCellSize, availableWidth / Math.max(1, layoutCols))
        : Math.min(maxCellSize, availableWidth / Math.max(1, layoutCols), availableHeight / Math.max(1, layoutRows))
    );
    board.style.setProperty("--cell-size", `${cellSize}px`);
  }

  private showWordInfo(wordId: string) {
    const word = this.snapshot?.puzzle?.words.find((candidate) => candidate.id === wordId);
    const popup = this.root.querySelector<HTMLElement>(".mp-word-info-popup");
    if (!word?.completed || !word.answer || !popup) return;
    const setText = (field: string, value: string) => {
      const element = popup.querySelector<HTMLElement>(`[data-word-info-field="${field}"]`);
      if (element) element.textContent = value;
    };
    setText("word", `Word: ${word.answer.toUpperCase()}`);
    setText("pos", `POS: ${word.pos || "N/A"}`);
    setText("definition", `Definition: ${word.definition || "N/A"}`);
    const sentence = popup.querySelector<HTMLElement>('[data-word-info-field="sentence"]');
    if (sentence) {
      sentence.textContent = word.sentence ? `Sentence: ${word.sentence}` : "";
      sentence.hidden = !word.sentence;
    }
    popup.hidden = false;
  }

  private hideWordInfo() {
    const popup = this.root.querySelector<HTMLElement>(".mp-word-info-popup");
    if (popup) popup.hidden = true;
  }

  private handleWordInfoPointerOver = (event: PointerEvent) => {
    if (event.pointerType === "touch" || this.pinnedWordInfoId) return;
    const cell = (event.target as HTMLElement).closest<HTMLElement>(".mp-cell[data-word-info]");
    if (cell?.dataset.wordInfo) this.showWordInfo(cell.dataset.wordInfo);
  };

  private handleWordInfoPointerOut = (event: PointerEvent) => {
    if (event.pointerType === "touch" || this.pinnedWordInfoId) return;
    const from = (event.target as HTMLElement).closest<HTMLElement>(".mp-cell[data-word-info]");
    const to = (event.relatedTarget as HTMLElement | null)?.closest?.<HTMLElement>(".mp-cell[data-word-info]");
    if (from && from.dataset.wordInfo !== to?.dataset.wordInfo) this.hideWordInfo();
  };

  private handleWordInfoFocusIn = (event: FocusEvent) => {
    if (this.pinnedWordInfoId) return;
    const cell = (event.target as HTMLElement).closest<HTMLElement>(".mp-cell[data-word-info]");
    if (cell?.dataset.wordInfo) this.showWordInfo(cell.dataset.wordInfo);
  };

  private handleWordInfoFocusOut = (event: FocusEvent) => {
    if (this.pinnedWordInfoId) return;
    const from = (event.target as HTMLElement).closest<HTMLElement>(".mp-cell[data-word-info]");
    const to = (event.relatedTarget as HTMLElement | null)?.closest?.<HTMLElement>(".mp-cell[data-word-info]");
    if (from && from.dataset.wordInfo !== to?.dataset.wordInfo) this.hideWordInfo();
  };

  private renderBonusInfo(snapshot: RoomSnapshot) {
    const puzzle = snapshot.puzzle;
    const found = puzzle?.claimedBonusCount ?? 0;
    const total = puzzle?.bonusWordCount ?? 0;
    const failed = [...(puzzle?.failedWords ?? [])].sort((left, right) => left.localeCompare(right));
    return `<section class="mp-bonus-info mp-paper">
      <span>Bonus words found: <strong>${found}/${total}</strong></span>
      <button data-action="toggle-bonus-list" ${total === 0 ? "disabled" : ""}>List</button>
    </section>
    <section class="mp-tried-info mp-paper">
      <span>Failed words: <strong>${failed.length}</strong></span>
      ${
        failed.length > 0
          ? `<div class="mp-tried-word-list">${failed
              .map((word) => `<span>${escapeHtml(word)}</span>`)
              .join("")}</div>`
          : `<p class="mp-tried-empty">No failed guesses yet.</p>`
      }
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

  private renderSummary(snapshot: RoomSnapshot, hintCredits: number, continued: boolean) {
    return `${this.renderScores(snapshot)}
      <div class="mp-play-status">
        <span>Puzzle ${snapshot.puzzleIndex + 1} of ${snapshot.puzzleCount}</span>
        <span style="font-weight:800;">Round ${snapshot.turnState?.roundNumber ?? 1}: Puzzle Complete</span>
        <span>Hint Credits: <strong>${hintCredits}</strong></span>
        <span>${escapeHtml(this.timeStatusText(snapshot))}</span>
      </div>
      <main class="mp-play-layout mp-summary-layout">
        <section class="mp-board-panel mp-paper">
          <div class="mp-board-stage">
            ${snapshot.puzzle ? this.renderBoard(snapshot.puzzle, snapshot) : ""}
          </div>
          ${this.renderFeedbackText(this.feedback)}
        <aside class="mp-paper mp-summary-card">
          <h1>${snapshot.puzzle?.skipped ? "Puzzle Skipped" : "Puzzle Complete"}</h1>
          <p>Puzzle ${snapshot.puzzleIndex + 1} of ${snapshot.puzzleCount}</p>
          <p>Review the completed board, then continue when you are ready.</p>
          <button class="mp-primary" data-action="continue" ${continued ? "disabled" : ""}>${continued ? "Waiting for others…" : "Continue"}</button>
        </aside>
        </section>
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
              <small>L ${participant.score.letters} · E ${participant.score.emerald} · R ${participant.score.ruby} · D ${participant.score.diamond}</small>
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

  private canUseWheel() {
    const snapshot = this.snapshot;
    if (!snapshot || snapshot.status !== "playing" || snapshot.paused) return false;
    return !(
      snapshot.settings.playMode === "Turn Based" &&
      (!this.isLocalTurn(snapshot) || this.isTurnOrderPopupActive())
    );
  }

  private handleWheelPointerDown = (event: PointerEvent) => {
    const button = (event.target as HTMLElement).closest<HTMLButtonElement>(".mp-wheel [data-action='letter']");
    if (!button || button.disabled || !event.isPrimary || (event.pointerType === "mouse" && event.button !== 0)) return;
    if (!this.canUseWheel()) {
      this.setError("Wait for your turn.");
      this.playSound("error");
      return;
    }

    const index = Number(button.dataset.index);
    if (!Number.isInteger(index) || index < 0 || index >= this.wheelLetters.length) return;

    this.unlockAudio();
    event.preventDefault();
    this.draggingWheel = true;
    this.wheelPointerId = event.pointerId;
    this.suppressNextLetterClick = true;
    this.selectedIndices = [index];
    this.root.setPointerCapture(event.pointerId);
    this.root.querySelector(".mp-wheel")?.classList.add("is-interacting");
    this.syncWheelGesture(event.clientX, event.clientY);
    playSpellTone(this.selectedIndices.length);
  };

  private handleWheelPointerMove = (event: PointerEvent) => {
    if (!this.draggingWheel || event.pointerId !== this.wheelPointerId) return;
    event.preventDefault();

    const index = this.wheelIndexAtPoint(event.clientX, event.clientY);
    if (index >= 0) {
      const existingIndex = this.selectedIndices.indexOf(index);
      if (existingIndex === -1) {
        this.selectedIndices.push(index);
        playSpellTone(this.selectedIndices.length);
      } else if (
        this.selectedIndices.length >= 2 &&
        this.selectedIndices[this.selectedIndices.length - 2] === index
      ) {
        this.selectedIndices.pop();
        playSpellTone(this.selectedIndices.length);
      }
    }
    this.syncWheelGesture(event.clientX, event.clientY);
  };

  private handleWheelPointerUp = (event: PointerEvent) => {
    if (!this.draggingWheel || event.pointerId !== this.wheelPointerId) return;
    event.preventDefault();
    this.finishWheelGesture(true);
  };

  private handleWheelPointerCancel = (event: PointerEvent) => {
    if (!this.draggingWheel || event.pointerId !== this.wheelPointerId) return;
    this.finishWheelGesture(false);
  };

  private wheelIndexAtPoint(clientX: number, clientY: number) {
    let closestIndex = -1;
    let closestDistance = Number.POSITIVE_INFINITY;
    const buttons = this.root.querySelectorAll<HTMLButtonElement>(".mp-wheel [data-action='letter']");
    buttons.forEach((button) => {
      const rect = button.getBoundingClientRect();
      const dx = clientX - (rect.left + rect.width / 2);
      const dy = clientY - (rect.top + rect.height / 2);
      const distance = Math.hypot(dx, dy);
      // Keep the hit circle close to the visible letter so neighbors stay distinct.
      const hitRadius = Math.max(rect.width, rect.height) * 0.58;
      const index = Number(button.dataset.index);
      if (distance <= hitRadius && distance < closestDistance && Number.isInteger(index)) {
        closestIndex = index;
        closestDistance = distance;
      }
    });
    return closestIndex;
  }

  private syncWheelGesture(pointerX?: number, pointerY?: number) {
    const wheel = this.root.querySelector<HTMLElement>(".mp-wheel");
    if (!wheel) return;

    const buttons = wheel.querySelectorAll<HTMLButtonElement>("[data-action='letter']");
    buttons.forEach((button) => {
      const selected = this.selectedIndices.includes(Number(button.dataset.index));
      button.classList.toggle("selected", selected);
      button.setAttribute("aria-pressed", String(selected));
    });

    this.syncSpellPanel();
    const wheelRect = wheel.getBoundingClientRect();
    const points = this.selectedIndices
      .map((index) => wheel.querySelector<HTMLButtonElement>(`[data-index="${index}"]`))
      .filter((button): button is HTMLButtonElement => Boolean(button))
      .map((button) => {
        const rect = button.getBoundingClientRect();
        const x = ((rect.left + rect.width / 2 - wheelRect.left) / wheelRect.width) * 100;
        const y = ((rect.top + rect.height / 2 - wheelRect.top) / wheelRect.height) * 100;
        return `${x.toFixed(2)},${y.toFixed(2)}`;
      });
    if (pointerX !== undefined && pointerY !== undefined) {
      const x = ((pointerX - wheelRect.left) / wheelRect.width) * 100;
      const y = ((pointerY - wheelRect.top) / wheelRect.height) * 100;
      points.push(`${x.toFixed(2)},${y.toFixed(2)}`);
    }
    wheel.querySelector("polyline")?.setAttribute("points", points.join(" "));
  }

  private finishWheelGesture(submit: boolean) {
    const pointerId = this.wheelPointerId;
    if (pointerId !== null && this.root.hasPointerCapture(pointerId)) {
      this.root.releasePointerCapture(pointerId);
    }
    this.draggingWheel = false;
    this.wheelPointerId = null;

    const guess = this.currentGuess();
    if (submit && guess.length >= 3) {
      if (this.client.send({ type: "submit-guess", guess })) {
        this.clearGuess();
      } else {
        this.playSound("error");
      }
    } else {
      this.clearGuess();
    }
    this.render();

    // Pointer gestures can synthesize a click after pointer-up. Ignore only that
    // click so keyboard activation of individual letters continues to work.
    window.setTimeout(() => {
      this.suppressNextLetterClick = false;
    }, 0);
  }

  private handleClick(event: Event) {
    this.unlockAudio();
    const button = (event.target as HTMLElement).closest<HTMLElement>("[data-action]");
    const snapshot = this.snapshot;
    if (!button || !snapshot) return;
    const action = button.dataset.action;
    if (action === "letter" && this.suppressNextLetterClick && event instanceof MouseEvent && event.detail > 0) {
      this.suppressNextLetterClick = false;
      return;
    }
    if (action !== "letter") this.playSound("click");
    const local = snapshot.participants.find((participant) => participant.id === snapshot.localParticipantId);
    const turnLocked =
      snapshot.settings.playMode === "Turn Based" &&
      (!this.isLocalTurn(snapshot) || this.isTurnOrderPopupActive()) &&
      (["letter", "clear", "shuffle", "submit", "hint", "skip"].includes(action ?? "") ||
        (action === "board-cell" && (this.awaitingLetterHint || this.awaitingFullWordHint)));
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
      playSpellTone(this.selectedIndices.length);
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
      if (!snapshot.settings.enabledPowerUps[hint]) {
        this.playSound("error");
        this.setError("That power-up is disabled for this session.");
        return;
      }
      if (hint === "letter") {
        this.awaitingLetterHint = true;
        this.awaitingFullWordHint = false;
        this.feedback = "Choose an unrevealed board letter.";
        this.render();
      } else if (hint === "full-word") {
        this.awaitingFullWordHint = true;
        this.awaitingLetterHint = false;
        this.feedback = "Choose any unrevealed board letter from the word you want solved.";
        this.render();
      } else {
        this.awaitingLetterHint = false;
        this.awaitingFullWordHint = false;
        this.client.send({ type: "use-hint", hint });
      }
    } else if (action === "board-cell" && this.awaitingLetterHint) {
      const cellKeyValue = `${button.dataset.row ?? ""},${button.dataset.col ?? ""}`;
      if (snapshot.puzzle?.visibleCells[cellKeyValue]) return;
      this.awaitingLetterHint = false;
      this.client.send({
        type: "use-hint",
        hint: "letter",
        wordId: button.dataset.word,
        position: Number(button.dataset.position)
      });
    } else if (action === "board-cell" && this.awaitingFullWordHint) {
      const cellKeyValue = `${button.dataset.row ?? ""},${button.dataset.col ?? ""}`;
      if (snapshot.puzzle?.visibleCells[cellKeyValue]) return;
      this.awaitingFullWordHint = false;
      this.client.send({
        type: "use-hint",
        hint: "full-word",
        wordId: button.dataset.word
      });
    } else if (action === "board-cell" && button.dataset.wordInfo) {
      const wordId = button.dataset.wordInfo;
      if (this.pinnedWordInfoId === wordId) {
        this.pinnedWordInfoId = null;
        this.hideWordInfo();
      } else {
        this.pinnedWordInfoId = wordId;
        this.showWordInfo(wordId);
      }
    } else if (action === "close-word-info") {
      this.pinnedWordInfoId = null;
      this.hideWordInfo();
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
    const optionToggle = (event.target as HTMLElement).closest<HTMLInputElement>("[data-setting-option]");
    const snapshot = this.snapshot;
    if (optionToggle && snapshot) {
      const settings = {
        ...snapshot.settings,
        includeBonusWordsWhenPossible: optionToggle.checked
      };
      this.client.send({ type: "update-settings", settings, expectedRevision: snapshot.revision });
      return;
    }

    const hintToggle = (event.target as HTMLElement).closest<HTMLInputElement>("[data-setting-hint]");
    if (hintToggle && snapshot) {
      const settings = {
        ...snapshot.settings,
        enabledPowerUps: {
          ...snapshot.settings.enabledPowerUps
        }
      };
      const hint = hintToggle.dataset.settingHint as HintKind;
      settings.enabledPowerUps[hint] = hintToggle.checked;
      this.client.send({ type: "update-settings", settings, expectedRevision: snapshot.revision });
      return;
    }

    const select = (event.target as HTMLElement).closest<HTMLSelectElement>("[data-setting]");
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
    if (select.dataset.setting === "puzzlesPerRound") {
      settings.puzzlesPerRound = Number(select.value) as typeof settings.puzzlesPerRound;
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
      this.selectedIndices.pop();
      playSpellTone(this.selectedIndices.length);
      this.render();
      return;
    }
    if (!/^[a-z]$/i.test(event.key)) return;
    const index = this.wheelLetters.findIndex(
      (letter, candidate) =>
        letter.toUpperCase() === event.key.toUpperCase() && !this.selectedIndices.includes(candidate)
    );
    if (index >= 0) {
      this.selectedIndices.push(index);
      playSpellTone(this.selectedIndices.length);
      this.render();
    }
  };

  private currentGuess() {
    return this.selectedIndices.map((index) => this.wheelLetters[index]).join("");
  }

  private spellPanelTiles(guess: string) {
    return [...guess]
      .map((letter) => `<span class="mp-spell-tile">${escapeHtml(letter)}</span>`)
      .join("");
  }

  private renderSpellPanel() {
    const guess = this.currentGuess();
    return `<div class="mp-spell-panel${guess ? " is-active" : ""}" aria-live="polite">${this.spellPanelTiles(guess)}</div>`;
  }

  private syncSpellPanel() {
    const panel = this.root.querySelector<HTMLElement>(".mp-spell-panel");
    if (!panel) return;
    const guess = this.currentGuess();
    panel.classList.toggle("is-active", guess.length > 0);
    panel.innerHTML = this.spellPanelTiles(guess);
  }

  private renderFeedbackText(message: string) {
    const withIcons = escapeHtml(message)
      .replace(/💚/g, `<img class="mp-feedback-gem" src="${escapeHtml(Assets.sapphire)}" alt="Emerald gem" aria-label="Emerald gem">`)
      .replace(/♦️/g, `<img class="mp-feedback-gem" src="${escapeHtml(Assets.ruby)}" alt="Ruby gem" aria-label="Ruby gem">`)
      .replace(/💎/g, `<img class="mp-feedback-gem" src="${escapeHtml(Assets.diamond)}" alt="Diamond gem" aria-label="Diamond gem">`);
    return `<p class="mp-feedback" role="status">${withIcons}</p>`;
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

      if (event.type === "word-solved") playWordSuccess();
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
      for (const gem of safeWordGems(word)) {
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
    const gridBounds = this.root.querySelector<HTMLElement>(".mp-board")?.getBoundingClientRect();
    const center = gridBounds
      ? {
          x: Math.round(gridBounds.left + gridBounds.width * 0.5),
          y: Math.round(gridBounds.top + gridBounds.height * 0.5)
        }
      : { x: Math.round(window.innerWidth * 0.5), y: Math.round(window.innerHeight * 0.5) };
    const centerTravelMs = 700;
    const centerHoldMs = 1400;
    const panelTravelMs = 460;
    const completedWords = wordIds
      .map((wordId) => snapshot.puzzle?.words.find((word) => word.id === wordId)?.answer)
      .filter((answer): answer is string => Boolean(answer))
      .map((answer) => answer.toUpperCase());
    const wordLabel = completedWords.join(" + ");

    const gemScore = (gem: Exclude<GemName, "none">, score: number) => `
      <span class="gem-${gem}">
        <img class="mp-score-flight-gem" src="${escapeHtml(gemArtwork(gem))}" alt="${gem} gem">
        <span>+${score}</span>
      </span>`;

    const flight = document.createElement("div");
    flight.className = `mp-score-flight gem-${strongestGem}`;
    flight.style.setProperty("--owner-color", ownerColor);
    flight.innerHTML = `
      ${wordLabel ? `<div class="mp-score-flight-word">${escapeHtml(wordLabel)}</div>` : ""}
      <div class="mp-score-flight-total">+${points}</div>
      <div class="mp-score-flight-breakdown">
        <span>Letters +${components.letters}</span>
        ${components.emerald > 0 ? gemScore("emerald", components.emerald) : ""}
        ${components.ruby > 0 ? gemScore("ruby", components.ruby) : ""}
        ${components.diamond > 0 ? gemScore("diamond", components.diamond) : ""}
      </div>`;
    this.animationLayer.appendChild(flight);

    if (reducedMotion) {
      flight.classList.add("reduced");
      flight.style.setProperty("--center-x", `${center.x}px`);
      flight.style.setProperty("--center-y", `${center.y}px`);
      flight.style.animationDuration = `${centerHoldMs}ms`;
      window.setTimeout(() => {
        flight.remove();
        this.triggerScoreImpact(participantId, components, strongestGem);
      }, centerHoldMs);
      return;
    }

    const startAt = performance.now();
    let lastTrailAt = startAt;
    const smoothstep = (value: number) => value * value * (3 - 2 * value);
    const positionFlight = (x: number, y: number, scale: number, opacity = 1) => {
      flight.style.transform = `translate(${x}px, ${y}px) translate(-50%, -50%) scale(${scale})`;
      flight.style.opacity = `${opacity}`;
    };
    const frame = () => {
      const elapsed = performance.now() - startAt;
      if (elapsed < centerTravelMs) {
        const progress = smoothstep(elapsed / centerTravelMs);
        const x = source.x + (center.x - source.x) * progress;
        const y = source.y + (center.y - source.y) * progress;
        positionFlight(x, y, 0.62 + progress * 0.38, Math.min(1, 0.3 + progress * 1.4));
        const now = performance.now();
        if (now - lastTrailAt > 55) {
          lastTrailAt = now;
          this.spawnTrailDot(x, y, ownerColor, strongestGem);
        }
        requestAnimationFrame(frame);
        return;
      }

      if (elapsed < centerTravelMs + centerHoldMs) {
        positionFlight(center.x, center.y, 1);
        requestAnimationFrame(frame);
        return;
      }

      const rawProgress = Math.min(
        1,
        (elapsed - centerTravelMs - centerHoldMs) / panelTravelMs
      );
      const progress = smoothstep(rawProgress);
      const x = center.x + (destination.x - center.x) * progress;
      const y = center.y + (destination.y - center.y) * progress;
      positionFlight(x, y, 1 - progress * 0.45, 1 - progress * 0.5);

      const now = performance.now();
      if (now - lastTrailAt > 35) {
        lastTrailAt = now;
        this.spawnTrailDot(x, y, ownerColor, strongestGem);
      }

      if (rawProgress < 1) {
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
    const card = this.findVisibleScoreCard(participantId);
    if (!card) return null;
    const totalEl = card.querySelector<HTMLElement>(".mp-score-total");
    const target = (totalEl ?? card).getBoundingClientRect();
    return { x: target.left + target.width * 0.5, y: target.top + target.height * 0.55 };
  }

  private triggerScoreImpact(participantId: string, components: ScoreFlightComponents, gem: GemName) {
    const card = this.findVisibleScoreCard(participantId);
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
    flashField("ruby", components.ruby > 0);
    flashField("diamond", components.diamond > 0);

    const totalRect = total.getBoundingClientRect();
    for (let i = 0; i < 8; i += 1) {
      const angle = (Math.PI * 2 * i) / 8;
      const x = totalRect.left + totalRect.width * 0.5 + Math.cos(angle) * 12;
      const y = totalRect.top + totalRect.height * 0.5 + Math.sin(angle) * 7;
      this.spawnTrailDot(x, y, getComputedStyle(card).getPropertyValue("--player-color") || "#526b3d", gem, true);
    }

    window.setTimeout(() => {
      card.classList.remove("mp-score-impact-panel");
      total.classList.remove("mp-score-impact-total", "gem-emerald", "gem-ruby", "gem-diamond");
    }, 460);
  }

  private findVisibleScoreCard(participantId: string) {
    const cards = this.root.querySelectorAll<HTMLElement>(
      `[data-participant-id="${participantId}"].mp-compact-score-row, .mp-score-strip article[data-participant-id="${participantId}"]`
    );
    return [...cards].find((card) => card.getClientRects().length > 0) ?? null;
  }

  private spawnTrailDot(x: number, y: number, ownerColor: string, gem: GemName, burst = false) {
    if (!this.animationLayer) return;
    const dot = document.createElement("span");
    dot.className = `mp-score-trail gem-${gem}${burst ? " burst" : ""}`;
    dot.style.setProperty("--owner-color", ownerColor.trim() || "#526b3d");
    dot.style.left = `${x}px`;
    dot.style.top = `${y}px`;
    this.animationLayer.appendChild(dot);
    window.setTimeout(() => dot.remove(), burst ? 460 : 360);
  }

  private unlockAudio() {
    unlockGameAudio();
    if (this.audioUnlocked) return;
    this.audioUnlocked = true;
    const firstSound = this.sounds.click;
    if (!firstSound) return;
    firstSound.volume = sampleVolume("click");
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
    sound.volume = sampleVolume(name);
    sound.currentTime = 0;
    sound.play().catch(() => undefined);
  }
}

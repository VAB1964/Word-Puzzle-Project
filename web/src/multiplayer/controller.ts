import type {
  AiLevel,
  HintKind,
  PresentationEvent,
  PublicPuzzle,
  RoomSnapshot
} from "../../../shared/multiplayer/types";
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

export class MultiplayerController {
  private snapshot: RoomSnapshot | null = null;
  private feedback = "Connected to the library table.";
  private connection = "connecting";
  private selectedIndices: number[] = [];
  private wheelLetters: string[] = [];
  private awaitingLetterHint = false;

  constructor(
    private readonly root: HTMLElement,
    readonly client: MultiplayerClient,
    private readonly onLeave: () => void
  ) {
    root.addEventListener("click", (event) => this.handleClick(event));
    root.addEventListener("change", (event) => this.handleChange(event));
    window.addEventListener("keydown", this.handleKeyDown);
  }

  destroy() {
    window.removeEventListener("keydown", this.handleKeyDown);
    this.client.close();
  }

  setConnection(status: string) {
    this.connection = status;
    this.render();
  }

  setError(message: string) {
    this.feedback = message;
    this.render();
  }

  setSnapshot(snapshot: RoomSnapshot, events: PresentationEvent[]) {
    const puzzleChanged = snapshot.puzzle?.id !== this.snapshot?.puzzle?.id;
    this.snapshot = snapshot;
    if (puzzleChanged) {
      this.wheelLetters = snapshot.puzzle?.baseLetters.split("") ?? [];
      this.clearGuess();
    }
    const latest = events[events.length - 1];
    if (latest) this.feedback = latest.text;
    this.render();
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
          <span>${escapeHtml(snapshot.settings.mode)} · ${escapeHtml(snapshot.settings.difficulty)}</span>
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
                      <small>${participant.kind === "ai" ? escapeHtml(participant.aiLevel ?? "AI") : participant.connected ? "Human · connected" : "Human · disconnected"}</small></div>
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
            <label>Mode
              <select data-setting="mode" ${host ? "" : "disabled"}>
                <option ${snapshot.settings.mode === "Casual" ? "selected" : ""}>Casual</option>
                <option ${snapshot.settings.mode === "Crossword" ? "selected" : ""}>Crossword</option>
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
    return `
      ${this.renderScores(snapshot)}
      <div class="mp-play-status">
        <span>Puzzle ${snapshot.puzzleIndex + 1} of ${snapshot.puzzleCount}</span>
        <span>Hint credits: <strong>${hintCredits}</strong></span>
      </div>
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
                      `<button style="--wheel-index:${index};--wheel-count:${this.wheelLetters.length}" data-action="letter" data-index="${index}" class="${this.selectedIndices.includes(index) ? "selected" : ""}">${escapeHtml(letter)}</button>`
                  )
                  .join("")}</div>
                <div class="mp-control-row">
                  <button data-action="clear">Clear</button>
                  <button data-action="shuffle">Shuffle</button>
                  <button class="mp-submit" data-action="submit">Submit</button>
                </div>
                <div class="mp-hints">
                  ${HINTS.map(
                    (hint) =>
                      `<button data-action="hint" data-hint="${hint.kind}" ${hintCredits < hint.cost ? "disabled" : ""}>${hint.label}<small>${hint.cost}</small></button>`
                  ).join("")}
                </div>
                <button data-action="skip">Request Skip</button>
              </aside>
            </main>`
      }`;
  }

  private renderScores(snapshot: RoomSnapshot) {
    return `<div class="mp-score-strip">${[...snapshot.participants]
      .sort((left, right) => left.seat - right.seat)
      .map(
        (participant) => `<article style="--player-color:${participant.color}">
          <div class="mp-player-identity">
            <span class="mp-color-dot"></span>
            <strong>${escapeHtml(participant.name)}</strong>
            <span class="mp-player-level">${
              participant.kind === "ai" ? escapeHtml(participant.aiLevel ?? "AI") : "Human"
            }</span>
          </div>
          <b>${participant.score.total}</b>
          <small>L ${participant.score.letters} · E ${participant.score.emerald} · D ${participant.score.diamond} · R ${participant.score.ruby}</small>
        </article>`
      )
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
    return `<div class="mp-board" style="--rows:${puzzle.rows};--cols:${puzzle.cols}">
      ${[...cells.entries()]
        .map(([key, cell]) => {
          const visible = puzzle.visibleCells[key];
          const owner = snapshot.participants.find((participant) => participant.id === visible?.ownerId);
          const ref = cell.refs.find(
            (candidate) => !puzzle.words.find((word) => word.id === candidate.wordId)?.completed
          ) ?? cell.refs[0];
          const gems = [
            ...new Set(
              cell.refs
                .map((candidate) => puzzle.words.find((word) => word.id === candidate.wordId)?.rarity ?? 1)
                .map((rarity) => (rarity === 2 ? "emerald" : rarity === 3 ? "ruby" : rarity === 4 ? "diamond" : ""))
                .filter(Boolean)
            )
          ];
          return `<button class="mp-cell ${visible ? "filled" : ""}" style="grid-row:${cell.row + 1};grid-column:${cell.col + 1};--owner-color:${owner?.color ?? "#5b4631"}"
            data-action="board-cell" data-word="${ref.wordId}" data-position="${ref.position}"
            aria-label="${visible ? `${visible.letter}, owned by ${owner?.name ?? "player"}` : "Unrevealed letter"}">
            ${visible ? escapeHtml(visible.letter) : ""}
            ${visible ? `<span class="mp-owner-mark"></span>` : ""}
            ${gems.length > 0 ? `<span class="mp-cell-gems">${gems.map((gem) => `<i class="${gem}" title="${gem} word"></i>`).join("")}</span>` : ""}
          </button>`;
        })
        .join("")}
    </div>`;
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
      </main>`;
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
              <span>${participant.kind === "ai" ? escapeHtml(participant.aiLevel ?? "AI") : "Human"}</span>
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
    const button = (event.target as HTMLElement).closest<HTMLElement>("[data-action]");
    const snapshot = this.snapshot;
    if (!button || !snapshot) return;
    const action = button.dataset.action;
    const local = snapshot.participants.find((participant) => participant.id === snapshot.localParticipantId);
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
      this.client.send({ type: "start-session" });
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
      if (guess.length >= 3 && this.client.send({ type: "submit-guess", guess })) this.clearGuess();
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
    if (select.dataset.setting === "difficulty") {
      settings.difficulty = select.value as typeof settings.difficulty;
    }
    if (select.dataset.setting === "capacity") {
      settings.capacity = Number(select.value) as typeof settings.capacity;
    }
    this.client.send({ type: "update-settings", settings, expectedRevision: snapshot.revision });
  }

  private handleKeyDown = (event: KeyboardEvent) => {
    const snapshot = this.snapshot;
    if (!snapshot || snapshot.status !== "playing" || snapshot.paused) return;
    if (event.key === "Enter") {
      const guess = this.currentGuess();
      if (guess.length >= 3 && this.client.send({ type: "submit-guess", guess })) this.clearGuess();
      return;
    }
    if (event.key === "Backspace") {
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
}

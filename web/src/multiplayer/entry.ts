import type { RoomSettings } from "../../../shared/multiplayer/types";
import {
  MultiplayerClient,
  createRoom,
  joinRoom,
  loadIdentity,
  loadRoomCredential,
  saveIdentity,
  saveRoomCredential,
  type SavedRoomCredential
} from "./client";
import { MultiplayerController } from "./controller";

const normalizeCode = (value: string) => {
  const trimmed = value.trim();
  try {
    const url = new URL(trimmed);
    const pathCode = url.pathname.match(/\/room\/([A-Za-z2-9]{6})/)?.[1];
    const queryCode = url.searchParams.get("room");
    return (pathCode ?? queryCode ?? "").toUpperCase();
  } catch {
    return trimmed.toUpperCase().replace(/[^A-Z2-9]/g, "");
  }
};

const escapeAttribute = (value: string) =>
  value
    .replace(/&/g, "&amp;")
    .replace(/"/g, "&quot;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;");

export const initializeAppEntry = (canvas: HTMLCanvasElement, startSinglePlayer: () => void) => {
  const root = document.getElementById("appEntry");
  if (!root) throw new Error("Missing #appEntry element.");
  let controller: MultiplayerController | null = null;

  const showSolo = () => {
    controller?.destroy();
    controller = null;
    root.hidden = true;
    canvas.hidden = false;
    startSinglePlayer();
  };

  const showHome = () => {
    controller?.destroy();
    controller = null;
    canvas.hidden = true;
    root.hidden = false;
    root.innerHTML = `
      <section class="entry-shell">
        <div class="entry-paper">
          <p class="entry-kicker">Cozy Library</p>
          <h1>Word Puzzle</h1>
          <p>Play a quiet solo session or compete together on a shared puzzle.</p>
          <div class="entry-choice-grid">
            <button class="entry-choice" data-entry-action="single">
              <strong>Single Player</strong><span>Classic Casual and Crossword sessions</span>
            </button>
            <button class="entry-choice entry-choice-primary" data-entry-action="multiplayer">
              <strong>Multiplayer</strong><span>One to four humans and AI players</span>
            </button>
          </div>
        </div>
      </section>`;
  };

  const showMultiplayer = (message = "") => {
    const identity = loadIdentity();
    canvas.hidden = true;
    root.hidden = false;
    root.innerHTML = `
      <section class="entry-shell">
        <div class="entry-paper entry-multiplayer">
          <button class="entry-back" data-entry-action="home">← Back</button>
          <p class="entry-kicker">Shared Table</p>
          <h1>Multiplayer</h1>
          <label class="entry-field">Your name
            <input id="entry-name" maxlength="24" autocomplete="nickname" value="${escapeAttribute(identity.name)}" placeholder="Player name">
          </label>
          <div class="entry-simple-actions">
            <form id="create-room-form">
              <button class="mp-primary" type="submit">Create Room</button>
            </form>
            <form id="join-room-form">
              <label>Room code
                <input id="entry-code" maxlength="160" autocomplete="off" placeholder="ABC234">
              </label>
              <button class="mp-primary" type="submit">Join Room</button>
            </form>
          </div>
          <p class="entry-message" role="alert">${message}</p>
        </div>
      </section>`;
  };

  const connect = (credential: SavedRoomCredential) => {
    saveRoomCredential(credential);
    let nextController: MultiplayerController | null = null;
    const client = new MultiplayerClient(credential, {
      onSnapshot: (snapshot, events) => nextController?.setSnapshot(snapshot, events),
      onConnection: (status) => nextController?.setConnection(status),
      onError: (message) => nextController?.setError(message)
    });
    nextController = new MultiplayerController(root, client, showHome);
    controller = nextController;
    client.connect();
  };

  root.addEventListener("click", (event) => {
    const action = (event.target as HTMLElement).closest<HTMLElement>("[data-entry-action]")?.dataset
      .entryAction;
    if (action === "single") showSolo();
    if (action === "multiplayer") showMultiplayer();
    if (action === "home") showHome();
  });

  root.addEventListener("submit", (event) => {
    const form = event.target as HTMLFormElement;
    if (form.id !== "create-room-form" && form.id !== "join-room-form") return;
    event.preventDefault();
    const name = (document.getElementById("entry-name") as HTMLInputElement)?.value.trim() ?? "";
    if (!name) {
      showMultiplayer("Enter your name first.");
      return;
    }
    saveIdentity({ name });
    const submit = form.querySelector<HTMLButtonElement>("button[type=submit]");
    if (submit) submit.disabled = true;

    if (form.id === "create-room-form") {
      const settings: RoomSettings = {
        mode: "Casual",
        difficulty: "Easy",
        capacity: 4
      };
      void createRoom({ name, settings })
        .then((result) =>
          connect({
            code: result.code,
            participantId: result.participantId,
            reconnectToken: result.reconnectToken
          })
        )
        .catch((error: unknown) => showMultiplayer(error instanceof Error ? error.message : "Could not create room."));
    } else {
      const code = normalizeCode((document.getElementById("entry-code") as HTMLInputElement)?.value ?? "");
      if (code.length !== 6) {
        showMultiplayer("Enter a valid six-character invite code.");
        return;
      }
      void joinRoom(code, name)
        .then((result) =>
          connect({
            code: result.code,
            participantId: result.participantId,
            reconnectToken: result.reconnectToken
          })
        )
        .catch((error: unknown) => showMultiplayer(error instanceof Error ? error.message : "Could not join room."));
    }
  });

  const requestedCode = normalizeCode(new URLSearchParams(window.location.search).get("room") ?? "");
  if (requestedCode.length === 6) {
    const saved = loadRoomCredential(requestedCode);
    if (saved) connect(saved);
    else {
      showMultiplayer();
      const codeInput = document.getElementById("entry-code") as HTMLInputElement | null;
      if (codeInput) codeInput.value = requestedCode;
    }
  } else {
    showHome();
  }
};

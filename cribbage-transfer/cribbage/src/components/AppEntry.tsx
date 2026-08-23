import { useEffect, useMemo, useRef, useState } from "react";
import CribbageGame from "../CribbageGame";
import { configureGameAudio, unlockGameAudio } from "../audio/gameAudio";
import { MultiplayerController, type MultiplayerSnapshot, type RoomMessage } from "../controllers/MultiplayerController";
import { AVATARS, avatarById, loadPreferences, savePreferences, type PlayerPreferences } from "../identity/preferences";
import type { ConnectionState } from "../multiplayer/protocol";
import MultiplayerTable from "./MultiplayerTable";

type Screen = "mode" | "identity" | "room" | "lobby" | "single";

function roomCodeFromLocation() {
  const match = location.pathname.match(/\/room\/([A-Z0-9]{6})(?:\/|$)/i);
  if (match) return match[1].toUpperCase();
  const query = new URLSearchParams(location.search).get("room");
  return /^[A-Z0-9]{6}$/i.test(query ?? "") ? query!.toUpperCase() : "";
}

function Identity({ value, onConfirm, onBack }: { value: PlayerPreferences; onConfirm: (value: PlayerPreferences) => void; onBack: () => void }) {
  const [draft, setDraft] = useState(value);
  return <section className="entry-card identity-card">
    <span className="eyebrow">Your chair at the table</span><h2>Player identity</h2>
    <label>Display name<input maxLength={24} autoComplete="nickname" value={draft.displayName} onChange={event => setDraft({ ...draft, displayName: event.target.value })} /></label>
    <fieldset><legend>Choose an avatar</legend><div className="avatar-grid">
      {AVATARS.map(avatar => <button key={avatar.id} className={draft.avatarId === avatar.id ? "selected" : ""} aria-pressed={draft.avatarId === avatar.id} aria-label={avatar.label} onClick={() => setDraft({ ...draft, avatarId: avatar.id })}><img src={avatar.src} alt="" /><small>{avatar.label}</small></button>)}
    </div></fieldset>
    <div className="preference-grid">
      <label><input type="checkbox" checked={draft.soundEnabled} onChange={event => setDraft({ ...draft, soundEnabled: event.target.checked })} /> Sound</label>
      <label><input type="checkbox" checked={draft.voiceEnabled} onChange={event => setDraft({ ...draft, voiceEnabled: event.target.checked })} /> Table-talk voice</label>
      <label><input type="checkbox" checked={draft.reducedAnimation} onChange={event => setDraft({ ...draft, reducedAnimation: event.target.checked })} /> Reduced animation</label>
      <label>Volume<input type="range" min="0" max="1" step=".05" value={draft.volume} onChange={event => setDraft({ ...draft, volume: Number(event.target.value) })} /></label>
      <label>Table talk<select value={draft.tableTalk} onChange={event => setDraft({ ...draft, tableTalk: event.target.value as PlayerPreferences["tableTalk"] })}><option value="off">Off</option><option value="occasional">Occasional</option><option value="chatty">Chatty</option></select></label>
    </div>
    <div className="entry-actions"><button className="quiet" onClick={onBack}>Back</button><button className="primary" disabled={!draft.displayName.trim()} onClick={() => onConfirm({ ...draft, displayName: draft.displayName.trim() })}>Continue</button></div>
  </section>;
}

type RoomPlayer = { id: string; name: string; avatarId: string; status?: string; seat: number | null; teamId: "gold" | "green" | null; connected: boolean; ready: boolean; isAI: boolean };
type RoomView = MultiplayerSnapshot & { hostPlayerId: string; seatCount: number; players: RoomPlayer[] };
const credentialKey = (code: string) => `cribbage.room.${code}.credential.v1`;

function Lobby({ code, preferences, create, onLeave }: { code: string; preferences: PlayerPreferences; create: boolean; onLeave: () => void }) {
  const [ready, setReady] = useState(false);
  const [view, setView] = useState<RoomView | null>(null);
  const [connectionState, setConnectionState] = useState<ConnectionState>("connecting");
  const [lastMessage, setLastMessage] = useState<RoomMessage>();
  const [localPlayerId, setLocalPlayerId] = useState<string | null>(null);
  const [error, setError] = useState("");
  const controllerRef = useRef<MultiplayerController | null>(null);
  const credentialRef = useRef<{ playerId: string; reconnectToken: string } | null>(null);
  const firstConnection = useRef(create);
  const [stakes, setStakes] = useState(false);
  const [gameAmount, setGameAmount] = useState(100);
  const [perHole, setPerHole] = useState(5);
  const invite = `${location.origin}/cribbage/room/${code}`;
  useEffect(() => {
    try { credentialRef.current = JSON.parse(localStorage.getItem(credentialKey(code)) ?? "null") as typeof credentialRef.current; } catch { credentialRef.current = null; }
    setLocalPlayerId(credentialRef.current?.playerId ?? null);
    const protocol = location.protocol === "https:" ? "wss:" : "ws:";
    const controller = new MultiplayerController(`${protocol}//${location.host}/api/cribbage/rooms/${code}/ws`, code, credentialRef.current?.playerId ?? null,
      (state, snapshot, message, messageError) => {
        setConnectionState(state);
        if (message) setLastMessage(message);
        if (messageError) setError(messageError);
        if (message?.type === "CONNECTED") {
          const credential = credentialRef.current;
          if (firstConnection.current && !credential) {
            controller.send("CREATE_ROOM", { name: preferences.displayName, avatarId: preferences.avatarId, seatCount: 4 });
            firstConnection.current = false;
          } else {
            controller.send("JOIN_ROOM", { name: preferences.displayName, avatarId: preferences.avatarId, reconnectToken: credential?.reconnectToken });
          }
        }
        if (message?.ok === true && message.event && ["CREATE_ROOM_ACCEPTED", "JOIN_ROOM_ACCEPTED"].includes(message.event.type)) {
          const playerId = String(message.event.data.playerId);
          controller.setPlayerId(playerId);
          setLocalPlayerId(playerId);
          const token = message.event.data.reconnectToken;
          if (typeof token === "string") {
            credentialRef.current = { playerId, reconnectToken: token };
            localStorage.setItem(credentialKey(code), JSON.stringify(credentialRef.current));
          }
        }
        const rotatedToken = message?.event?.data.reconnectToken ?? (snapshot as Record<string, unknown> | null)?.reconnectToken;
        if (typeof rotatedToken === "string" && credentialRef.current) {
          credentialRef.current = { ...credentialRef.current, reconnectToken: rotatedToken };
          localStorage.setItem(credentialKey(code), JSON.stringify(credentialRef.current));
        }
        if (message?.ok === false && message.error) setError(message.error.message);
        if (snapshot && "players" in snapshot) {
          const room = snapshot as RoomView;
          setView(room);
          const own = room.players.find(player => player.id === credentialRef.current?.playerId);
          if (own) setReady(own.ready);
        }
      });
    controllerRef.current = controller;
    controller.connect();
    return () => controller.disconnect();
  }, [code, preferences.avatarId, preferences.displayName]);
  const send = (type: Parameters<MultiplayerController["send"]>[0], payload: unknown) => {
    try { controllerRef.current?.send(type, payload); setError(""); } catch (sendError) { setError(sendError instanceof Error ? sendError.message : "Command failed."); }
  };
  const seatCount = view?.seatCount ?? 4;
  const seats = Array.from({ length: seatCount }, (_, seat) => view?.players.find(player => player.seat === seat) ?? null);
  const playerId = localPlayerId ?? credentialRef.current?.playerId;
  const isHost = Boolean(view && playerId === view.hostPlayerId);
  const phase = String(view?.phase ?? (view?.game && typeof view.game === "object" ? (view.game as { phase?: unknown }).phase : "lobby")).toLowerCase();
  if (view && playerId && !["", "lobby", "waiting", "setup"].includes(phase)) return <MultiplayerTable view={view} playerId={playerId} preferences={preferences} connection={connectionState} message={lastMessage} send={send} onLeave={() => { send("LEAVE_ROOM", {}); onLeave(); }} />;
  return <section className="lobby entry-card">
    <header><div><span className="eyebrow">Private room · {connectionState === "connected" ? "Connected" : connectionState === "reconnecting" ? "Reconnecting…" : "Connecting…"}</span><h2>Waiting room</h2></div><div className="room-code"><small>Room code</small><strong>{code}</strong></div></header>
    {error && <p role="alert" className="error-message">{error}</p>}
    <div className="invite-card"><input readOnly aria-label="Invite link" value={invite} /><button onClick={() => void navigator.clipboard?.writeText(invite)}>Copy link</button>{navigator.share && <button onClick={() => void navigator.share({ title: "Cribbage room", url: invite })}>Share</button>}</div>
    <div className="seat-grid">{seats.map((player, index) => {
      const avatar = avatarById(player?.avatarId);
      const displayTeam = seatCount === 4
        ? (player?.teamId === "green" || index % 2 ? "green" : "red")
        : index === 2 ? "blue" : index === 1 ? "green" : "red";
      return <article className={`lobby-seat team-${displayTeam}`} key={index}><span className="seat-avatar">{avatar ? <img src={avatar.src} alt="" /> : "○"}</span><div><strong>{player?.name ?? "Open seat"}{player?.id === view?.hostPlayerId ? " ♛" : ""}</strong><small>{player ? (player.isAI ? "AI" : player.connected ? "Occupied" : "Reconnecting") : "Open"} · Team {displayTeam[0].toUpperCase() + displayTeam.slice(1)} · {player?.ready ? "Ready" : "Not ready"}</small></div></article>;
    })}</div>
    <div className="host-setup"><h3>Host setup</h3><label>Seats<select disabled={!isHost} value={seatCount} onChange={event => send("UPDATE_SETUP", { seatCount: Number(event.target.value) })}><option>2</option><option>3</option><option>4</option></select></label><label>Add AI<select disabled={!isHost} defaultValue="" onChange={event => { const openSeat = seats.findIndex(player => !player); if (openSeat >= 0 && event.target.value) send("ADD_AI", { seat: openSeat, difficulty: event.target.value }); event.target.value = ""; }}><option value="">Choose difficulty</option><option value="medium">Medium</option><option value="easy">Easy</option><option value="hard">Hard</option></select></label><label>Table talk<select defaultValue={preferences.tableTalk}><option>off</option><option>occasional</option><option>chatty</option></select></label><label><input type="checkbox" disabled={!isHost} checked={stakes} onChange={event => { const enabled = event.target.checked; setStakes(enabled); send("UPDATE_LEDGER", { enabled, baseStakeCents: gameAmount, perHoleCents: perHole }); }} /> Session Ledger</label>{stakes && <><label>Game amount (cents)<input type="number" min="0" max="2000" value={gameAmount} onChange={event => { const value = Number(event.target.value); setGameAmount(value); send("UPDATE_LEDGER", { enabled: true, baseStakeCents: value, perHoleCents: perHole }); }} /></label><label>Per hole<select value={perHole} onChange={event => { const value = Number(event.target.value); setPerHole(value); send("UPDATE_LEDGER", { enabled: true, baseStakeCents: gameAmount, perHoleCents: value }); }}><option>5</option><option>10</option><option>15</option><option>20</option></select> cents</label></>}</div>
    <p className="ledger-note">Session ledger: 0 games · no balance entries yet. Friendly recordkeeping only; no payments are processed.</p>
    <div className="entry-actions"><button className="quiet" onClick={() => { send("LEAVE_ROOM", {}); onLeave(); }}>Leave room</button><button onClick={() => { configureGameAudio(preferences.soundEnabled, preferences.volume); unlockGameAudio(); send("SET_READY", { ready: !ready }); }}>{ready ? "Not ready" : "Ready"}</button>{isHost && <button className="primary" disabled={!ready || seats.some(player => !player)} onClick={() => { configureGameAudio(preferences.soundEnabled, preferences.volume); unlockGameAudio(); send("START_GAME", {}); }}>Start game</button>}</div>
  </section>;
}

export default function AppEntry() {
  const inviteCode = useMemo(roomCodeFromLocation, []);
  const [preferences, setPreferences] = useState(loadPreferences);
  const [screen, setScreen] = useState<Screen>(inviteCode ? "identity" : "mode");
  const [pendingMode, setPendingMode] = useState<"single" | "multiplayer">(inviteCode ? "multiplayer" : "single");
  const [roomInput, setRoomInput] = useState(inviteCode);
  const [roomCode, setRoomCode] = useState(inviteCode);
  const [createRoomIntent, setCreateRoomIntent] = useState(false);
  const [roomError, setRoomError] = useState("");
  const [creatingRoom, setCreatingRoom] = useState(false);
  const confirmIdentity = (next: PlayerPreferences) => {
    savePreferences(next); setPreferences(next);
    setScreen(pendingMode === "single" ? "single" : inviteCode ? "lobby" : "room");
  };
  if (screen === "single") return <div className={preferences.reducedAnimation ? "reduce-animation" : ""}><CribbageGame initialPreferences={preferences} onExit={() => setScreen("mode")} /></div>;
  if (screen === "lobby") return <main className={`multiplayer-shell ${preferences.reducedAnimation ? "reduce-animation" : ""}`}><Lobby code={roomCode} preferences={preferences} create={createRoomIntent} onLeave={() => setScreen("mode")} /></main>;
  return <main className={`entry-shell ${preferences.reducedAnimation ? "reduce-animation" : ""}`}>
    {screen === "mode" && <section className="entry-card mode-card"><span className="eyebrow">A classic card-room game</span><h1>Pull up a chair.</h1><p>Choose local play against the computer or join friends at a private online table.</p><div className="mode-picks"><button onClick={() => { setPendingMode("single"); setScreen("identity"); }}><strong>Single Player</strong><span>Play locally with familiar AI opponents.</span></button><button onClick={() => { setPendingMode("multiplayer"); setScreen("identity"); }}><strong>Multiplayer</strong><span>Create or join a private room.</span></button></div><a href="https://vabgames.com">Back to VABGames.com</a></section>}
    {screen === "identity" && <Identity value={preferences} onConfirm={confirmIdentity} onBack={() => setScreen("mode")} />}
    {screen === "room" && <section className="entry-card room-choice"><span className="eyebrow">Private online room</span><h2>Create or join</h2>{roomError && <p role="alert" className="error-message">{roomError}</p>}<button className="primary" disabled={creatingRoom} onClick={() => { setCreatingRoom(true); setRoomError(""); void fetch("/api/cribbage/rooms", { method: "POST", headers: { "content-type": "application/json", accept: "application/json" }, body: "{}" }).then(async response => { const contentType = response.headers.get("content-type") ?? ""; const body = contentType.includes("application/json") ? await response.json() as { roomId?: string; error?: { message?: string } } : null; if (!response.ok || !body?.roomId) throw new Error(body?.error?.message ?? `Room service unavailable (${response.status}).`); setRoomCode(body.roomId); setCreateRoomIntent(true); setScreen("lobby"); }).catch(error => setRoomError(error instanceof Error ? error.message : "Unable to create a room.")).finally(() => setCreatingRoom(false)); }}>{creatingRoom ? "Creating…" : "Create game"}</button><div className="join-divider">or</div><label>Room code or invite link<input value={roomInput} onChange={event => setRoomInput(event.target.value)} placeholder="7KQ4MT or invite link" /></label><button disabled={!roomInput.trim()} onClick={() => { const match = roomInput.toUpperCase().match(/([A-HJ-NP-Z2-9]{6})(?:\/?$)/); if (match) { setRoomCode(match[1]); setCreateRoomIntent(false); setScreen("lobby"); } else setRoomError("Enter a valid six-character room code or invite link."); }}>Join game</button><button className="quiet" onClick={() => setScreen("mode")}>Back</button></section>}
  </main>;
}

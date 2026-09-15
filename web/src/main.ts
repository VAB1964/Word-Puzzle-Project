import "./style.css";
import { Game } from "./game";
import { initializeAppEntry } from "./multiplayer/entry";

const CANONICAL_HOST = "vabgames.com";
const NON_CANONICAL_HOSTS = new Set(["thevabgames.com", "www.thevabgames.com"]);
if (NON_CANONICAL_HOSTS.has(window.location.hostname)) {
  const redirected = new URL(window.location.href);
  redirected.hostname = CANONICAL_HOST;
  window.location.replace(redirected.toString());
}

const canvas = document.getElementById("gameCanvas") as HTMLCanvasElement | null;
if (!canvas) {
  throw new Error("Missing #gameCanvas element.");
}

const ctx = canvas.getContext("2d");
if (!ctx) {
  throw new Error("Failed to get 2D context.");
}

let game: Game | null = null;

const resizeCanvas = () => {
  const dpr = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();
  canvas.width = Math.floor(rect.width * dpr);
  canvas.height = Math.floor(rect.height * dpr);
  game?.onResize(rect.width, rect.height, dpr);
};

initializeAppEntry(canvas, () => {
  game ??= new Game(canvas, ctx);
  resizeCanvas();
});

window.addEventListener("resize", resizeCanvas);
resizeCanvas();

let lastTime = performance.now();
const frame = (now: number) => {
  const dt = Math.min(0.1, (now - lastTime) / 1000);
  lastTime = now;
  game?.update(dt);
  game?.render();
  requestAnimationFrame(frame);
};

requestAnimationFrame(frame);

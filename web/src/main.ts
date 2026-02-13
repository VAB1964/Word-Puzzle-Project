import "./style.css";
import { Game } from "./game";

const canvas = document.getElementById("gameCanvas") as HTMLCanvasElement | null;
if (!canvas) {
  throw new Error("Missing #gameCanvas element.");
}

const ctx = canvas.getContext("2d");
if (!ctx) {
  throw new Error("Failed to get 2D context.");
}

const game = new Game(canvas, ctx);

const resizeCanvas = () => {
  const dpr = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();
  canvas.width = Math.floor(rect.width * dpr);
  canvas.height = Math.floor(rect.height * dpr);
  game.onResize(rect.width, rect.height, dpr);
};

window.addEventListener("resize", resizeCanvas);
resizeCanvas();

let lastTime = performance.now();
const frame = (now: number) => {
  const dt = Math.min(0.1, (now - lastTime) / 1000);
  lastTime = now;
  game.update(dt);
  game.render();
  requestAnimationFrame(frame);
};

requestAnimationFrame(frame);

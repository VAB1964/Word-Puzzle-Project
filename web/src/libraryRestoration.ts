import rooms from "./data/library-restorations.json";
import type { Rect } from "./core/types";

const sources = import.meta.glob("../../assets/living-library/rooms/**/*.webp", {
  eager: true, query: "?url", import: "default"
}) as Record<string, string>;
export const LIBRARY_RESTORATIONS = rooms;
export const restorationForPuzzle = (puzzle: number) =>
  rooms[Math.floor((puzzle - 1) / 10)]?.restorations.find(reward => reward.puzzle === puzzle);
export const earnedRestorations = (room: number, completed: readonly number[]) =>
  rooms[room].restorations.filter(reward => completed.includes(reward.puzzle)).sort((a, b) => a.layer - b.layer);

/** Decode rooms on demand; retain at most two rooms to limit tablet texture memory. */
export class LibraryRestorationArt {
  private loaded = new Map<number, Map<string, HTMLImageElement>>();
  private pending = new Map<number, Promise<void>>();
  private failed = new Set<number>();

  load(room: number): Promise<void> {
    if (this.loaded.has(room) || this.failed.has(room)) return Promise.resolve();
    const pending = this.pending.get(room);
    if (pending) return pending;
    const definition = rooms[room];
    const task = Promise.all([definition.base, ...definition.restorations.map(reward => reward.file)].map(async file => {
      const image = new Image();
      image.src = sources[`../../assets/living-library/rooms/${file}`];
      await image.decode();
      return [file, image] as const;
    })).then(entries => {
      this.loaded.set(room, new Map(entries));
      while (this.loaded.size > 2) this.loaded.delete(this.loaded.keys().next().value!);
    }).catch(() => {
      this.failed.add(room);
    }).finally(() => { this.pending.delete(room); });
    this.pending.set(room, task);
    return task;
  }

  draw(ctx: CanvasRenderingContext2D, room: number, rect: Rect, completed: readonly number[], revealPuzzle: number | null = null, reveal = 1): boolean {
    void this.load(room);
    const images = this.loaded.get(room);
    const definition = rooms[room];
    if (!images) {
      ctx.save();
      ctx.fillStyle = "#102d36";
      ctx.fillRect(rect.x, rect.y, rect.width, rect.height);
      ctx.fillStyle = "#f5e2b8";
      ctx.font = "16px Georgia, serif";
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(this.failed.has(room) ? "Room artwork unavailable" : "Opening the room…", rect.x + rect.width / 2, rect.y + rect.height / 2);
      ctx.restore();
      return false;
    }
    ctx.save();
    ctx.beginPath();
    ctx.rect(rect.x, rect.y, rect.width, rect.height);
    ctx.clip();
    ctx.drawImage(images.get(definition.base)!, rect.x, rect.y, rect.width, rect.height);
    for (const reward of earnedRestorations(room, completed)) {
      const image = images.get(reward.file)!;
      const box = reward.box;
      const scale = Math.min(rect.width * box.w / 100 / image.width, rect.height * box.h / 100 / image.height);
      const w = image.width * scale, h = image.height * scale;
      const x = rect.x + rect.width * box.x / 100 - w / 2;
      const y = rect.y + rect.height * box.y / 100 - h;
      ctx.save();
      const progress = reward.puzzle === revealPuzzle ? Math.max(0, Math.min(1, reveal)) : 1;
      ctx.globalAlpha = progress;
      if (progress < 1) {
        ctx.shadowColor = "#ffe3a0";
        ctx.shadowBlur = 25 * Math.sin(progress * Math.PI);
      }
      ctx.drawImage(image, x, y + (1 - progress) * 12, w, h);
      ctx.restore();
    }
    ctx.restore();
    return true;
  }
}

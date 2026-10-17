import type { Rect } from "./core/types";

export const LIBRARY_ROOMS = [
  { name: "The Front Hall", rule: "3-letter words" },
  { name: "The Garden Atrium", rule: "4 letters · find 3–4 letter words" },
  { name: "The Reading Room", rule: "5 letters · find 3–5 letter words" },
  { name: "The Wordsmith’s Study", rule: "6 letters · find 3–6 letter words" },
  { name: "The Map Room", rule: "7 letters · find 3–7 letter words" },
  { name: "The Observatory", rule: "7 letters · find 4–7 letter words" }
] as const;

// All rectangles use the game's existing design coordinate system.
export function journeyLayout(width: number, height: number) {
  const portrait = height > width;
  const roomWidth = Math.min(380, width - 70);
  const roomHeight = portrait ? 86 : 68;
  const top = portrait ? 165 : 145;
  const bottom = height - 180;
  const offsets = [0.58, 0.38, 0.57, 0.38, 0.56, 0.48];
  const rooms: Rect[] = offsets.map((offset, index) => ({
    x: Math.max(24, Math.min(width - roomWidth - 24, width * offset - roomWidth / 2)),
    y: bottom - (bottom - top) * index / 5 - roomHeight / 2,
    width: roomWidth,
    height: roomHeight
  }));
  const panelWidth = Math.min(550, width - 40);
  const panelHeight = Math.min(720, height - 120);
  const panel: Rect = { x: (width - panelWidth) / 2, y: (height - panelHeight) / 2, width: panelWidth, height: panelHeight };
  const gap = 12;
  const cellWidth = (panelWidth - 60 - gap) / 2;
  const cellHeight = (panelHeight - 285 - 4 * gap) / 5;
  const puzzles: Rect[] = Array.from({ length: 10 }, (_, i) => ({
    x: panel.x + 30 + (i % 2) * (cellWidth + gap),
    y: panel.y + 185 + Math.floor(i / 2) * (cellHeight + gap),
    width: cellWidth, height: cellHeight
  }));
  const artWidth = Math.min(panelWidth - 50, (panelHeight - 305) * 1.5);
  return {
    rooms, panel, puzzles,
    roomTab: { x: panel.x + 30, y: panel.y + 117, width: (panelWidth - 72) / 2, height: 48 },
    puzzlesTab: { x: panel.x + panelWidth / 2 + 6, y: panel.y + 117, width: (panelWidth - 72) / 2, height: 48 },
    roomArt: { x: panel.x + (panelWidth - artWidth) / 2, y: panel.y + 185, width: artWidth, height: artWidth * 2 / 3 },
    previous: { x: panel.x + 30, y: panel.y + panelHeight - 75, width: 130, height: 52 },
    next: { x: panel.x + panelWidth - 160, y: panel.y + panelHeight - 75, width: 130, height: 52 },
    close: { x: panel.x + panelWidth / 2 - 65, y: panel.y + panelHeight - 75, width: 130, height: 52 },
    back: { x: 25, y: height - 88, width: 135, height: 60 },
    newCampaign: { x: width - 235, y: height - 88, width: 210, height: 60 }
  };
}

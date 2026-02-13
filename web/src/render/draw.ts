import type { Color, Vec2 } from "../core/types";
import { colorToCss } from "../core/utils";

export const drawRoundedRect = (
  ctx: CanvasRenderingContext2D,
  x: number,
  y: number,
  width: number,
  height: number,
  radius: number,
  fill?: Color,
  stroke?: Color,
  strokeWidth = 1
) => {
  const r = Math.min(radius, width / 2, height / 2);
  ctx.beginPath();
  ctx.moveTo(x + r, y);
  ctx.arcTo(x + width, y, x + width, y + height, r);
  ctx.arcTo(x + width, y + height, x, y + height, r);
  ctx.arcTo(x, y + height, x, y, r);
  ctx.arcTo(x, y, x + width, y, r);
  ctx.closePath();

  if (fill) {
    ctx.fillStyle = colorToCss(fill);
    ctx.fill();
  }
  if (stroke) {
    ctx.strokeStyle = colorToCss(stroke);
    ctx.lineWidth = strokeWidth;
    ctx.stroke();
  }
};

export const drawCenteredText = (
  ctx: CanvasRenderingContext2D,
  text: string,
  center: Vec2,
  color: Color,
  font: string
) => {
  ctx.save();
  ctx.font = font;
  ctx.fillStyle = colorToCss(color);
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(text, center.x, center.y);
  ctx.restore();
};

const wrapSingleLine = (ctx: CanvasRenderingContext2D, line: string, maxWidth: number) => {
  if (maxWidth <= 0) return line;
  const words = line.split(/\s+/).filter(Boolean);
  let currentLine = "";
  const result: string[] = [];

  const flushLine = () => {
    if (currentLine) {
      result.push(currentLine);
      currentLine = "";
    }
  };

  for (const word of words) {
    const testLine = currentLine ? `${currentLine} ${word}` : word;
    if (ctx.measureText(testLine).width > maxWidth && currentLine) {
      flushLine();
      currentLine = word;
    } else if (ctx.measureText(testLine).width > maxWidth && !currentLine) {
      let chopped = "";
      for (const ch of word) {
        const testChopped = chopped + ch;
        if (ctx.measureText(testChopped).width > maxWidth && chopped) {
          result.push(chopped);
          chopped = "";
        }
        chopped += ch;
      }
      if (chopped) result.push(chopped);
    } else {
      currentLine = testLine;
    }
  }

  flushLine();
  return result.join("\n");
};

export const wrapTextForWidth = (
  ctx: CanvasRenderingContext2D,
  source: string,
  maxWidth: number
) => {
  if (maxWidth <= 0) return source;
  const lines = source.split(/\r?\n/);
  return lines.map((line) => wrapSingleLine(ctx, line, maxWidth)).join("\n");
};

export const measureMultilineText = (
  ctx: CanvasRenderingContext2D,
  text: string,
  lineHeight: number
) => {
  const lines = text.split("\n");
  const widths = lines.map((line) => ctx.measureText(line).width);
  const maxWidth = widths.length ? Math.max(...widths) : 0;
  const height = lines.length * lineHeight;
  return { width: maxWidth, height, lineCount: lines.length };
};

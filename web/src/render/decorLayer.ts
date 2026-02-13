import type { Vec2, Color } from "../core/types";
import type { ColorTheme } from "../core/themes";
import { colorToCss, randRange } from "../core/utils";
import { degToRad } from "../core/utils";

export enum DecorKind {
  Circle = "Circle",
  Triangle = "Triangle",
  Line = "Line"
}

interface DecorShape {
  kind: DecorKind;
  pos: Vec2;
  vel: Vec2;
  size: number;
  rot: number;
  color: Color;
}

export class DecorLayer {
  private shapes: DecorShape[] = [];

  constructor(private readonly targetCount = 25) {}

  update(dt: number, winSize: Vec2, theme: ColorTheme) {
    while (this.shapes.length < this.targetCount) {
      this.spawn(winSize, theme.decorBase, theme.decorAccent1, theme.decorAccent2);
    }

    for (const shape of this.shapes) {
      shape.pos = { x: shape.pos.x + shape.vel.x * dt, y: shape.pos.y + shape.vel.y * dt };
      if (shape.vel.x > 0 && shape.pos.x - shape.size > winSize.x) {
        shape.pos.x = -shape.size - randRange(50, 150);
        shape.pos.y = randRange(0, winSize.y);
      } else if (shape.vel.x < 0 && shape.pos.x + shape.size < 0) {
        shape.pos.x = winSize.x + shape.size + randRange(50, 150);
        shape.pos.y = randRange(0, winSize.y);
      }
      if (shape.pos.y - shape.size > winSize.y) shape.pos.y = -shape.size;
      else if (shape.pos.y + shape.size < 0) shape.pos.y = winSize.y + shape.size;
    }
  }

  draw(ctx: CanvasRenderingContext2D) {
    for (const shape of this.shapes) {
      ctx.save();
      ctx.fillStyle = colorToCss(shape.color);

      if (shape.kind === DecorKind.Circle) {
        ctx.beginPath();
        ctx.arc(shape.pos.x, shape.pos.y, shape.size, 0, Math.PI * 2);
        ctx.fill();
      } else if (shape.kind === DecorKind.Triangle) {
        const points: Vec2[] = [];
        for (let i = 0; i < 3; i += 1) {
          const ang = degToRad(shape.rot + i * 120);
          points.push({
            x: shape.pos.x + Math.cos(ang) * shape.size,
            y: shape.pos.y + Math.sin(ang) * shape.size
          });
        }
        ctx.beginPath();
        ctx.moveTo(points[0].x, points[0].y);
        ctx.lineTo(points[1].x, points[1].y);
        ctx.lineTo(points[2].x, points[2].y);
        ctx.closePath();
        ctx.fill();
      } else if (shape.kind === DecorKind.Line) {
        ctx.translate(shape.pos.x, shape.pos.y);
        ctx.rotate(degToRad(shape.rot));
        ctx.fillRect(-shape.size / 2, -1, shape.size, 2);
      }

      ctx.restore();
    }
  }

  private spawn(win: Vec2, base: Color, accent1: Color, accent2: Color) {
    const spawnLeft = randRange(0, 1, true) === 0;
    const kindIndex = randRange(0, 2, true);
    const kind = [DecorKind.Circle, DecorKind.Triangle, DecorKind.Line][kindIndex] ?? DecorKind.Circle;
    const yPos = randRange(0, win.y);
    const xPos = spawnLeft ? -randRange(50, 200) : win.x + randRange(50, 200);

    const shape: DecorShape = {
      kind,
      pos: { x: xPos, y: yPos },
      vel: { x: (spawnLeft ? 1 : -1) * randRange(15, 45), y: 0 },
      size: 0,
      rot: randRange(0, 360),
      color: base
    };

    if (kind === DecorKind.Circle) {
      shape.size = randRange(50, 120);
      shape.color = { ...base, a: randRange(60, 100, true) };
    } else if (kind === DecorKind.Triangle) {
      shape.size = randRange(40, 150);
      shape.color = { ...accent1, a: randRange(30, 60, true) };
    } else {
      shape.size = randRange(80, 180);
      shape.color = { ...accent2, a: randRange(80, 140, true) };
    }

    this.shapes.push(shape);
  }
}

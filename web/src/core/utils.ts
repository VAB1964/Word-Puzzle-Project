import type { Color, Rect, Vec2 } from "./types";

export const PI = Math.PI;

export const degToRad = (deg: number) => (deg * PI) / 180;

export const distSq = (a: Vec2, b: Vec2) => {
  const dx = a.x - b.x;
  const dy = a.y - b.y;
  return dx * dx + dy * dy;
};

export const clamp = (value: number, min: number, max: number) =>
  Math.max(min, Math.min(max, value));

export const lerp = (a: number, b: number, t: number) => a + (b - a) * t;

export const randRange = (min: number, max: number, integer = false) => {
  const value = Math.random() * (max - min) + min;
  return integer ? Math.floor(value) : value;
};

export const shuffle = <T>(arr: T[]) => {
  for (let i = arr.length - 1; i > 0; i -= 1) {
    const j = Math.floor(Math.random() * (i + 1));
    [arr[i], arr[j]] = [arr[j], arr[i]];
  }
  return arr;
};

export const rectContains = (rect: Rect, p: Vec2) =>
  p.x >= rect.x &&
  p.x <= rect.x + rect.width &&
  p.y >= rect.y &&
  p.y <= rect.y + rect.height;

export const colorToCss = (color: Color) => {
  const a = color.a ?? 255;
  const alpha = Math.max(0, Math.min(1, a / 255));
  return `rgba(${color.r}, ${color.g}, ${color.b}, ${alpha})`;
};

export const withAlpha = (color: Color, alpha: number): Color => ({
  r: color.r,
  g: color.g,
  b: color.b,
  a: alpha
});

export const addVec2 = (a: Vec2, b: Vec2): Vec2 => ({ x: a.x + b.x, y: a.y + b.y });
export const subVec2 = (a: Vec2, b: Vec2): Vec2 => ({ x: a.x - b.x, y: a.y - b.y });
export const mulVec2 = (a: Vec2, scalar: number): Vec2 => ({
  x: a.x * scalar,
  y: a.y * scalar
});

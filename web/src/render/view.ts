import type { Rect, Vec2 } from "../core/types";

export interface ViewTransform {
  scale: number;
  offset: Vec2;
  designSize: Vec2;
  screenSize: Vec2;
  viewport: Rect;
}

export const createViewTransform = (
  screenWidth: number,
  screenHeight: number,
  designWidth: number,
  designHeight: number
): ViewTransform => {
  const scale = Math.min(screenWidth / designWidth, screenHeight / designHeight);
  const viewWidth = designWidth * scale;
  const viewHeight = designHeight * scale;
  const offset = {
    x: (screenWidth - viewWidth) / 2,
    y: (screenHeight - viewHeight) / 2
  };
  return {
    scale,
    offset,
    designSize: { x: designWidth, y: designHeight },
    screenSize: { x: screenWidth, y: screenHeight },
    viewport: { x: offset.x, y: offset.y, width: viewWidth, height: viewHeight }
  };
};

export const applyView = (ctx: CanvasRenderingContext2D, view: ViewTransform, dpr = 1) => {
  ctx.setTransform(
    view.scale * dpr,
    0,
    0,
    view.scale * dpr,
    view.offset.x * dpr,
    view.offset.y * dpr
  );
};

export const screenToWorld = (view: ViewTransform, p: Vec2): Vec2 => ({
  x: (p.x - view.offset.x) / view.scale,
  y: (p.y - view.offset.y) / view.scale
});

export const worldToScreen = (view: ViewTransform, p: Vec2): Vec2 => ({
  x: p.x * view.scale + view.offset.x,
  y: p.y * view.scale + view.offset.y
});

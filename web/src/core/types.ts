export type Vec2 = { x: number; y: number };
export type Rect = { x: number; y: number; width: number; height: number };

export type Color = { r: number; g: number; b: number; a?: number };

export enum AnimTarget {
  Grid = "Grid",
  Score = "Score"
}

export enum GState {
  Playing = "Playing",
  Solved = "Solved"
}

export enum GameScreen {
  MainMenu = "MainMenu",
  CasualMenu = "CasualMenu",
  CompetitiveMenu = "CompetitiveMenu",
  Playing = "Playing",
  GameOver = "GameOver",
  SessionComplete = "SessionComplete"
}

export enum DifficultyLevel {
  None = "None",
  Easy = "Easy",
  Medium = "Medium",
  Hard = "Hard"
}

export enum HintType {
  RevealFirst = "RevealFirst",
  RevealRandom = "RevealRandom",
  RevealLast = "RevealLast",
  RevealFirstOfEach = "RevealFirstOfEach"
}

export interface WordInfo {
  text: string;
  rarity: number;
  pos: string;
  definition: string;
  sentence: string;
  avgSubLen: number;
  countGE3: number;
  countGE4: number;
  countGE5: number;
  easyValidCount: number;
  mediumValidCount: number;
  hardValidCount: number;
}

export interface LetterAnim {
  ch: string;
  start: Vec2;
  end: Vec2;
  t: number;
  wordIdx: number;
  charIdx: number;
  target: AnimTarget;
}

export interface ScoreParticleAnim {
  startPos: Vec2;
  endPos: Vec2;
  t: number;
  speed: number;
  points: number;
}

export interface ScoreFlourishParticle {
  textString: string;
  position: Vec2;
  velocity: Vec2;
  lifetime: number;
  initialLifetime: number;
  color: Color;
}

export interface HintPointAnimParticle {
  textString: string;
  currentPosition: Vec2;
  startPosition: Vec2;
  targetPosition: Vec2;
  color: Color;
  t: number;
  speed: number;
}

export interface GridLetterFlourish {
  wordIdx: number;
  charIdx: number;
  timer: number;
}

export interface ConfettiParticle {
  position: Vec2;
  velocity: Vec2;
  angularVelocity: number;
  lifetime: number;
  initialLifetime: number;
  rotation: number;
  size: Vec2;
  color: Color;
}

export interface Balloon {
  position: Vec2;
  swayAmount: number;
  swaySpeed: number;
  swayTimer: number;
  riseSpeed: number;
  timeToDisappear: number;
  radius: number;
  color: Color;
  stringLength: number;
}

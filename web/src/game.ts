import { Assets } from "./assets";
import {
  BONUS_POPUP_SCROLL_SPEED,
  COL_PAD,
  CONTINUE_BTN_OFFSET_Y,
  EASY_MAX_SOLUTIONS,
  EASY_PUZZLE_COUNT,
  GUESS_DISPLAY_GAP,
  GUESS_DISPLAY_OFFSET_Y,
  GRID_COLUMN_DIVIDER_ROWS,
  GRID_COLUMN_DIVIDER_WIDTH,
  GRID_TILE_RELATIVE_SCALE_WHEN_SHRUNK,
  GRID_ZONE_PADDING_X_DESIGN,
  GRID_ZONE_PADDING_Y_DESIGN,
  GRID_ZONE_RECT_DESIGN,
  HARD_MAX_SOLUTIONS,
  HARD_MIN_WORD_LENGTH,
  HARD_PUZZLE_COUNT,
  HINT_COST_REVEAL_FIRST,
  HINT_COST_REVEAL_FIRST_OF_EACH,
  HINT_COST_REVEAL_LAST,
  HINT_COST_REVEAL_RANDOM,
  HINT_POPUP_HEIGHT_DESIGN,
  HINT_POPUP_LINE_SPACING_DESIGN,
  HINT_POPUP_PADDING_DESIGN,
  HINT_POPUP_WIDTH_DESIGN,
  HINT_ZONE_RECT_DESIGN,
  LETTER_R,
  LETTER_R_BASE_DESIGN,
  MAJOR_COL_SPACING_BASE,
  MAX_LETTER_RADIUS_FACTOR,
  MAX_MINOR_COLS_PER_GROUP,
  MEDIUM_MAX_SOLUTIONS,
  MEDIUM_PUZZLE_COUNT,
  MENU_BUTTON_HEIGHT_DESIGN,
  MENU_BUTTON_SPACING_DESIGN,
  MENU_BUTTON_WIDTH_DESIGN,
  MENU_PANEL_EXTRA_HEIGHT_DESIGN,
  MENU_PANEL_EXTRA_WIDTH_DESIGN,
  MENU_PANEL_PADDING_DESIGN,
  MIN_DESIRED_GRID_WORDS,
  MIN_LETTER_RADIUS_FACTOR,
  MIN_WORD_LENGTH,
  POPUP_CORNER_RADIUS_BASE,
  POPUP_MAX_HEIGHT_DESIGN_RATIO,
  POPUP_MAX_WIDTH_DESIGN_RATIO,
  POPUP_MIN_TEXT_SCALE,
  POPUP_MAX_TEXT_SCALE,
  POPUP_PADDING_BASE,
  POPUP_SCROLL_BOTTOM_BUFFER,
  POPUP_SCREEN_MARGIN_DESIGN,
  PROGRESS_METER_HEIGHT_DESIGN,
  REF_H,
  REF_W,
  RETURN_BTN_HEIGHT_DESIGN,
  RETURN_BTN_WIDTH_DESIGN,
  TITLE_BOTTOM_MARGIN_BASE,
  SCORE_FLOURISH_FONT_SIZE_BASE_DESIGN,
  SCORE_FLOURISH_DURATION,
  SCORE_FLOURISH_LIFETIME_MAX_SEC,
  SCORE_FLOURISH_LIFETIME_MIN_SEC,
  SCORE_FLOURISH_SCALE,
  SCORE_FLOURISH_VEL_X_RANGE_DESIGN,
  SCORE_FLOURISH_VEL_Y_MAX_DESIGN,
  SCORE_FLOURISH_VEL_Y_MIN_DESIGN,
  SCORE_LABEL_VALUE_GAP_DESIGN,
  SCORE_ZONE_LABEL_FONT_SIZE,
  SCORE_ZONE_PADDING_Y_DESIGN,
  SCORE_ZONE_RECT_DESIGN,
  SCORE_ZONE_VALUE_FONT_SIZE,
  SCRAMBLE_BTN_HEIGHT,
  SCRAMBLE_BTN_OFFSET_X,
  SCRAMBLE_BTN_OFFSET_Y,
  TILE_PAD,
  TILE_SIZE,
  TOP_BAR_PADDING_X_DESIGN,
  TOP_BAR_ZONE_DESIGN,
  UI_SCALE_MODIFIER,
  WORD_LINE_SPACING_BASE,
  POPUP_WORD_FONT_SIZE_BASE,
  POPUP_TITLE_FONT_SIZE_BASE,
  MINOR_COL_SPACING_BASE,
  WORD_INFO_POPUP_LINE_SPACING_DESIGN,
  WORD_INFO_POPUP_MAX_WIDTH_DESIGN,
  WORD_INFO_POPUP_OFFSET_FROM_MOUSE_DESIGN,
  WORD_INFO_POPUP_PADDING_DESIGN,
  WHEEL_BG_PADDING_AROUND_LETTERS_DESIGN,
  WHEEL_LETTER_FONT_SIZE_BASE_DESIGN,
  WHEEL_LETTER_VISUAL_SCALE,
  WHEEL_TOUCH_SCALE_FACTOR,
  WHEEL_R,
  WHEEL_ZONE_PADDING_DESIGN,
  WHEEL_ZONE_RECT_DESIGN
} from "./core/constants";
import {
  AnimTarget,
  DifficultyLevel,
  GState,
  GameScreen,
  HintType,
  type Balloon,
  type Color,
  type ConfettiParticle,
  type GridLetterFlourish,
  type HintPointAnimParticle,
  type LetterAnim,
  type Rect,
  type ScoreFlourishParticle,
  type Vec2,
  type WordInfo
} from "./core/types";
import { loadThemes, type ColorTheme } from "./core/themes";
import {
  clamp,
  colorToCss,
  distSq,
  lerp,
  randRange,
  rectContains,
  shuffle
} from "./core/utils";
import { loadProcessedWordList, sortForGrid, subWords, withLength } from "./data/words";
import { DecorLayer } from "./render/decorLayer";
import { drawCenteredText, drawRoundedRect, wrapTextForWidth } from "./render/draw";
import { applyView, createViewTransform, screenToWorld, type ViewTransform } from "./render/view";

const HINT_LABELS = ["Letter", "Random", "Full Word", "1st of Each"];
const HINT_LABEL_FONT_BASE = 75;
const HINT_LABEL_X_REL = 0.33;
const HINT_LABEL_Y_REL = 0.5;
const HINT_LIGHT_CENTER_X_REL = 0.15;
const HINT_LIGHT_CENTER_Y_REL = 0.46;
const HINT_LIGHT_DIAMETER_REL = 0.55;
const HINT_FRAME_VERTICAL_SPACING = 2;
const HINT_FRAME_MIN_HEIGHT = 20;
const HINT_ZONE_SIDE_PADDING = 4;
const HINT_ZONE_BOTTOM_PADDING = 5;
const HINT_ZONE_BONUS_TEXT_SIZE = 20;
const HINT_ZONE_BONUS_TEXT_PADDING_TOP = 5;
const HINT_ZONE_BONUS_TEXT_GAP = 5;
const HINT_COSTS = [
  HINT_COST_REVEAL_FIRST,
  HINT_COST_REVEAL_RANDOM,
  HINT_COST_REVEAL_LAST,
  HINT_COST_REVEAL_FIRST_OF_EACH
];
const HINT_TYPES = [
  HintType.RevealFirst,
  HintType.RevealRandom,
  HintType.RevealLast,
  HintType.RevealFirstOfEach
];
const HINT_DESCRIPTIONS = [
  "This hint will reveal the next available letter.",
  "This hint will reveal one random letter for each word.",
  "This hint will reveal the last word that hasn't been revealed.",
  "This hint will reveal the first unrevealed letter for every word."
];
const UI_ORANGE: Color = { r: 255, g: 190, b: 70, a: 255 };
const UI_WHITE: Color = { r: 255, g: 255, b: 255, a: 255 };

export class Game {
  private width = 0;
  private height = 0;
  private view: ViewTransform = createViewTransform(1, 1, REF_W, REF_H);
  private uiScale = 1;
  private fontFamily = "WordPuzzleFont";
  private dpr = 1;

  private ready = false;
  private loadingError: string | null = null;
  private hasUserInteracted = false;

  private images: Record<string, HTMLImageElement> = {};
  private sounds: Record<string, HTMLAudioElement> = {};
  private backgroundMusic: HTMLAudioElement | null = null;

  private themes: ColorTheme[] = [];
  private currentTheme: ColorTheme = loadThemes()[0];

  private decor = new DecorLayer(10);

  private currentScreen: GameScreen = GameScreen.MainMenu;
  private gameState: GState = GState.Playing;

  private mousePos: Vec2 = { x: 0, y: 0 };

  private fullWordList: WordInfo[] = [];
  private roots: WordInfo[] = [];
  private base = "";
  private solutions: WordInfo[] = [];
  private sorted: WordInfo[] = [];
  private grid: string[][] = [];
  private allPotentialSolutions: WordInfo[] = [];
  private found = new Set<string>();
  private foundBonusWords = new Set<string>();
  private usedBaseWordsThisSession = new Set<string>();
  private usedLetterSetsThisSession = new Set<string>();

  private hintPoints = 0;
  private hintsAvailable = 0;
  private wordsSolvedSinceHint = 0;
  private currentScore = 0;

  private puzzlesPerSession = 0;
  private currentPuzzleIndex = 0;
  private isInSession = false;
  private selectedDifficulty: DifficultyLevel = DifficultyLevel.None;

  private bonusWordsPopupScrollOffset = 0;
  private bonusWordsPopupMaxScrollOffset = 0;
  private cachedBonusWords: WordInfo[] = [];
  private bonusWordsCacheIsValid = false;
  private bonusTextFlourishTimer = 0;

  private hintFrameClickAnimTimers = [0, 0, 0, 0];
  private hintClickableRegions: Rect[] = [];
  private hoveredHintIndex = -1;
  private isHoveringHintPointsText = false;
  private hoveredSolvedWordIndex = -1;

  private dragging = false;
  private path: number[] = [];
  private currentGuess = "";

  private letterPositionRadius = 0;
  private visualBgRadius = 0;
  private wheelLetterRenderPos: Vec2[] = [];
  private currentWheelRadius = 0;
  private currentLetterRenderRadius = 0;
  private wheelCenter: Vec2 = { x: 0, y: 0 };
  private wheelTouchScaleActive = false;

  private gridStartX = 0;
  private gridStartY = 0;
  private currentGridLayoutScale = 1;
  private wordCol: number[] = [];
  private wordRow: number[] = [];
  private colMaxLen: number[] = [];
  private colXOffset: number[] = [];

  private scoreFlourishes: ScoreFlourishParticle[] = [];
  private hintPointAnims: HintPointAnimParticle[] = [];
  private scoreFlourishTimer = 0;
  private gridFlourishes: GridLetterFlourish[] = [];
  private letterAnims: LetterAnim[] = [];

  private confetti: ConfettiParticle[] = [];
  private balloons: Balloon[] = [];
  private celebrationTimer = 0;

  private mainMenuButtons: Rect[] = [];
  private casualMenuButtons: Rect[] = [];
  private returnToMenuButton: Rect = { x: 0, y: 0, width: 0, height: 0 };
  private continueButton: Rect = { x: 0, y: 0, width: 0, height: 0 };
  private scrambleButton: Rect = { x: 0, y: 0, width: 0, height: 0 };
  private bonusWordsTextRect: Rect = { x: 0, y: 0, width: 0, height: 0 };

  constructor(
    private readonly canvas: HTMLCanvasElement,
    private readonly ctx: CanvasRenderingContext2D
  ) {
    this.attachInput();
    this.init().catch((err) => {
      this.loadingError = err instanceof Error ? err.message : "Failed to initialize game.";
    });
  }

  onResize(width: number, height: number, dpr = 1) {
    this.width = width;
    this.height = height;
    this.dpr = dpr;
    this.view = createViewTransform(width, height, REF_W, REF_H);
    this.uiScale = clamp(Math.min(width / REF_W, height / REF_H), 0.65, 1.6);
    this.updateLayout();
  }

  update(dt: number) {
    if (!this.ready) return;

    this.decor.update(dt, { x: REF_W, y: REF_H }, this.currentTheme);

    if (this.scoreFlourishTimer > 0) {
      this.scoreFlourishTimer = Math.max(0, this.scoreFlourishTimer - dt);
    }
    if (this.bonusTextFlourishTimer > 0) {
      this.bonusTextFlourishTimer = Math.max(0, this.bonusTextFlourishTimer - dt);
    }

    this.gridFlourishes = this.gridFlourishes.filter((f) => {
      f.timer -= dt;
      return f.timer > 0;
    });

    for (let i = 0; i < this.hintFrameClickAnimTimers.length; i += 1) {
      if (this.hintFrameClickAnimTimers[i] > 0) {
        this.hintFrameClickAnimTimers[i] = Math.max(0, this.hintFrameClickAnimTimers[i] - dt);
      }
    }

    this.updateLetterAnims(dt);
    this.updateScoreFlourishes(dt);
    this.updateHintPointAnims(dt);
    this.updateCelebrationEffects(dt);

    if (this.currentScreen === GameScreen.Playing || this.currentScreen === GameScreen.GameOver) {
      this.hoveredHintIndex = -1;
      for (let i = 0; i < this.hintClickableRegions.length; i += 1) {
        if (rectContains(this.hintClickableRegions[i], this.mousePos)) {
          this.hoveredHintIndex = i;
          break;
        }
      }
      this.isHoveringHintPointsText = rectContains(this.bonusWordsTextRect, this.mousePos);
      this.hoveredSolvedWordIndex = -1;
      if (this.sorted.length > 0 && this.grid.length > 0 && this.found.size > 0) {
        const tileSize = TILE_SIZE * this.currentGridLayoutScale;
        for (let w = 0; w < this.sorted.length && w < this.grid.length; w += 1) {
          if (!this.found.has(this.sorted[w].text)) continue;
          for (let c = 0; c < this.sorted[w].text.length && c < this.grid[w].length; c += 1) {
            const pos = this.tilePos(w, c);
            const tileRect = { x: pos.x, y: pos.y, width: tileSize, height: tileSize };
            if (rectContains(tileRect, this.mousePos)) {
              this.hoveredSolvedWordIndex = w;
              break;
            }
          }
          if (this.hoveredSolvedWordIndex !== -1) break;
        }
      }
    } else {
      this.hoveredHintIndex = -1;
      this.isHoveringHintPointsText = false;
      this.bonusWordsPopupScrollOffset = 0;
      this.hoveredSolvedWordIndex = -1;
    }
  }

  render() {
    const ctx = this.ctx;
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
    ctx.setTransform(this.dpr, 0, 0, this.dpr, 0, 0);
    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, this.width, this.height);

    if (this.loadingError) {
      ctx.fillStyle = "#fff";
      ctx.font = "18px sans-serif";
      ctx.fillText(this.loadingError, 20, 30);
      return;
    }
    if (!this.ready) {
      ctx.fillStyle = "#fff";
      ctx.font = "18px sans-serif";
      ctx.fillText("Loading...", 20, 30);
      return;
    }

    applyView(ctx, this.view, this.dpr);
    ctx.fillStyle = colorToCss(this.currentTheme.winBg);
    ctx.fillRect(0, 0, REF_W, REF_H);

    this.drawBackground(ctx);
    // this.decor.draw(ctx); // Disabled - old background decor code (circles, triangles, lines)

    if (this.currentScreen === GameScreen.MainMenu) {
      this.renderMainMenu(ctx);
    } else if (this.currentScreen === GameScreen.CasualMenu) {
      this.renderCasualMenu(ctx);
    } else if (this.currentScreen === GameScreen.SessionComplete) {
      this.renderSessionComplete(ctx);
    } else {
      this.renderGameScreen(ctx);
    }

    if (this.isHoveringHintPointsText && (this.currentScreen === GameScreen.Playing || this.currentScreen === GameScreen.GameOver)) {
      this.renderBonusWordsPopup(ctx);
    }
  }

  private async init() {
    this.themes = loadThemes();
    this.currentTheme = this.themes[0] ?? this.currentTheme;

    await Promise.all([this.loadFont(), this.loadImages(), this.loadSounds(), this.loadWords()]);
    this.ready = true;
    this.updateLayout();
  }

  private async loadFont() {
    const font = new FontFace(this.fontFamily, `url(${Assets.fonts.arialBold})`);
    await font.load();
    document.fonts.add(font);
  }

  private async loadImages() {
    const imageMap: Record<string, string> = {
      menuBackground: Assets.menuBackground,
      menuButton: Assets.menuButton,
      mainBackground: Assets.mainBackground,
      hintFrame: Assets.hintFrame,
      hintIndicator: Assets.hintIndicator,
      scrambleButton: Assets.scrambleButton,
      gridButton: Assets.gridButton,
      circularLetterFrame: Assets.circularLetterFrame,
      sapphire: Assets.sapphire,
      ruby: Assets.ruby,
      diamond: Assets.diamond
    };

    const entries = Object.entries(imageMap);
    await Promise.all(
      entries.map(async ([key, url]) => {
        this.images[key] = await this.loadImage(url);
      })
    );
  }

  private async loadSounds() {
    const entries = Object.entries(Assets.sounds);
    for (const [key, url] of entries) {
      const audio = new Audio(url);
      audio.preload = "auto";
      this.sounds[key] = audio;
    }
  }

  private async loadWords() {
    this.fullWordList = await loadProcessedWordList(Assets.wordsCsv);
    this.roots = [];
    const lengths = [4, 5, 6, 7];
    for (const len of lengths) {
      this.roots.push(...withLength(this.fullWordList, len));
    }
  }

  private async loadImage(url: string) {
    const img = new Image();
    img.src = url;
    await img.decode().catch(
      () =>
        new Promise<void>((resolve, reject) => {
          img.onload = () => resolve();
          img.onerror = () => reject(new Error(`Failed to load image ${url}`));
        })
    );
    return img;
  }

  private attachInput() {
    const onPointerDown = (ev: PointerEvent) => {
      this.hasUserInteracted = true;
      const world = this.eventToWorld(ev);
      this.mousePos = world;
      this.canvas.setPointerCapture(ev.pointerId);
      this.handlePointerDown(world, ev.pointerType);
    };

    const onPointerMove = (ev: PointerEvent) => {
      const world = this.eventToWorld(ev);
      this.mousePos = world;
      this.handlePointerMove(world, ev.pointerType);
    };

    const onPointerUp = (ev: PointerEvent) => {
      const world = this.eventToWorld(ev);
      this.mousePos = world;
      this.canvas.releasePointerCapture(ev.pointerId);
      this.handlePointerUp(world, ev.pointerType);
    };

    const onWheel = (ev: WheelEvent) => {
      if (this.isHoveringHintPointsText) {
        this.bonusWordsPopupScrollOffset = clamp(
          this.bonusWordsPopupScrollOffset + Math.sign(ev.deltaY) * BONUS_POPUP_SCROLL_SPEED,
          0,
          this.bonusWordsPopupMaxScrollOffset
        );
        ev.preventDefault();
      }
    };

    this.canvas.addEventListener("pointerdown", onPointerDown);
    this.canvas.addEventListener("pointermove", onPointerMove);
    this.canvas.addEventListener("pointerup", onPointerUp);
    this.canvas.addEventListener("wheel", onWheel, { passive: false });
  }

  private eventToWorld(ev: PointerEvent) {
    const rect = this.canvas.getBoundingClientRect();
    const x = ev.clientX - rect.left;
    const y = ev.clientY - rect.top;
    return screenToWorld(this.view, { x, y });
  }

  private handlePointerDown(world: Vec2, pointerType: string) {
    if (!this.ready) return;
    this.updateWheelTouchScale(world, pointerType);

    if (this.currentScreen === GameScreen.MainMenu) {
      this.handleMainMenuInput(world);
      return;
    }
    if (this.currentScreen === GameScreen.CasualMenu) {
      this.handleCasualMenuInput(world);
      return;
    }
    if (this.currentScreen === GameScreen.SessionComplete) {
      if (rectContains(this.continueButton, world)) {
        this.playSound("click");
        // Stop celebration effects & transition to main menu
        this.confetti = [];
        this.balloons = [];
        this.currentScreen = GameScreen.MainMenu;
        this.isInSession = false;
        this.selectedDifficulty = DifficultyLevel.None;
      }
      return;
    }

    if (this.currentScreen === GameScreen.GameOver) {
      if (rectContains(this.continueButton, world)) {
        this.handleContinue();
      }
      return;
    }

    if (rectContains(this.returnToMenuButton, world)) {
      this.playSound("click");
      this.currentScreen = GameScreen.MainMenu;
      this.isInSession = false;
      this.selectedDifficulty = DifficultyLevel.None;
      this.clearDragState();
      return;
    }

    if (rectContains(this.scrambleButton, world)) {
      this.playSound("click");
      this.base = shuffle(this.base.split("")).join("");
      this.updateLayout();
      this.clearDragState();
      return;
    }

    for (let i = 0; i < this.hintClickableRegions.length; i += 1) {
      if (rectContains(this.hintClickableRegions[i], world)) {
        this.hintFrameClickAnimTimers[i] = 0.15;
        if (this.hintPoints >= HINT_COSTS[i]) {
          this.hintPoints -= HINT_COSTS[i];
          this.activateHint(HINT_TYPES[i]);
        } else {
          this.playSound("error");
        }
        return;
      }
    }

    const touchScale = pointerType === "touch" && this.wheelTouchScaleActive ? WHEEL_TOUCH_SCALE_FACTOR : 1;
    const hitRadius = this.currentLetterRenderRadius * touchScale;
    const hitRadiusSq = hitRadius * hitRadius;
    for (let i = 0; i < this.base.length; i += 1) {
      const pos = this.getWheelLetterPosition(i);
      if (pos && distSq(world, pos) < hitRadiusSq) {
        this.dragging = true;
        this.path = [i];
        this.currentGuess = this.base[i].toUpperCase();
        this.playSound("select");
        return;
      }
    }
  }

  private handlePointerMove(world: Vec2, pointerType: string) {
    this.updateWheelTouchScale(world, pointerType);
    if (!this.dragging) return;

    const touchScale = pointerType === "touch" && this.wheelTouchScaleActive ? WHEEL_TOUCH_SCALE_FACTOR : 1;
    const hitRadius = this.currentLetterRenderRadius * touchScale;
    const hitRadiusSq = hitRadius * hitRadius;
    for (let i = 0; i < this.base.length; i += 1) {
      const pos = this.getWheelLetterPosition(i, touchScale);
      if (!pos) continue;
      if (distSq(world, pos) < hitRadiusSq) {
        const index = i;
        const existingIndex = this.path.indexOf(index);
        if (existingIndex === -1) {
          this.path.push(index);
          this.currentGuess += this.base[i].toUpperCase();
          this.playSound("select");
        } else if (this.path.length >= 2 && this.path[this.path.length - 2] === index) {
          this.path.pop();
          this.currentGuess = this.currentGuess.slice(0, -1);
        }
        break;
      }
    }
  }

  private handlePointerUp(_world: Vec2, _pointerType: string) {
    this.wheelTouchScaleActive = false;
    if (!this.dragging) return;

    const minGuessLength = 3;
    if (this.currentGuess.length < minGuessLength) {
      this.clearDragState();
      return;
    }

    let actionTaken = false;
    let wordIndexMatched = -1;

    for (let w = 0; w < this.sorted.length; w += 1) {
      const solution = this.sorted[w].text.toUpperCase();
      if (solution === this.currentGuess) {
        wordIndexMatched = w;
        if (this.found.has(this.sorted[w].text)) {
          for (let c = 0; c < solution.length; c += 1) {
            this.gridFlourishes.push({ wordIdx: w, charIdx: c, timer: 0.6 });
          }
          this.playSound("place");
          actionTaken = true;
        } else {
          this.found.add(this.sorted[w].text);
          const baseScore = this.currentGuess.length * 10;
          const rarityBonus = this.sorted[w].rarity > 1 ? this.sorted[w].rarity * 25 : 0;
          const wordScore = baseScore + rarityBonus;
          this.currentScore += wordScore;
          this.spawnScoreFlourish(wordScore, w);
          this.scoreFlourishTimer = SCORE_FLOURISH_DURATION;

          for (let c = 0; c < this.currentGuess.length; c += 1) {
            if (c < this.path.length) {
              const pathIdx = this.path[c];
              const startPos = this.wheelLetterRenderPos[pathIdx];
              const endPos = this.tilePos(w, c);
              const finalTileSize =
                TILE_SIZE * this.currentGridLayoutScale * (this.currentGridLayoutScale < 1 ? GRID_TILE_RELATIVE_SCALE_WHEN_SHRUNK : 1);
              this.letterAnims.push({
                ch: this.currentGuess[c],
                start: startPos,
                end: { x: endPos.x + finalTileSize / 2, y: endPos.y + finalTileSize / 2 },
                t: 0 - c * 0.03,
                wordIdx: w,
                charIdx: c,
                target: AnimTarget.Grid
              });
            }
          }

          if (this.found.size === this.solutions.length) {
            this.gameState = GState.Solved;
            this.currentScreen = GameScreen.GameOver;
          }
          actionTaken = true;
        }
        break;
      }
    }

    if (!actionTaken) {
      for (const bonus of this.allPotentialSolutions) {
        if (bonus.text.toUpperCase() === this.currentGuess) {
          if (this.foundBonusWords.has(bonus.text)) {
            this.bonusTextFlourishTimer = 0.6;
            this.playSound("place");
          } else {
            this.foundBonusWords.add(bonus.text);
            const len = bonus.text.length;
            let hintAward = 0;
            if (len === 3) hintAward = 1;
            else if (len === 4) hintAward = 2;
            else if (len === 5) hintAward = 3;
            else if (len >= 6) hintAward = 4;
            if (hintAward > 0) {
              this.hintPoints += hintAward;
              this.spawnHintPointAnimation(this.bonusWordsTextRect, hintAward);
            }
            this.playSound("place");
          }
          actionTaken = true;
          break;
        }
      }
    }

    if (!actionTaken) {
      this.playSound("error");
    }

    this.clearDragState();
  }

  private getWheelLetterPosition(index: number, scale = 1) {
    const basePos = this.wheelLetterRenderPos[index];
    if (!basePos || scale === 1) return basePos;
    const dx = basePos.x - this.wheelCenter.x;
    const dy = basePos.y - this.wheelCenter.y;
    return { x: this.wheelCenter.x + dx * scale, y: this.wheelCenter.y + dy * scale };
  }

  private getWheelTouchRadius(scale = 1) {
    const ringRadius = this.letterPositionRadius * scale;
    const visualRadius = this.currentLetterRenderRadius * WHEEL_LETTER_VISUAL_SCALE * scale;
    return ringRadius + visualRadius;
  }

  private updateWheelTouchScale(world: Vec2, pointerType: string) {
    if (!this.ready || !this.base || this.base.length === 0) {
      this.wheelTouchScaleActive = false;
      return;
    }
    if (pointerType !== "touch") {
      this.wheelTouchScaleActive = false;
      return;
    }
    if (this.currentScreen !== GameScreen.Playing && this.currentScreen !== GameScreen.GameOver) {
      this.wheelTouchScaleActive = false;
      return;
    }

    if (this.wheelTouchScaleActive) return;
    const radius = this.getWheelTouchRadius(1);
    this.wheelTouchScaleActive = distSq(world, this.wheelCenter) <= radius * radius;
  }

  private handleMainMenuInput(world: Vec2) {
    if (this.mainMenuButtons.length < 3) return;
    if (rectContains(this.mainMenuButtons[0], world)) {
      this.playSound("click");
      this.currentScreen = GameScreen.CasualMenu;
      return;
    }
    if (rectContains(this.mainMenuButtons[2], world)) {
      this.playSound("click");
      this.currentScreen = GameScreen.MainMenu;
    }
  }

  private handleCasualMenuInput(world: Vec2) {
    if (this.casualMenuButtons.length < 4) return;

    const [easy, medium, hard, back] = this.casualMenuButtons;
    if (rectContains(back, world)) {
      this.playSound("click");
      this.currentScreen = GameScreen.MainMenu;
      return;
    }

    let selected = DifficultyLevel.None;
    let puzzles = 0;
    if (rectContains(easy, world)) {
      selected = DifficultyLevel.Easy;
      puzzles = EASY_PUZZLE_COUNT;
    } else if (rectContains(medium, world)) {
      selected = DifficultyLevel.Medium;
      puzzles = MEDIUM_PUZZLE_COUNT;
    } else if (rectContains(hard, world)) {
      selected = DifficultyLevel.Hard;
      puzzles = HARD_PUZZLE_COUNT;
    }

    if (selected !== DifficultyLevel.None) {
      this.playSound("click");
      this.selectedDifficulty = selected;
      this.puzzlesPerSession = puzzles;
      this.currentPuzzleIndex = 0;
      this.isInSession = true;
      this.usedBaseWordsThisSession.clear();
      this.usedLetterSetsThisSession.clear();
      this.rebuild();
      this.currentScreen = GameScreen.Playing;
    }
  }

  private handleContinue() {
    if (this.isInSession) {
      if (this.currentPuzzleIndex + 1 < this.puzzlesPerSession) {
        this.currentPuzzleIndex += 1;
        this.rebuild();
        this.currentScreen = GameScreen.Playing;
      } else {
        this.currentScreen = GameScreen.SessionComplete;
        this.startCelebrationEffects();
      }
    } else {
      this.currentScreen = GameScreen.MainMenu;
    }
  }

  private rebuild() {
    if (this.themes.length > 0) {
      this.currentTheme = this.themes[Math.floor(Math.random() * this.themes.length)];
    }
    const criteria = this.getCriteriaForCurrentPuzzle();
    let maxSolutionsForDifficulty = 0;
    let minSubLengthForDifficulty = MIN_WORD_LENGTH;

    switch (this.selectedDifficulty) {
      case DifficultyLevel.Easy:
        maxSolutionsForDifficulty = EASY_MAX_SOLUTIONS;
        minSubLengthForDifficulty = MIN_WORD_LENGTH;
        break;
      case DifficultyLevel.Medium:
        maxSolutionsForDifficulty = MEDIUM_MAX_SOLUTIONS;
        minSubLengthForDifficulty = MIN_WORD_LENGTH;
        break;
      case DifficultyLevel.Hard:
        maxSolutionsForDifficulty = HARD_MAX_SOLUTIONS;
        minSubLengthForDifficulty = HARD_MIN_WORD_LENGTH;
        break;
      default:
        maxSolutionsForDifficulty = 999;
        minSubLengthForDifficulty = MIN_WORD_LENGTH;
        break;
    }

    let selectedBaseWord = "";
    let baseWordForGrid = "";
    let baseFound = false;

    const roots = shuffle([...this.roots]);
    const idealCandidates: number[] = [];
    const fallbackCandidates: number[] = [];
    const broadFallback: number[] = [];

    for (let i = 0; i < roots.length; i += 1) {
      const candidate = roots[i];
      if (!criteria.allowedLengths.includes(candidate.text.length)) continue;
      if (!criteria.allowedRarities.includes(candidate.rarity)) continue;
      if (this.usedBaseWordsThisSession.has(candidate.text)) continue;
      const canonical = this.getCanonicalLetters(candidate.text);
      if (this.usedLetterSetsThisSession.has(canonical)) continue;

      let validSubCount = candidate.countGE4;
      if (this.selectedDifficulty === DifficultyLevel.Easy) validSubCount = candidate.easyValidCount;
      if (this.selectedDifficulty === DifficultyLevel.Medium) validSubCount = candidate.mediumValidCount;
      if (this.selectedDifficulty === DifficultyLevel.Hard) validSubCount = candidate.hardValidCount;

      if (validSubCount >= MIN_DESIRED_GRID_WORDS) {
        idealCandidates.push(i);
      } else {
        fallbackCandidates.push(i);
      }
    }

    const pickIndex = (list: number[]) => list[Math.floor(Math.random() * list.length)];

    if (idealCandidates.length > 0) {
      selectedBaseWord = roots[pickIndex(idealCandidates)].text;
      baseFound = true;
    } else if (fallbackCandidates.length > 0) {
      selectedBaseWord = roots[pickIndex(fallbackCandidates)].text;
      baseFound = true;
    } else {
      for (let i = 0; i < roots.length; i += 1) {
        const candidate = roots[i];
        if (this.usedBaseWordsThisSession.has(candidate.text)) continue;
        const canonical = this.getCanonicalLetters(candidate.text);
        if (this.usedLetterSetsThisSession.has(canonical)) continue;
        broadFallback.push(i);
      }
      if (broadFallback.length > 0) {
        selectedBaseWord = roots[pickIndex(broadFallback)].text;
        baseFound = true;
      } else if (roots.length > 0) {
        selectedBaseWord = roots[0].text;
        baseFound = true;
      }
    }

    if (baseFound && selectedBaseWord) {
      const canonical = this.getCanonicalLetters(selectedBaseWord);
      this.usedBaseWordsThisSession.add(selectedBaseWord);
      this.usedLetterSetsThisSession.add(canonical);
    }

    this.base = selectedBaseWord || "ERROR";
    baseWordForGrid = selectedBaseWord;
    if (this.base !== "ERROR") {
      this.base = shuffle(this.base.split("")).join("");
    }

    let finalSolutions: WordInfo[] = [];
    if (this.base !== "ERROR") {
      this.allPotentialSolutions = subWords(this.base, this.fullWordList);
      const allowedSubRarities =
        this.selectedDifficulty === DifficultyLevel.Easy
          ? [1, 2]
          : this.selectedDifficulty === DifficultyLevel.Medium
            ? [1, 2, 3]
            : this.selectedDifficulty === DifficultyLevel.Hard
              ? [2, 3, 4]
              : [1, 2, 3, 4];

      const filtered = this.allPotentialSolutions.filter((info) => {
        if (info.text.length < minSubLengthForDifficulty) return false;
        return allowedSubRarities.includes(info.rarity);
      });

      const unique = new Map<string, WordInfo>();
      for (const info of filtered) {
        if (!unique.has(info.text)) unique.set(info.text, info);
      }

      const sortedUnique = Array.from(unique.values()).sort((a, b) => {
        if (a.text.length !== b.text.length) return b.text.length - a.text.length;
        if (a.rarity !== b.rarity) return a.rarity - b.rarity;
        return a.text.localeCompare(b.text);
      });

      if (sortedUnique.length > maxSolutionsForDifficulty) {
        sortedUnique.length = maxSolutionsForDifficulty;
      }
      finalSolutions = [...sortedUnique];

      if (baseWordForGrid) {
        const baseLower = baseWordForGrid.toLowerCase();
        if (!finalSolutions.some((info) => info.text === baseLower)) {
          const baseInfo =
            this.roots.find((info) => info.text === baseLower) ??
            this.fullWordList.find((info) => info.text === baseLower) ?? {
              text: baseLower,
              rarity: 0,
              pos: "",
              definition: "",
              sentence: "",
              avgSubLen: 0,
              countGE3: 0,
              countGE4: 0,
              countGE5: 0,
              easyValidCount: 0,
              mediumValidCount: 0,
              hardValidCount: 0
            };

          if (baseInfo.text.length >= minSubLengthForDifficulty) {
            finalSolutions.push(baseInfo);
          }
        }
      }

      const minGridTarget = Math.min(MIN_DESIRED_GRID_WORDS, maxSolutionsForDifficulty);
      if (finalSolutions.length < minGridTarget) {
        const existing = new Set(finalSolutions.map((info) => info.text));
        const fallback = this.allPotentialSolutions
          .filter((info) => info.text.length >= minSubLengthForDifficulty && !existing.has(info.text))
          .sort((a, b) => {
            if (a.text.length !== b.text.length) return b.text.length - a.text.length;
            if (a.rarity !== b.rarity) return a.rarity - b.rarity;
            return a.text.localeCompare(b.text);
          });
        for (const info of fallback) {
          finalSolutions.push(info);
          existing.add(info.text);
          if (finalSolutions.length >= minGridTarget) break;
        }
      }
    } else {
      this.allPotentialSolutions = [];
    }

    this.solutions = finalSolutions;
    this.sorted = sortForGrid(this.solutions);
    this.grid = this.sorted.map((info) => Array(info.text.length).fill("_"));

    this.found.clear();
    this.foundBonusWords.clear();
    this.letterAnims = [];
    this.scoreFlourishes = [];
    this.hintPointAnims = [];
    this.gridFlourishes = [];
    this.gameState = GState.Playing;
    this.bonusWordsCacheIsValid = false;
    this.bonusWordsPopupScrollOffset = 0;

    if (this.currentPuzzleIndex === 0 && this.isInSession) {
      this.currentScore = 0;
      this.wordsSolvedSinceHint = 0;
    } else if (this.isInSession) {
      this.wordsSolvedSinceHint = 0;
    }

    this.updateLayout();
    // this.startBackgroundMusic(); // Disabled - music gets repetitive
  }

  private updateLayout() {
    this.updateMenuLayout();
    this.updateGridLayout();
    this.updateWheelLayout();
    this.updateHintLayout();
    this.updateTopBarLayout();
  }

  private updateMenuLayout() {
    const titleSize = this.scale(36);
    const titleHeight = titleSize + 10;
    const panelWidth = MENU_BUTTON_WIDTH_DESIGN + MENU_PANEL_PADDING_DESIGN * 2 + MENU_PANEL_EXTRA_WIDTH_DESIGN;
    const mainButtons = 3;
    const panelHeight =
      MENU_PANEL_PADDING_DESIGN * 2 +
      titleHeight +
      mainButtons * MENU_BUTTON_HEIGHT_DESIGN +
      (mainButtons - 1) * MENU_BUTTON_SPACING_DESIGN +
      MENU_PANEL_EXTRA_HEIGHT_DESIGN;
    const panelX = (REF_W - panelWidth) / 2;
    const panelY = (REF_H - panelHeight) / 2;

    this.mainMenuButtons = [];
    for (let i = 0; i < mainButtons; i += 1) {
      const x = panelX + MENU_PANEL_PADDING_DESIGN;
      const y =
        panelY +
        MENU_PANEL_PADDING_DESIGN +
        titleHeight +
        i * (MENU_BUTTON_HEIGHT_DESIGN + MENU_BUTTON_SPACING_DESIGN);
      this.mainMenuButtons.push({
        x,
        y,
        width: MENU_BUTTON_WIDTH_DESIGN,
        height: MENU_BUTTON_HEIGHT_DESIGN
      });
    }

    const casualButtons = 4;
    const casualPanelHeight =
      MENU_PANEL_PADDING_DESIGN * 2 +
      titleHeight +
      casualButtons * MENU_BUTTON_HEIGHT_DESIGN +
      (casualButtons - 1) * MENU_BUTTON_SPACING_DESIGN +
      MENU_PANEL_EXTRA_HEIGHT_DESIGN;
    const casualPanelY = (REF_H - casualPanelHeight) / 2;

    this.casualMenuButtons = [];
    for (let i = 0; i < casualButtons; i += 1) {
      const x = panelX + MENU_PANEL_PADDING_DESIGN;
      const y =
        casualPanelY +
        MENU_PANEL_PADDING_DESIGN +
        titleHeight +
        i * (MENU_BUTTON_HEIGHT_DESIGN + MENU_BUTTON_SPACING_DESIGN);
      this.casualMenuButtons.push({
        x,
        y,
        width: MENU_BUTTON_WIDTH_DESIGN,
        height: MENU_BUTTON_HEIGHT_DESIGN
      });
    }
  }

  private updateGridLayout() {
    const zoneInnerX = GRID_ZONE_RECT_DESIGN.x + GRID_ZONE_PADDING_X_DESIGN;
    const zoneInnerY = GRID_ZONE_RECT_DESIGN.y + GRID_ZONE_PADDING_Y_DESIGN;
    const zoneInnerWidth = GRID_ZONE_RECT_DESIGN.width - 2 * GRID_ZONE_PADDING_X_DESIGN;
    const zoneInnerHeight = GRID_ZONE_RECT_DESIGN.height - 2 * GRID_ZONE_PADDING_Y_DESIGN;

    const wordCount = this.sorted.length || 1;
    let numCols = 1;
    let maxRowsPerCol = wordCount;
    let gridScale = 1;

    if (this.sorted.length > 0) {
      const maxPossibleCols = Math.min(8, wordCount);
      let narrowestCols = 1;
      let narrowestRows = wordCount;
      let minWidth = Number.POSITIVE_INFINITY;

      for (let cols = 1; cols <= maxPossibleCols; cols += 1) {
        const rows = Math.ceil(wordCount / cols);
        const colMaxLen = Array(cols).fill(0);
        for (let w = 0; w < wordCount; w += 1) {
          const c = Math.floor(w / rows);
          colMaxLen[c] = Math.max(colMaxLen[c], this.sorted[w].text.length);
        }
        let width = 0;
        for (const len of colMaxLen) {
          const colWidth = len * (TILE_SIZE + TILE_PAD) - (len > 0 ? TILE_PAD : 0);
          width += Math.max(0, colWidth);
        }
        width += (cols - 1) * COL_PAD;
        if (width < minWidth) {
          minWidth = width;
          narrowestCols = cols;
          narrowestRows = rows;
        }
      }

      numCols = narrowestCols;
      maxRowsPerCol = narrowestRows;
      const MAX_ROWS_LIMIT_GRID = 5;
      if (maxRowsPerCol > MAX_ROWS_LIMIT_GRID) {
        maxRowsPerCol = MAX_ROWS_LIMIT_GRID;
        numCols = Math.ceil(wordCount / maxRowsPerCol);
      }

      this.colMaxLen = Array(numCols).fill(0);
      for (let w = 0; w < wordCount; w += 1) {
        const c = Math.min(numCols - 1, Math.floor(w / maxRowsPerCol));
        this.colMaxLen[c] = Math.max(this.colMaxLen[c], this.sorted[w].text.length);
      }

      let totalWidth = 0;
      for (let c = 0; c < numCols; c += 1) {
        const len = this.colMaxLen[c];
        const colWidth = len * (TILE_SIZE + TILE_PAD) - (len > 0 ? TILE_PAD : 0);
        totalWidth += Math.max(0, colWidth);
      }
      totalWidth += (numCols - 1) * COL_PAD;
      const totalHeight = maxRowsPerCol * (TILE_SIZE + TILE_PAD) - (maxRowsPerCol > 0 ? TILE_PAD : 0);

      const scaleToFitX = totalWidth > 0 ? Math.min(1, zoneInnerWidth / totalWidth) : 1;
      const scaleToFitY = totalHeight > 0 ? Math.min(1, zoneInnerHeight / totalHeight) : 1;
      gridScale = Math.min(scaleToFitX, scaleToFitY);

      const tileSize = TILE_SIZE * gridScale;
      const tilePad = TILE_PAD * gridScale;
      const colPad = COL_PAD * gridScale;

      this.colXOffset = [];
      let currentX = 0;
      for (let c = 0; c < numCols; c += 1) {
        this.colXOffset[c] = currentX;
        const len = this.colMaxLen[c];
        const colWidth = len * (tileSize + tilePad) - (len > 0 ? tilePad : 0);
        currentX += Math.max(0, colWidth) + colPad;
      }
      const totalGridW = currentX - (numCols > 0 ? colPad : 0);
      const totalGridH = maxRowsPerCol * (tileSize + tilePad) - (maxRowsPerCol > 0 ? tilePad : 0);

      this.gridStartX = zoneInnerX + (zoneInnerWidth - totalGridW) / 2;
      this.gridStartY = zoneInnerY + (zoneInnerHeight - totalGridH) / 2;

      for (let c = 0; c < this.colXOffset.length; c += 1) {
        this.colXOffset[c] += this.gridStartX;
      }

      this.wordCol = [];
      this.wordRow = [];
      for (let w = 0; w < wordCount; w += 1) {
        const col = Math.min(numCols - 1, Math.floor(w / maxRowsPerCol));
        const row = w % maxRowsPerCol;
        this.wordCol[w] = col;
        this.wordRow[w] = row;
      }
    }

    this.currentGridLayoutScale = gridScale;
  }

  private updateWheelLayout() {
    const innerX = WHEEL_ZONE_RECT_DESIGN.x + WHEEL_ZONE_PADDING_DESIGN;
    const innerY = WHEEL_ZONE_RECT_DESIGN.y + WHEEL_ZONE_PADDING_DESIGN;
    const innerW = WHEEL_ZONE_RECT_DESIGN.width - 2 * WHEEL_ZONE_PADDING_DESIGN;
    const innerH = WHEEL_ZONE_RECT_DESIGN.height - 2 * WHEEL_ZONE_PADDING_DESIGN;

    this.wheelCenter = { x: innerX + innerW / 2, y: innerY + innerH / 2 };
    const maxRadiusForZone = Math.min(innerW / 2, innerH / 2);
    this.currentWheelRadius = Math.max(Math.min(maxRadiusForZone, WHEEL_R), LETTER_R * 1.5);

    if (this.base.length > 0) {
      let radiusBasedOnCount = (Math.PI * this.currentWheelRadius) / this.base.length;
      radiusBasedOnCount *= 0.75;
      const scaleFactor = WHEEL_R > 0 ? this.currentWheelRadius / WHEEL_R : 1;
      const radiusBasedOnScale = LETTER_R_BASE_DESIGN * scaleFactor;
      let letterRadius = Math.min(radiusBasedOnCount, radiusBasedOnScale);
      const minAbs = this.currentWheelRadius * MIN_LETTER_RADIUS_FACTOR;
      const maxAbs = this.currentWheelRadius * MAX_LETTER_RADIUS_FACTOR;
      letterRadius = clamp(letterRadius, minAbs, maxAbs);
      this.currentLetterRenderRadius = Math.max(letterRadius, 5);
    } else {
      this.currentLetterRenderRadius = LETTER_R_BASE_DESIGN;
    }

    this.letterPositionRadius = Math.max(
      this.currentWheelRadius,
      this.currentLetterRenderRadius * 0.5,
      this.currentWheelRadius * 0.3
    );

    const letterSizeRatio =
      LETTER_R_BASE_DESIGN > 0 ? this.currentLetterRenderRadius / LETTER_R_BASE_DESIGN : 1;
    this.visualBgRadius =
      this.letterPositionRadius +
      this.currentLetterRenderRadius +
      WHEEL_BG_PADDING_AROUND_LETTERS_DESIGN * letterSizeRatio;

    this.wheelLetterRenderPos = [];
    if (this.base.length > 0) {
      const angleStep = (Math.PI * 2) / this.base.length;
      for (let i = 0; i < this.base.length; i += 1) {
        const ang = i * angleStep - Math.PI / 2;
        this.wheelLetterRenderPos.push({
          x: this.wheelCenter.x + this.letterPositionRadius * Math.cos(ang),
          y: this.wheelCenter.y + this.letterPositionRadius * Math.sin(ang)
        });
      }
    }

    const scrambleSize = SCRAMBLE_BTN_HEIGHT;
    this.scrambleButton = {
      x: this.wheelCenter.x + SCRAMBLE_BTN_OFFSET_X,
      y: this.wheelCenter.y + SCRAMBLE_BTN_OFFSET_Y - scrambleSize / 2,
      width: scrambleSize,
      height: scrambleSize
    };

    this.continueButton = {
      x: this.wheelCenter.x - 100,
      y: this.wheelCenter.y + this.visualBgRadius + CONTINUE_BTN_OFFSET_Y,
      width: 200,
      height: 50
    };
  }

  private updateHintLayout() {
    this.hintClickableRegions = [];
    const zone = HINT_ZONE_RECT_DESIGN;
    const numHintFrames = HINT_LABELS.length;
    const verticalSpacing = HINT_FRAME_VERTICAL_SPACING;
    const frameImage = this.images.hintFrame;
    const frameTexW = frameImage?.naturalWidth || frameImage?.width || 0;
    const frameTexH = frameImage?.naturalHeight || frameImage?.height || 0;
    const bonusTextHeight = HINT_ZONE_BONUS_TEXT_SIZE;

    let currentY =
      zone.y + HINT_ZONE_BONUS_TEXT_PADDING_TOP + bonusTextHeight + HINT_ZONE_BONUS_TEXT_GAP;
    const availableWidth = Math.max(0, zone.width - HINT_ZONE_SIDE_PADDING * 2);
    const availableHeight = Math.max(
      0,
      zone.y + zone.height - currentY - HINT_ZONE_BOTTOM_PADDING
    );

    if (availableWidth <= 0 || availableHeight <= 0) return;

    let frameW = availableWidth;
    let frameH =
      (availableHeight - (numHintFrames - 1) * verticalSpacing) / Math.max(1, numHintFrames);

    if (frameTexW > 0 && frameTexH > 0) {
      let panelScale = availableWidth / frameTexW;
      let scaledFrameHeight = frameTexH * panelScale;
      let scaledFrameWidth = frameTexW * panelScale;
      const totalRequiredStackHeight =
        numHintFrames * scaledFrameHeight + (numHintFrames - 1) * verticalSpacing;

      if (totalRequiredStackHeight > availableHeight && availableHeight > 0) {
        const newScaledFrameHeight =
          (availableHeight - (numHintFrames - 1) * verticalSpacing) / numHintFrames;
        const clampedFrameHeight = Math.max(newScaledFrameHeight, HINT_FRAME_MIN_HEIGHT);
        panelScale = clampedFrameHeight / frameTexH;
        scaledFrameWidth = frameTexW * panelScale;
        scaledFrameHeight = clampedFrameHeight;
      }

      frameW = scaledFrameWidth;
      frameH = scaledFrameHeight;
    } else {
      frameH = Math.max(frameH, HINT_FRAME_MIN_HEIGHT);
    }

    const frameX = zone.x + (zone.width - frameW) / 2;
    for (let i = 0; i < numHintFrames; i += 1) {
      this.hintClickableRegions.push({
        x: frameX,
        y: currentY,
        width: frameW,
        height: frameH
      });
      currentY += frameH + verticalSpacing;
    }
  }

  private updateTopBarLayout() {
    this.returnToMenuButton = {
      x: TOP_BAR_ZONE_DESIGN.x + TOP_BAR_PADDING_X_DESIGN,
      y:
        TOP_BAR_ZONE_DESIGN.y +
        (TOP_BAR_ZONE_DESIGN.height - RETURN_BTN_HEIGHT_DESIGN) / 2,
      width: RETURN_BTN_WIDTH_DESIGN,
      height: RETURN_BTN_HEIGHT_DESIGN
    };
  }

  private renderMainMenu(ctx: CanvasRenderingContext2D) {
    const title = "Word Puzzle";
    const buttons = [
      { label: "Casual", rect: this.mainMenuButtons[0] },
      { label: "Competitive", rect: this.mainMenuButtons[1], disabled: true },
      { label: "Quit", rect: this.mainMenuButtons[2] }
    ];

    this.drawMenuPanel(ctx, title, buttons);
  }

  private renderCasualMenu(ctx: CanvasRenderingContext2D) {
    const title = "Select Difficulty";
    const buttons = [
      { label: "Easy", rect: this.casualMenuButtons[0] },
      { label: "Medium", rect: this.casualMenuButtons[1] },
      { label: "Hard", rect: this.casualMenuButtons[2] },
      { label: "Return", rect: this.casualMenuButtons[3] }
    ];

    this.drawMenuPanel(ctx, title, buttons);
  }

  private renderSessionComplete(ctx: CanvasRenderingContext2D) {
    // Draw semi-transparent dark overlay (matching PC version)
    ctx.save();
    ctx.fillStyle = colorToCss({ r: 0, g: 0, b: 0, a: 150 });
    ctx.fillRect(0, 0, REF_W, REF_H);
    ctx.restore();

    // Draw Final Score Prominently (matching PC version)
    const finalScoreText = `Final Score: ${this.currentScore}`;
    drawCenteredText(
      ctx,
      finalScoreText,
      { x: REF_W / 2, y: REF_H * 0.3 },
      { r: 255, g: 255, b: 0, a: 255 }, // Yellow color like PC version
      this.font(48, true)
    );

    // Draw Celebration Effects (on top of score/background)
    this.renderCelebrationEffects(ctx);

    // Draw Continue Button (matching PC version positioning)
    const buttonWidth = 200;
    const buttonHeight = 50;
    const buttonX = REF_W / 2 - buttonWidth / 2;
    const buttonY = REF_H * 0.8 - buttonHeight / 2;
    
    this.continueButton = {
      x: buttonX,
      y: buttonY,
      width: buttonWidth,
      height: buttonHeight
    };

    const hover = rectContains(this.continueButton, this.mousePos);
    this.drawButton(
      ctx,
      this.continueButton,
      "Continue",
      hover ? this.currentTheme.menuButtonHover : this.currentTheme.menuButtonNormal,
      UI_WHITE
    );
  }

  private renderGameScreen(ctx: CanvasRenderingContext2D) {
    this.renderTopBar(ctx);
    this.renderScoreZone(ctx);
    this.renderGrid(ctx);
    this.renderPath(ctx);
    this.renderWheel(ctx);
    this.renderLetterAnims(ctx);
    this.renderScoreFlourishes(ctx);
    this.renderHintPointAnims(ctx);
    this.renderGuessDisplay(ctx);
    this.renderHintZone(ctx);
    this.renderSolvedWordPopup(ctx);

    if (this.gameState === GState.Solved || this.currentScreen === GameScreen.GameOver) {
      const title = "Puzzle Solved!";
      const titleFont = this.font(24, true);
      ctx.save();
      ctx.font = titleFont;
      const titleMetrics = ctx.measureText(title);
      const titleHeight =
        (titleMetrics.actualBoundingBoxAscent ?? 0) + (titleMetrics.actualBoundingBoxDescent ?? 0) || 24;
      ctx.restore();

      const buttonWidth = 260;
      const buttonHeight = 75;
      const popupWidth = Math.max(titleMetrics.width, buttonWidth) + 50;
      const popupHeight = titleHeight + buttonHeight + 70;
      const popupX = REF_W / 2 - popupWidth / 2;
      const popupY = REF_H / 2 - popupHeight / 2;

      if (this.images.menuBackground) {
        ctx.drawImage(this.images.menuBackground, popupX, popupY, popupWidth, popupHeight);
      } else {
        drawRoundedRect(
          ctx,
          popupX,
          popupY,
          popupWidth,
          popupHeight,
          15,
          this.currentTheme.solvedOverlayBg,
          this.currentTheme.menuButtonHover,
          1
        );
      }

      drawCenteredText(
        ctx,
        title,
        { x: REF_W / 2, y: popupY + 24 + titleHeight / 2 },
        UI_WHITE,
        titleFont
      );

      this.continueButton = {
        x: REF_W / 2 - buttonWidth / 2,
        y: popupY + titleHeight + 35,
        width: buttonWidth,
        height: buttonHeight
      };

      const hover = rectContains(this.continueButton, this.mousePos);
      this.drawButton(
        ctx,
        this.continueButton,
        "Continue",
        hover ? this.currentTheme.menuButtonHover : this.currentTheme.menuButtonNormal,
        UI_WHITE
      );
    }
  }

  private renderTopBar(ctx: CanvasRenderingContext2D) {
    if (this.currentScreen !== GameScreen.Playing && this.currentScreen !== GameScreen.GameOver) {
      return;
    }

    const hover = rectContains(this.returnToMenuButton, this.mousePos);
    this.drawButton(
      ctx,
      this.returnToMenuButton,
      "Menu",
      hover ? this.currentTheme.menuButtonHover : this.currentTheme.menuButtonNormal
    );
  }

  private renderScoreZone(ctx: CanvasRenderingContext2D) {
    const zone = SCORE_ZONE_RECT_DESIGN;
    const labelPos = { x: zone.x + zone.width / 2, y: zone.y + SCORE_ZONE_PADDING_Y_DESIGN };

    drawCenteredText(
      ctx,
      "SCORE:",
      labelPos,
      UI_ORANGE,
      this.font(SCORE_ZONE_LABEL_FONT_SIZE, true)
    );

    const valueY = labelPos.y + SCORE_LABEL_VALUE_GAP_DESIGN + SCORE_ZONE_LABEL_FONT_SIZE;
    const scaleFactor =
      this.scoreFlourishTimer > 0
        ? 1 +
          SCORE_FLOURISH_SCALE *
            Math.sin((SCORE_FLOURISH_DURATION - this.scoreFlourishTimer) / SCORE_FLOURISH_DURATION * Math.PI)
        : 1;

    ctx.save();
    ctx.translate(zone.x + zone.width / 2, valueY + SCORE_ZONE_VALUE_FONT_SIZE / 2);
    ctx.scale(scaleFactor, scaleFactor);
    ctx.fillStyle = colorToCss(UI_ORANGE);
    ctx.font = this.font(SCORE_ZONE_VALUE_FONT_SIZE, true);
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText(`${this.currentScore}`, 0, 0);
    ctx.restore();

    if (this.isInSession) {
      const padding = 45;
      const meterWidth = zone.width - padding * 2;
      const meterHeight = PROGRESS_METER_HEIGHT_DESIGN;
      const meterX = zone.x + padding;
      const meterY = zone.y + zone.height - meterHeight - 40;
      const progressRatio =
        this.puzzlesPerSession > 0 ? (this.currentPuzzleIndex + 1) / this.puzzlesPerSession : 0;

      drawRoundedRect(ctx, meterX, meterY, meterWidth, meterHeight, 4, { r: 50, g: 50, b: 50, a: 150 }, this.currentTheme.scoreTextLabel, 1);
      ctx.fillStyle = colorToCss(UI_ORANGE);
      ctx.fillRect(meterX, meterY, meterWidth * progressRatio, meterHeight);

      drawCenteredText(
        ctx,
        `${this.currentPuzzleIndex + 1}/${this.puzzlesPerSession}`,
        { x: meterX + meterWidth / 2, y: meterY + meterHeight / 2 },
        UI_ORANGE,
        this.font(12, true)
      );
    }
  }

  private renderGrid(ctx: CanvasRenderingContext2D) {
    if (this.sorted.length === 0) return;
    const tileSize = TILE_SIZE * this.currentGridLayoutScale;
    const tilePad = TILE_PAD * this.currentGridLayoutScale;
    const buttonImage = this.images.gridButton;

    for (let w = 0; w < this.sorted.length; w += 1) {
      for (let c = 0; c < this.sorted[w].text.length; c += 1) {
        const pos = this.tilePos(w, c);
        if (buttonImage) {
          ctx.drawImage(buttonImage, pos.x, pos.y, tileSize, tileSize);
        } else {
          drawRoundedRect(ctx, pos.x, pos.y, tileSize, tileSize, 4, this.currentTheme.gridEmptyTile);
        }

        const isFilled = this.grid[w]?.[c] !== "_";
        if (!isFilled) {
          const rarity = this.sorted[w].rarity;
          const gem = rarity === 2 ? this.images.sapphire : rarity === 3 ? this.images.ruby : rarity === 4 ? this.images.diamond : null;
          if (gem) {
            const gemSize = tileSize * 0.6;
            ctx.drawImage(gem, pos.x + tileSize / 2 - gemSize / 2, pos.y + tileSize / 2 - gemSize / 2, gemSize, gemSize);
          }
          continue;
        }

        const animating = this.letterAnims.some(
          (anim) => anim.target === AnimTarget.Grid && anim.wordIdx === w && anim.charIdx === c && anim.t < 1
        );
        if (animating) continue;

        let scale = 1;
        for (const flourish of this.gridFlourishes) {
          if (flourish.wordIdx === w && flourish.charIdx === c) {
            const progress = (0.6 - flourish.timer) / 0.6;
            scale = 1 + 0.4 * Math.sin(progress * Math.PI);
            break;
          }
        }

        ctx.save();
        ctx.translate(pos.x + tileSize / 2, pos.y + tileSize / 2);
        ctx.scale(scale, scale);
        ctx.fillStyle = colorToCss(UI_ORANGE);
        ctx.font = this.font(Math.max(8, 20 * this.currentGridLayoutScale), true);
        ctx.textAlign = "center";
        ctx.textBaseline = "middle";
        ctx.fillText(this.grid[w][c], 0, 0);
        ctx.restore();
      }
    }

    if (GRID_COLUMN_DIVIDER_WIDTH > 0 && this.colXOffset.length >= 2) {
      const dividerRows = GRID_COLUMN_DIVIDER_ROWS;
      const dividerHeight =
        dividerRows * (tileSize + tilePad) - (dividerRows > 0 ? tilePad : 0);
      for (let c = 0; c < this.colXOffset.length - 1; c += 1) {
        const gapCenterX = this.colXOffset[c + 1] - (COL_PAD * this.currentGridLayoutScale) / 2;
        ctx.fillStyle = "rgba(120,100,80,0.8)";
        ctx.fillRect(gapCenterX - GRID_COLUMN_DIVIDER_WIDTH / 2, this.gridStartY, GRID_COLUMN_DIVIDER_WIDTH, dividerHeight);
      }
    }
  }

  private renderPath(ctx: CanvasRenderingContext2D) {
    if (!this.dragging || this.path.length === 0) return;
    const touchScale = this.wheelTouchScaleActive ? WHEEL_TOUCH_SCALE_FACTOR : 1;
    ctx.strokeStyle = colorToCss(UI_ORANGE);
    ctx.lineWidth = 5 * touchScale;
    ctx.lineCap = "round";
    ctx.beginPath();
    for (let i = 0; i < this.path.length; i += 1) {
      const pos = this.getWheelLetterPosition(this.path[i], touchScale);
      if (!pos) continue;
      if (i === 0) ctx.moveTo(pos.x, pos.y);
      else ctx.lineTo(pos.x, pos.y);
    }
    ctx.lineTo(this.mousePos.x, this.mousePos.y);
    ctx.stroke();
  }

  private renderWheel(ctx: CanvasRenderingContext2D) {
    if (!this.base) return;
    const frame = this.images.circularLetterFrame;
    const touchScale = this.wheelTouchScaleActive ? WHEEL_TOUCH_SCALE_FACTOR : 1;
    const visualRadius = this.currentLetterRenderRadius * WHEEL_LETTER_VISUAL_SCALE * touchScale;
    const fontScaleRatio = clamp(
      (this.currentLetterRenderRadius / LETTER_R_BASE_DESIGN) * WHEEL_LETTER_VISUAL_SCALE,
      0.5,
      2
    );
    const fontSize = Math.max(8, WHEEL_LETTER_FONT_SIZE_BASE_DESIGN * fontScaleRatio * touchScale);

    for (let i = 0; i < this.base.length; i += 1) {
      const pos = this.getWheelLetterPosition(i, touchScale);
      if (!pos) continue;

      if (frame) {
        const diameter = visualRadius * 2;
        ctx.drawImage(frame, pos.x - diameter / 2, pos.y - diameter / 2, diameter, diameter);
      } else {
        ctx.beginPath();
        ctx.arc(pos.x, pos.y, visualRadius, 0, Math.PI * 2);
        ctx.fillStyle = colorToCss(this.currentTheme.letterCircleNormal);
        ctx.fill();
        ctx.strokeStyle = colorToCss(this.currentTheme.wheelOutline);
        ctx.lineWidth = 2 * touchScale;
        ctx.stroke();
      }

      ctx.fillStyle = colorToCss(UI_ORANGE);
      ctx.font = this.font(fontSize, true);
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(this.base[i].toUpperCase(), pos.x, pos.y);
    }

    if (this.images.scrambleButton) {
      ctx.drawImage(
        this.images.scrambleButton,
        this.scrambleButton.x,
        this.scrambleButton.y,
        this.scrambleButton.width,
        this.scrambleButton.height
      );
    }
  }

  private renderLetterAnims(ctx: CanvasRenderingContext2D) {
    const fontSize = Math.max(8, 20 * this.uiScale);
    for (const anim of this.letterAnims) {
      if (anim.t < 0) continue;
      const t = clamp(anim.t, 0, 1);
      const eased = t * t * (3 - 2 * t);
      const x = lerp(anim.start.x, anim.end.x, eased);
      const y = lerp(anim.start.y, anim.end.y, eased);
      const alpha = t > 0.7 ? (1 - t) / 0.3 : 1;
      ctx.fillStyle = `rgba(255, 190, 70, ${alpha})`;
      ctx.font = this.font(fontSize, true);
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(anim.ch, x, y);
    }
  }

  private renderScoreFlourishes(ctx: CanvasRenderingContext2D) {
    const fontSize = Math.max(8, SCORE_FLOURISH_FONT_SIZE_BASE_DESIGN * this.uiScale);
    for (const p of this.scoreFlourishes) {
      ctx.fillStyle = colorToCss(p.color);
      ctx.font = this.font(fontSize, true);
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(p.textString, p.position.x, p.position.y);
    }
  }

  private renderHintPointAnims(ctx: CanvasRenderingContext2D) {
    const fontSize = 20;
    for (const p of this.hintPointAnims) {
      ctx.fillStyle = colorToCss(p.color);
      ctx.font = this.font(fontSize, true);
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(p.textString, p.currentPosition.x, p.currentPosition.y);
    }
  }

  private renderGuessDisplay(ctx: CanvasRenderingContext2D) {
    if (!this.currentGuess) return;
    const n = this.currentGuess.length;
    const guessTileSize = TILE_SIZE * this.currentGridLayoutScale * 1.25;
    const guessPad = TILE_PAD * this.currentGridLayoutScale;
    const totalWidth = n * guessTileSize + (n - 1) * guessPad;
    const touchScale = this.wheelTouchScaleActive ? WHEEL_TOUCH_SCALE_FACTOR : 1;
    const wheelTop = this.wheelCenter.y - this.visualBgRadius * touchScale;
    const guessRowTop = wheelTop - guessTileSize - GUESS_DISPLAY_GAP - GUESS_DISPLAY_OFFSET_Y;
    const startX = this.wheelCenter.x - totalWidth / 2;
    const buttonImage = this.images.gridButton;

    for (let i = 0; i < n; i += 1) {
      const x = startX + i * (guessTileSize + guessPad);
      if (buttonImage) {
        ctx.drawImage(buttonImage, x, guessRowTop, guessTileSize, guessTileSize);
      } else {
        drawRoundedRect(ctx, x, guessRowTop, guessTileSize, guessTileSize, 4, this.currentTheme.gridEmptyTile);
      }
      ctx.fillStyle = colorToCss(UI_ORANGE);
      ctx.font = this.font(Math.max(8, guessTileSize * 0.65), true);
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(this.currentGuess[i], x + guessTileSize / 2, guessRowTop + guessTileSize / 2);
    }
  }

  private renderHintZone(ctx: CanvasRenderingContext2D) {
    const zone = HINT_ZONE_RECT_DESIGN;
    const totalBonus = this.calculateTotalPossibleBonusWords();
    const bonusText = `Bonus Words: ${this.foundBonusWords.size}/${totalBonus}`;
    ctx.font = this.font(HINT_ZONE_BONUS_TEXT_SIZE, true);
    ctx.fillStyle = colorToCss(UI_ORANGE);
    ctx.textAlign = "left";
    ctx.textBaseline = "top";
    const textWidth = ctx.measureText(bonusText).width;
    const textMetrics = ctx.measureText(bonusText);
    const textHeight =
      (textMetrics.actualBoundingBoxAscent ?? 0) +
        (textMetrics.actualBoundingBoxDescent ?? 0) ||
      HINT_ZONE_BONUS_TEXT_SIZE;
    const bonusX = zone.x + (zone.width - textWidth) / 2;
    const bonusY = zone.y + HINT_ZONE_BONUS_TEXT_PADDING_TOP;
    ctx.fillText(bonusText, bonusX, bonusY);
    this.bonusWordsTextRect = { x: bonusX, y: bonusY, width: textWidth, height: textHeight };

    for (let i = 0; i < this.hintClickableRegions.length; i += 1) {
      const rect = this.hintClickableRegions[i];
      const hover = this.hoveredHintIndex === i;
      const baseColor = hover ? this.currentTheme.menuButtonHover : this.currentTheme.menuButtonNormal;
      const frameImage = this.images.hintFrame;
      const frameTexW = frameImage?.naturalWidth || frameImage?.width || rect.width;
      const frameTexH = frameImage?.naturalHeight || frameImage?.height || rect.height;
      const panelScale = frameTexW > 0 ? rect.width / frameTexW : 1;
      if (this.images.hintFrame) {
        ctx.drawImage(this.images.hintFrame, rect.x, rect.y, rect.width, rect.height);
        if (this.hintFrameClickAnimTimers[i] > 0) {
          ctx.fillStyle = "rgba(0,255,0,0.2)";
          ctx.fillRect(rect.x, rect.y, rect.width, rect.height);
        }
      } else {
        drawRoundedRect(ctx, rect.x, rect.y, rect.width, rect.height, 6, baseColor);
      }

      const label = HINT_LABELS[i];
      const cost = HINT_COSTS[i];
      const labelFontSize = Math.max(8, HINT_LABEL_FONT_BASE * panelScale);
      ctx.fillStyle = colorToCss(UI_ORANGE);
      ctx.font = this.font(labelFontSize, true);
      ctx.textAlign = "left";
      ctx.textBaseline = "middle";
      const labelX = rect.x + frameTexW * HINT_LABEL_X_REL * panelScale;
      const labelY = rect.y + frameTexH * HINT_LABEL_Y_REL * panelScale;
      ctx.fillText(label, labelX, labelY);

      if (this.images.hintIndicator) {
        const lightSize = frameTexH * HINT_LIGHT_DIAMETER_REL * panelScale;
        const lightX = rect.x + frameTexW * HINT_LIGHT_CENTER_X_REL * panelScale;
        const lightY = rect.y + frameTexH * HINT_LIGHT_CENTER_Y_REL * panelScale;
        ctx.globalAlpha = this.hintPoints >= cost ? 1 : 0.3;
        ctx.drawImage(
          this.images.hintIndicator,
          lightX - lightSize / 2,
          lightY - lightSize / 2,
          lightSize,
          lightSize
        );
        ctx.globalAlpha = 1;
      }
    }

    if (this.hoveredHintIndex >= 0) {
      const rect = this.hintClickableRegions[this.hoveredHintIndex];
      const popupX = rect.x + rect.width + 10;
      const popupY = rect.y;
      if (this.images.menuBackground) {
        ctx.drawImage(
          this.images.menuBackground,
          popupX,
          popupY,
          HINT_POPUP_WIDTH_DESIGN,
          HINT_POPUP_HEIGHT_DESIGN
        );
      } else {
        drawRoundedRect(
          ctx,
          popupX,
          popupY,
          HINT_POPUP_WIDTH_DESIGN,
          HINT_POPUP_HEIGHT_DESIGN,
          8,
          this.currentTheme.menuBg,
          this.currentTheme.menuButtonHover,
          1
        );
      }

      const desc = HINT_DESCRIPTIONS[this.hoveredHintIndex];
      const padding = HINT_POPUP_PADDING_DESIGN;
      const contentWidth = HINT_POPUP_WIDTH_DESIGN - padding * 2;
      const contentHeight = HINT_POPUP_HEIGHT_DESIGN - padding * 2;
      const baseHeaderFont = 14;
      const baseDescFont = 12;
      const baseSpacing = HINT_POPUP_LINE_SPACING_DESIGN;

      const buildLayout = (scale: number) => {
        const headerFont = Math.max(8, baseHeaderFont * scale);
        const descFont = Math.max(8, baseDescFont * scale);
        const spacing = baseSpacing * scale;
        ctx.font = this.font(descFont);
        const wrapped = wrapTextForWidth(ctx, desc, contentWidth);
        const descLines = wrapped.split("\n").filter((line) => line.length > 0);
        const headerLines = ["Available Points:", `${this.hintPoints}`, `Cost: ${HINT_COSTS[this.hoveredHintIndex]}`];
        const headerHeight = headerLines.length * headerFont + Math.max(0, headerLines.length - 1) * spacing;
        const descHeight = descLines.length * descFont + Math.max(0, descLines.length - 1) * spacing;
        const totalHeight = headerHeight + (descLines.length > 0 ? spacing + descHeight : 0);
        return {
          headerFont,
          descFont,
          spacing,
          descLines,
          headerLines,
          headerHeight,
          totalHeight
        };
      };

      let layout = buildLayout(1);
      if (layout.totalHeight > contentHeight) {
        const scale = Math.max(0.6, contentHeight / layout.totalHeight);
        layout = buildLayout(scale);
      }
      if (layout.totalHeight > contentHeight && layout.descLines.length > 0) {
        const availableDescHeight = contentHeight - (layout.headerHeight + layout.spacing);
        const lineHeight = layout.descFont + layout.spacing;
        const maxLines = Math.max(1, Math.floor((availableDescHeight + layout.spacing) / lineHeight));
        layout.descLines = layout.descLines.slice(0, maxLines);
      }

      ctx.fillStyle = colorToCss(UI_WHITE);
      ctx.textAlign = "left";
      ctx.textBaseline = "top";

      let textY = popupY + padding;
      ctx.font = this.font(layout.headerFont, true);
      for (let i = 0; i < layout.headerLines.length; i += 1) {
        ctx.fillText(layout.headerLines[i], popupX + padding, textY);
        textY += layout.headerFont;
        if (i < layout.headerLines.length - 1) {
          textY += layout.spacing;
        }
      }
      if (layout.descLines.length > 0) {
        textY += layout.spacing;
        ctx.font = this.font(layout.descFont);
        for (let i = 0; i < layout.descLines.length; i += 1) {
          ctx.fillText(layout.descLines[i], popupX + padding, textY);
          textY += layout.descFont;
          if (i < layout.descLines.length - 1) {
            textY += layout.spacing;
          }
        }
      }
    }
  }

  private renderBonusWordsPopup(ctx: CanvasRenderingContext2D) {
    const zone = GRID_ZONE_RECT_DESIGN;
    const popupWidth = zone.width * POPUP_MAX_WIDTH_DESIGN_RATIO;
    const popupHeight = zone.height * POPUP_MAX_HEIGHT_DESIGN_RATIO;
    const popupX = zone.x + (zone.width - popupWidth) / 2;
    const popupY = zone.y + (zone.height - popupHeight) / 2;

    if (this.images.menuBackground) {
      ctx.drawImage(this.images.menuBackground, popupX, popupY, popupWidth, popupHeight);
    } else {
      drawRoundedRect(
        ctx,
        popupX,
        popupY,
        popupWidth,
        popupHeight,
        POPUP_CORNER_RADIUS_BASE,
        this.currentTheme.menuBg,
        this.currentTheme.menuButtonHover,
        1
      );
    }

    const groups = this.buildBonusWordGroups();
    if (groups.length === 0) return;

    type ColumnWord = { text: string; color: Color };
    type GroupLayout = {
      title: string;
      totalWidth: number;
      totalHeight: number;
      wordsHeight: number;
      wordColumnWidth: number;
      columns: ColumnWord[][];
    };

    const maxContentWidth = popupWidth - POPUP_PADDING_BASE * 2;
    const maxContentHeight = popupHeight - POPUP_PADDING_BASE * 2;

    const buildLayout = (scale: number) => {
      const wordFontSize = Math.max(5, POPUP_WORD_FONT_SIZE_BASE * scale);
      const titleFontSize = Math.max(7, POPUP_TITLE_FONT_SIZE_BASE * scale);
      const majorSpacing = MAJOR_COL_SPACING_BASE * scale;
      const minorSpacing = MINOR_COL_SPACING_BASE * scale;
      const titleBottomMargin = TITLE_BOTTOM_MARGIN_BASE * scale;
      const wordLineSpacing = WORD_LINE_SPACING_BASE * scale;

      const layouts: GroupLayout[] = groups.map((group) => {
        ctx.font = this.font(titleFontSize, true);
        const titleWidth = ctx.measureText(group.title).width;
        const titleHeight = titleFontSize;

        ctx.font = this.font(wordFontSize);
        let maxWordWidth = 0;
        const wordItems = group.words.map((word) => {
          const display = word.found ? word.text.toUpperCase() : "*".repeat(word.text.length);
          maxWordWidth = Math.max(maxWordWidth, ctx.measureText(display).width);
          return {
            text: display,
            color: word.found ? this.currentTheme.hudTextFound : this.currentTheme.menuButtonText
          };
        });

        let numMinorCols = 1;
        if (wordItems.length >= 15) numMinorCols = MAX_MINOR_COLS_PER_GROUP;
        else if (wordItems.length >= 6) numMinorCols = Math.min(2, MAX_MINOR_COLS_PER_GROUP);

        const columns: ColumnWord[][] = Array.from({ length: numMinorCols }, () => []);
        const baseCount = Math.floor(wordItems.length / numMinorCols);
        const remainder = wordItems.length % numMinorCols;
        let idx = 0;
        for (let col = 0; col < numMinorCols; col += 1) {
          const countInCol = baseCount + (col < remainder ? 1 : 0);
          for (let k = 0; k < countInCol; k += 1) {
            if (idx >= wordItems.length) break;
            columns[col].push(wordItems[idx]);
            idx += 1;
          }
        }

        const columnHeights = columns.map((col) => {
          if (col.length === 0) return 0;
          return col.length * wordFontSize + (col.length - 1) * wordLineSpacing;
        });
        const wordsHeight = columnHeights.length ? Math.max(...columnHeights) : 0;
        const columnsWidth =
          numMinorCols * maxWordWidth + (numMinorCols - 1) * minorSpacing;
        const totalWidth = Math.max(titleWidth, columnsWidth);
        const totalHeight = titleHeight + titleBottomMargin + wordsHeight;

        return {
          title: group.title,
          totalWidth,
          totalHeight,
          wordsHeight,
          wordColumnWidth: Math.max(maxWordWidth, 1),
          columns
        };
      });

      const totalWidth =
        layouts.reduce((sum, group) => sum + group.totalWidth, 0) +
        Math.max(0, layouts.length - 1) * majorSpacing;
      const maxGroupHeight = layouts.reduce((max, group) => Math.max(max, group.totalHeight), 0);

      return {
        layouts,
        totalWidth,
        maxGroupHeight,
        wordFontSize,
        titleFontSize,
        majorSpacing,
        minorSpacing,
        titleBottomMargin,
        wordLineSpacing
      };
    };

    let layout = buildLayout(1);
    let textScale = 1;
    if (layout.totalWidth > 0 && maxContentWidth > 0) {
      textScale = Math.min(textScale, maxContentWidth / layout.totalWidth);
    }
    if (layout.maxGroupHeight > 0 && maxContentHeight > 0) {
      textScale = Math.min(textScale, maxContentHeight / layout.maxGroupHeight);
    }
    textScale = clamp(textScale, POPUP_MIN_TEXT_SCALE, POPUP_MAX_TEXT_SCALE);
    if (textScale !== 1) {
      layout = buildLayout(textScale);
    }

    const contentStartX = popupX + (popupWidth - layout.totalWidth) / 2;
    const headerY = popupY + POPUP_PADDING_BASE;

    ctx.textAlign = "center";
    ctx.textBaseline = "top";
    ctx.font = this.font(layout.titleFontSize, true);
    ctx.fillStyle = colorToCss(UI_WHITE);

    let currentMajorX = contentStartX;
    for (const group of layout.layouts) {
      const titleCenterX = currentMajorX + group.totalWidth / 2;
      ctx.fillText(group.title, titleCenterX, headerY);
      currentMajorX += group.totalWidth + layout.majorSpacing;
    }

    const headerRowHeight = layout.titleFontSize + layout.titleBottomMargin;
    const topBuffer = POPUP_PADDING_BASE + headerRowHeight;
    const scrollViewportHeight = Math.max(
      1,
      popupHeight - topBuffer - POPUP_SCROLL_BOTTOM_BUFFER
    );
    const wordContentHeight = layout.layouts.reduce(
      (max, group) => Math.max(max, group.wordsHeight),
      0
    );

    this.bonusWordsPopupMaxScrollOffset = Math.max(0, wordContentHeight - scrollViewportHeight);
    this.bonusWordsPopupScrollOffset = clamp(
      this.bonusWordsPopupScrollOffset,
      0,
      this.bonusWordsPopupMaxScrollOffset
    );

    const scrollY = popupY + topBuffer - this.bonusWordsPopupScrollOffset;
    ctx.save();
    ctx.beginPath();
    ctx.rect(popupX, popupY + topBuffer, popupWidth, scrollViewportHeight);
    ctx.clip();

    ctx.font = this.font(layout.wordFontSize);
    ctx.textAlign = "center";
    ctx.textBaseline = "top";

    currentMajorX = contentStartX;
    for (const group of layout.layouts) {
      let currentMinorX = currentMajorX;
      for (const column of group.columns) {
        let y = scrollY;
        for (let i = 0; i < column.length; i += 1) {
          ctx.fillStyle = colorToCss(UI_WHITE);
          const centerX = currentMinorX + group.wordColumnWidth / 2;
          ctx.fillText(column[i].text, centerX, y);
          y += layout.wordFontSize;
          if (i < column.length - 1) {
            y += layout.wordLineSpacing;
          }
        }
        currentMinorX += group.wordColumnWidth + layout.minorSpacing;
      }
      currentMajorX += group.totalWidth + layout.majorSpacing;
    }

    ctx.restore();
  }

  private renderSolvedWordPopup(ctx: CanvasRenderingContext2D) {
    if (this.hoveredSolvedWordIndex < 0) return;
    const info = this.sorted[this.hoveredSolvedWordIndex];
    if (!info) return;

    const popupPadding = WORD_INFO_POPUP_PADDING_DESIGN;
    const popupMaxWidth = WORD_INFO_POPUP_MAX_WIDTH_DESIGN;
    const lineSpacing = WORD_INFO_POPUP_LINE_SPACING_DESIGN;
    const contentWidthLimit = Math.max(120, popupMaxWidth - popupPadding * 2);
    const titleFontSize = Math.max(10, 16);
    const bodyFontSize = Math.max(8, 14);

    const blocks = [
      {
        text: `Word: ${info.text}`,
        fontSize: titleFontSize,
        bold: true,
        color: { r: 230, g: 230, b: 230, a: 255 }
      },
      {
        text: `POS: ${info.pos ? info.pos : "N/A"}`,
        fontSize: bodyFontSize,
        bold: false,
        color: { r: 200, g: 210, b: 220, a: 255 }
      },
      {
        text: `Definition: ${info.definition ? info.definition : "N/A"}`,
        fontSize: bodyFontSize,
        bold: false,
        color: { r: 190, g: 200, b: 210, a: 255 }
      },
      {
        text: `Sentence: ${info.sentence ? info.sentence : "N/A"}`,
        fontSize: bodyFontSize,
        bold: false,
        color: { r: 190, g: 200, b: 210, a: 255 }
      }
    ];

    const measured = blocks.map((block) => {
      ctx.font = this.font(block.fontSize, block.bold);
      const wrapped = wrapTextForWidth(ctx, block.text, contentWidthLimit);
      const lines = wrapped.split("\n");
      const width = lines.reduce((max, line) => Math.max(max, ctx.measureText(line).width), 0);
      const height = lines.length * block.fontSize + Math.max(0, lines.length - 1) * lineSpacing;
      return { ...block, lines, width, height };
    });

    const maxTextWidth = measured.reduce((max, block) => Math.max(max, block.width), 0);
    const popupWidth = Math.min(popupMaxWidth, maxTextWidth + popupPadding * 2);
    const popupHeight =
      measured.reduce((sum, block) => sum + block.height, 0) +
      lineSpacing * Math.max(0, measured.length - 1) +
      popupPadding * 2;

    const popupOffset = WORD_INFO_POPUP_OFFSET_FROM_MOUSE_DESIGN;
    const popupMargin = POPUP_SCREEN_MARGIN_DESIGN;
    let popupX = this.mousePos.x + popupOffset;
    let popupY = this.mousePos.y + popupOffset;

    if (popupX + popupWidth > REF_W - popupMargin) {
      popupX = REF_W - popupWidth - popupMargin;
    }
    if (popupY + popupHeight > REF_H - popupMargin) {
      popupY = REF_H - popupHeight - popupMargin;
    }
    popupX = Math.max(popupX, popupMargin);
    popupY = Math.max(popupY, popupMargin);

    ctx.save();
    if (this.images.menuBackground) {
      ctx.drawImage(this.images.menuBackground, popupX, popupY, popupWidth, popupHeight);
    } else {
      drawRoundedRect(
        ctx,
        popupX,
        popupY,
        popupWidth,
        popupHeight,
        8,
        this.currentTheme.menuBg,
        this.currentTheme.menuButtonHover,
        1
      );
    }

    let textY = popupY + popupPadding;
    const textX = popupX + popupPadding;
    ctx.textAlign = "left";
    ctx.textBaseline = "top";

    for (let i = 0; i < measured.length; i += 1) {
      const block = measured[i];
      ctx.font = this.font(block.fontSize, block.bold);
      ctx.fillStyle = colorToCss(UI_WHITE);
      let lineY = textY;
      for (const line of block.lines) {
        ctx.fillText(line, textX, lineY);
        lineY += block.fontSize + lineSpacing;
      }
      textY += block.height + (i < measured.length - 1 ? lineSpacing : 0);
    }

    ctx.restore();
  }

  private buildBonusWordLines() {
    if (!this.bonusWordsCacheIsValid) {
      const unique = new Map<string, WordInfo>();
      for (const info of this.allPotentialSolutions) {
        if (!this.isGridSolution(info.text)) {
          unique.set(info.text, info);
        }
      }
      this.cachedBonusWords = Array.from(unique.values()).sort((a, b) => {
        if (a.text.length !== b.text.length) return a.text.length - b.text.length;
        return a.text.localeCompare(b.text);
      });
      this.bonusWordsCacheIsValid = true;
    }

    const lines: { text: string; found: boolean; isHeader: boolean }[] = [];
    let currentLength = 0;
    for (const word of this.cachedBonusWords) {
      if (word.text.length !== currentLength) {
        currentLength = word.text.length;
        lines.push({ text: `${currentLength}-Letter Words:`, found: true, isHeader: true });
      }
      const found = this.foundBonusWords.has(word.text);
      lines.push({ text: found ? word.text.toUpperCase() : "*".repeat(word.text.length), found, isHeader: false });
    }
    return lines;
  }

  private buildBonusWordGroups() {
    if (!this.bonusWordsCacheIsValid) {
      const unique = new Map<string, WordInfo>();
      for (const info of this.allPotentialSolutions) {
        if (!this.isGridSolution(info.text)) {
          unique.set(info.text, info);
        }
      }
      this.cachedBonusWords = Array.from(unique.values()).sort((a, b) => {
        if (a.text.length !== b.text.length) return a.text.length - b.text.length;
        return a.text.localeCompare(b.text);
      });
      this.bonusWordsCacheIsValid = true;
    }

    const groups = new Map<number, { text: string; found: boolean }[]>();
    for (const word of this.cachedBonusWords) {
      const found = this.foundBonusWords.has(word.text);
      const existing = groups.get(word.text.length);
      if (existing) {
        existing.push({ text: word.text, found });
      } else {
        groups.set(word.text.length, [{ text: word.text, found }]);
      }
    }

    return Array.from(groups.entries())
      .sort(([a], [b]) => a - b)
      .map(([length, words]) => ({
        length,
        title: `${length}-Letter Words:`,
        words
      }));
  }

  private drawMenuPanel(
    ctx: CanvasRenderingContext2D,
    title: string,
    buttons: { label: string; rect: Rect; disabled?: boolean }[]
  ) {
    if (buttons.length === 0) return;
    const panelX = buttons[0].rect.x - MENU_PANEL_PADDING_DESIGN;
    const panelY = buttons[0].rect.y - MENU_PANEL_PADDING_DESIGN - 40;
    const panelWidth = MENU_BUTTON_WIDTH_DESIGN + MENU_PANEL_PADDING_DESIGN * 2;
    const panelHeight =
      MENU_PANEL_PADDING_DESIGN * 2 +
      40 +
      buttons.length * MENU_BUTTON_HEIGHT_DESIGN +
      (buttons.length - 1) * MENU_BUTTON_SPACING_DESIGN;

    if (this.images.menuBackground) {
      ctx.drawImage(this.images.menuBackground, panelX, panelY, panelWidth, panelHeight);
    } else {
      drawRoundedRect(ctx, panelX, panelY, panelWidth, panelHeight, 12, this.currentTheme.menuBg);
    }

    drawCenteredText(
      ctx,
      title,
      { x: panelX + panelWidth / 2, y: panelY + 24 },
      this.currentTheme.menuTitleText,
      this.font(24, true)
    );

    for (const button of buttons) {
      const hover = !button.disabled && rectContains(button.rect, this.mousePos);
      if (button.disabled) {
        ctx.save();
        ctx.globalAlpha = 0.45;
      }
      this.drawButton(
        ctx,
        button.rect,
        button.label,
        hover ? this.currentTheme.menuButtonHover : this.currentTheme.menuButtonNormal
      );
      if (button.disabled) {
        ctx.restore();
      }
    }
  }

  private drawButton(
    ctx: CanvasRenderingContext2D,
    rect: Rect,
    label: string,
    color: Color,
    textColor: Color = this.currentTheme.menuButtonText
  ) {
    if (this.images.menuButton) {
      ctx.drawImage(this.images.menuButton, rect.x, rect.y, rect.width, rect.height);
    } else {
      drawRoundedRect(ctx, rect.x, rect.y, rect.width, rect.height, 8, color);
    }

    drawCenteredText(
      ctx,
      label,
      { x: rect.x + rect.width / 2, y: rect.y + rect.height / 2 },
      textColor,
      this.font(18, true)
    );
  }

  private updateLetterAnims(dt: number) {
    this.letterAnims = this.letterAnims.filter((anim) => {
      anim.t += dt * 3;
      if (anim.t >= 1) {
        anim.t = 1;
        if (anim.target === AnimTarget.Grid) {
          const gridRow = this.grid[anim.wordIdx];
          if (gridRow && anim.charIdx >= 0 && anim.charIdx < gridRow.length) {
            gridRow[anim.charIdx] = anim.ch;
            this.checkWordCompletion(anim.wordIdx);
          }
          this.playSound("place");
        }
        return false;
      }
      return true;
    });
  }

  private updateScoreFlourishes(dt: number) {
    this.scoreFlourishes = this.scoreFlourishes.filter((p) => {
      p.position = { x: p.position.x + p.velocity.x * dt, y: p.position.y + p.velocity.y * dt };
      p.lifetime -= dt;
      if (p.lifetime <= 0) return false;
      const alphaRatio = clamp(p.lifetime / p.initialLifetime, 0, 1);
      p.color = { ...p.color, a: alphaRatio * 255 };
      return true;
    });
  }

  private updateHintPointAnims(dt: number) {
    this.hintPointAnims = this.hintPointAnims.filter((p) => {
      p.t += dt * p.speed;
      const t = clamp(p.t, 0, 1);
      p.currentPosition = {
        x: lerp(p.startPosition.x, p.targetPosition.x, t),
        y: lerp(p.startPosition.y, p.targetPosition.y, t)
      };
      return p.t < 1;
    });
  }

  private spawnScoreFlourish(points: number, wordIdx: number) {
    if (points === 0) return;
    const word = this.sorted[wordIdx]?.text;
    if (!word) return;

    const firstPos = this.tilePos(wordIdx, 0);
    const lastPos = this.tilePos(wordIdx, word.length - 1);
    const tileSize =
      TILE_SIZE * this.currentGridLayoutScale * (this.currentGridLayoutScale < 1 ? GRID_TILE_RELATIVE_SCALE_WHEN_SHRUNK : 1);
    const center = {
      x: (firstPos.x + lastPos.x) / 2 + tileSize / 2,
      y: firstPos.y - tileSize * 0.5
    };

    const lifetime = randRange(SCORE_FLOURISH_LIFETIME_MIN_SEC, SCORE_FLOURISH_LIFETIME_MAX_SEC);
    this.scoreFlourishes.push({
      textString: `+${points}`,
      position: center,
      velocity: {
        x: randRange(-SCORE_FLOURISH_VEL_X_RANGE_DESIGN, SCORE_FLOURISH_VEL_X_RANGE_DESIGN),
        y: randRange(SCORE_FLOURISH_VEL_Y_MAX_DESIGN, SCORE_FLOURISH_VEL_Y_MIN_DESIGN)
      },
      lifetime,
      initialLifetime: lifetime,
      color: { r: 255, g: 215, b: 0, a: 255 }
    });
  }

  private spawnHintPointAnimation(targetRect: Rect, pointsAwarded: number) {
    const start = {
      x: targetRect.x + targetRect.width / 2,
      y: targetRect.y
    };
    const target = {
      x: targetRect.x + targetRect.width / 2,
      y: targetRect.y - 24
    };
    this.hintPointAnims.push({
      textString: `+${pointsAwarded}`,
      currentPosition: start,
      startPosition: start,
      targetPosition: target,
      color: { r: 255, g: 255, b: 0, a: 255 },
      t: 0,
      speed: 0.8
    });
  }

  private activateHint(type: HintType) {
    if (this.gameState === GState.Solved) {
      this.playSound("error");
      return;
    }

    let anyBlanksLeft = false;
    for (let w = 0; w < this.grid.length; w += 1) {
      if (this.found.has(this.sorted[w].text)) continue;
      if (this.grid[w].some((ch) => ch === "_")) {
        anyBlanksLeft = true;
        break;
      }
    }
    if (!anyBlanksLeft) {
      this.playSound("error");
      return;
    }

    const lettersToReveal: Array<{ wordIdx: number; charIdx: number; letter: string }> = [];

    if (type === HintType.RevealFirst) {
      for (let w = 0; w < this.grid.length; w += 1) {
        if (this.found.has(this.sorted[w].text)) continue;
        for (let c = 0; c < this.grid[w].length; c += 1) {
          if (this.grid[w][c] === "_") {
            lettersToReveal.push({ wordIdx: w, charIdx: c, letter: this.sorted[w].text[c] });
            break;
          }
        }
        if (lettersToReveal.length) break;
      }
    } else if (type === HintType.RevealRandom) {
      for (let w = 0; w < this.grid.length; w += 1) {
        if (this.found.has(this.sorted[w].text)) continue;
        const blanks = this.grid[w]
          .map((ch, idx) => (ch === "_" ? idx : -1))
          .filter((idx) => idx >= 0);
        if (blanks.length > 0) {
          const idx = blanks[Math.floor(Math.random() * blanks.length)];
          lettersToReveal.push({ wordIdx: w, charIdx: idx, letter: this.sorted[w].text[idx] });
        }
      }
    } else if (type === HintType.RevealLast) {
      for (let w = this.grid.length - 1; w >= 0; w -= 1) {
        if (this.found.has(this.sorted[w].text)) continue;
        for (let c = 0; c < this.grid[w].length; c += 1) {
          if (this.grid[w][c] === "_") {
            lettersToReveal.push({ wordIdx: w, charIdx: c, letter: this.sorted[w].text[c] });
          }
        }
        if (lettersToReveal.length) break;
      }
    } else if (type === HintType.RevealFirstOfEach) {
      for (let w = 0; w < this.grid.length; w += 1) {
        if (this.found.has(this.sorted[w].text)) continue;
        const idx = this.grid[w].findIndex((ch) => ch === "_");
        if (idx >= 0) {
          lettersToReveal.push({ wordIdx: w, charIdx: idx, letter: this.sorted[w].text[idx] });
        }
      }
    }

    if (lettersToReveal.length === 0) {
      this.playSound("error");
      return;
    }

    this.playSound("hintUsed");
    let delay = 0;
    for (const reveal of lettersToReveal) {
      const endPos = this.tilePos(reveal.wordIdx, reveal.charIdx);
      const tileSize =
        TILE_SIZE * this.currentGridLayoutScale * (this.currentGridLayoutScale < 1 ? GRID_TILE_RELATIVE_SCALE_WHEN_SHRUNK : 1);
      let startPos = this.wheelCenter;
      for (let i = 0; i < this.base.length; i += 1) {
        if (this.base[i].toUpperCase() === reveal.letter.toUpperCase()) {
          startPos = this.wheelLetterRenderPos[i] ?? startPos;
          break;
        }
      }

      this.letterAnims.push({
        ch: reveal.letter.toUpperCase(),
        start: startPos,
        end: { x: endPos.x + tileSize / 2, y: endPos.y + tileSize / 2 },
        t: delay,
        wordIdx: reveal.wordIdx,
        charIdx: reveal.charIdx,
        target: AnimTarget.Grid
      });
      delay -= 0.03;
    }
  }

  private checkWordCompletion(wordIdx: number) {
    if (!this.grid[wordIdx] || !this.sorted[wordIdx]) return;
    const solution = this.sorted[wordIdx].text;
    if (this.found.has(solution)) return;

    if (this.grid[wordIdx].some((ch) => ch === "_")) return;

    const gridWord = this.grid[wordIdx].join("").toUpperCase();
    if (gridWord === solution.toUpperCase()) {
      this.found.add(solution);
      const baseScore = solution.length * 10;
      const rarityBonus = this.sorted[wordIdx].rarity > 1 ? this.sorted[wordIdx].rarity * 25 : 0;
      const wordScore = baseScore + rarityBonus;
      this.currentScore += wordScore;
      this.spawnScoreFlourish(wordScore, wordIdx);
      this.scoreFlourishTimer = SCORE_FLOURISH_DURATION;

      if (this.found.size === this.solutions.length) {
        this.gameState = GState.Solved;
        this.currentScreen = GameScreen.GameOver;
        this.playSound("win");
      }
    }
  }

  private tilePos(wordIdx: number, charIdx: number): Vec2 {
    const tileSize = TILE_SIZE * this.currentGridLayoutScale;
    const tilePad = TILE_PAD * this.currentGridLayoutScale;
    const col = this.wordCol[wordIdx] ?? 0;
    const row = this.wordRow[wordIdx] ?? 0;
    const x = (this.colXOffset[col] ?? this.gridStartX) + charIdx * (tileSize + tilePad);
    const y = this.gridStartY + row * (tileSize + tilePad);
    return { x, y };
  }

  private calculateTotalPossibleBonusWords() {
    return this.allPotentialSolutions.filter((info) => !this.isGridSolution(info.text)).length;
  }

  private isGridSolution(wordText: string) {
    return this.solutions.some((info) => info.text === wordText);
  }

  private startCelebrationEffects() {
    this.celebrationTimer = 0;
    this.confetti = [];
    for (let i = 0; i < 100; i += 1) {
      this.confetti.push({
        position: { x: randRange(0, REF_W), y: randRange(0, REF_H) },
        velocity: { x: randRange(-30, 30), y: randRange(20, 80) },
        angularVelocity: randRange(-3, 3),
        lifetime: randRange(2, 4),
        initialLifetime: randRange(2, 4),
        rotation: randRange(0, Math.PI * 2),
        size: { x: randRange(4, 8), y: randRange(6, 12) },
        color: {
          r: randRange(50, 255, true),
          g: randRange(50, 255, true),
          b: randRange(50, 255, true),
          a: 255
        }
      });
    }
  }

  private updateCelebrationEffects(dt: number) {
    if (this.currentScreen !== GameScreen.SessionComplete) return;
    this.celebrationTimer += dt;
    this.confetti = this.confetti.filter((p) => {
      p.position = { x: p.position.x + p.velocity.x * dt, y: p.position.y + p.velocity.y * dt };
      p.rotation += p.angularVelocity * dt;
      p.lifetime -= dt;
      return p.lifetime > 0;
    });
  }

  private renderCelebrationEffects(ctx: CanvasRenderingContext2D) {
    for (const p of this.confetti) {
      ctx.save();
      ctx.translate(p.position.x, p.position.y);
      ctx.rotate(p.rotation);
      ctx.fillStyle = colorToCss(p.color);
      ctx.fillRect(-p.size.x / 2, -p.size.y / 2, p.size.x, p.size.y);
      ctx.restore();
    }
  }

  private clearDragState() {
    this.dragging = false;
    this.path = [];
    this.currentGuess = "";
    this.wheelTouchScaleActive = false;
  }

  private getCriteriaForCurrentPuzzle() {
    const isLast = this.currentPuzzleIndex === this.puzzlesPerSession - 1;
    if (this.selectedDifficulty === DifficultyLevel.Easy) {
      return { allowedLengths: [7], allowedRarities: [1] };
    }
    if (this.selectedDifficulty === DifficultyLevel.Medium) {
      return { allowedLengths: [7], allowedRarities: [1, 2, 3] };
    }
    if (this.selectedDifficulty === DifficultyLevel.Hard) {
      return { allowedLengths: [7], allowedRarities: isLast ? [4] : [3, 4] };
    }
    return { allowedLengths: [4, 5, 6, 7], allowedRarities: [1, 2, 3, 4] };
  }

  private getCanonicalLetters(word: string) {
    return word.toLowerCase().split("").sort().join("");
  }

  private startBackgroundMusic() {
    if (!this.hasUserInteracted) return;
    if (this.backgroundMusic) return;
    if (Assets.music.length === 0) return;
    const track = Assets.music[Math.floor(Math.random() * Assets.music.length)];
    const audio = new Audio(track);
    audio.loop = true;
    audio.volume = 0.4;
    audio.play().catch(() => undefined);
    this.backgroundMusic = audio;
  }

  private playSound(name: string) {
    const sound = this.sounds[name];
    if (!sound) return;
    sound.currentTime = 0;
    sound.play().catch(() => undefined);
  }

  private drawBackground(ctx: CanvasRenderingContext2D) {
    const bg = this.images.mainBackground;
    if (!bg) return;
    const scale = Math.max(REF_W / bg.width, REF_H / bg.height);
    const width = bg.width * scale;
    const height = bg.height * scale;
    const x = (REF_W - width) / 2;
    const y = (REF_H - height) / 2;
    ctx.drawImage(bg, x, y, width, height);
  }

  private font(size: number, bold = false) {
    return `${bold ? "bold " : ""}${size}px ${this.fontFamily}, Arial, sans-serif`;
  }

  private scale(value: number) {
    return value * this.uiScale * UI_SCALE_MODIFIER;
  }
}

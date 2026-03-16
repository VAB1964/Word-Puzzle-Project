import type { WordInfo } from "../core/types";

export enum Direction {
  Horizontal = "Horizontal",
  Vertical = "Vertical"
}

export interface CrosswordPlacement {
  gridRow: number;
  gridCol: number;
  dir: Direction;
}

export interface CrosswordResult {
  placedWords: WordInfo[];
  placements: CrosswordPlacement[];
  sharedCells: Map<string, [number, number][]>;
  gridRows: number;
  gridCols: number;
}

const cellKey = (r: number, c: number) => `${r},${c}`;

interface PlacedWord {
  text: string;
  startRow: number;
  startCol: number;
  dir: Direction;
}

function canPlace(
  word: string,
  startRow: number,
  startCol: number,
  dir: Direction,
  occupiedCells: Map<string, string>
): { ok: boolean; intersections: number } {
  let intersections = 0;
  const len = word.length;

  for (let i = 0; i < len; i++) {
    const r = dir === Direction.Horizontal ? startRow : startRow + i;
    const c = dir === Direction.Horizontal ? startCol + i : startCol;
    const existing = occupiedCells.get(cellKey(r, c));
    const wch = word[i].toLowerCase();

    if (existing != null) {
      if (existing !== wch) return { ok: false, intersections: 0 };
      intersections++;
    } else {
      if (dir === Direction.Horizontal) {
        if (occupiedCells.has(cellKey(r - 1, c)) || occupiedCells.has(cellKey(r + 1, c)))
          return { ok: false, intersections: 0 };
      } else {
        if (occupiedCells.has(cellKey(r, c - 1)) || occupiedCells.has(cellKey(r, c + 1)))
          return { ok: false, intersections: 0 };
      }
    }
  }

  // Check cell before word start
  const beforeR = dir === Direction.Horizontal ? startRow : startRow - 1;
  const beforeC = dir === Direction.Horizontal ? startCol - 1 : startCol;
  if (occupiedCells.has(cellKey(beforeR, beforeC))) return { ok: false, intersections: 0 };

  // Check cell after word end
  const afterR = dir === Direction.Horizontal ? startRow : startRow + len;
  const afterC = dir === Direction.Horizontal ? startCol + len : startCol;
  if (occupiedCells.has(cellKey(afterR, afterC))) return { ok: false, intersections: 0 };

  return { ok: intersections > 0, intersections };
}

function shuffleArray<T>(arr: T[]): void {
  for (let i = arr.length - 1; i > 0; i--) {
    const j = Math.floor(Math.random() * (i + 1));
    [arr[i], arr[j]] = [arr[j], arr[i]];
  }
}

function generateCrosswordTrial(words: WordInfo[]): CrosswordResult {
  const result: CrosswordResult = {
    placedWords: [],
    placements: [],
    sharedCells: new Map(),
    gridRows: 0,
    gridCols: 0
  };

  if (words.length === 0) return result;

  const sortedWords = [...words].sort((a, b) => b.text.length - a.text.length);
  let gi = 0;
  while (gi < sortedWords.length) {
    let gj = gi;
    while (gj < sortedWords.length && sortedWords[gj].text.length === sortedWords[gi].text.length) gj++;
    const group = sortedWords.slice(gi, gj);
    shuffleArray(group);
    for (let k = 0; k < group.length; k++) sortedWords[gi + k] = group[k];
    gi = gj;
  }

  const placed: PlacedWord[] = [];
  const occupiedCells = new Map<string, string>();

  const firstWord = sortedWords[0].text;
  placed.push({ text: firstWord, startRow: 0, startCol: 0, dir: Direction.Horizontal });
  for (let i = 0; i < firstWord.length; i++) {
    occupiedCells.set(cellKey(0, i), firstWord[i].toLowerCase());
  }

  const isPlaced = new Array(sortedWords.length).fill(false);
  isPlaced[0] = true;

  let curMinRow = 0;
  let curMaxRow = 0;
  let curMinCol = 0;
  let curMaxCol = firstWord.length - 1;

  for (let wi = 1; wi < sortedWords.length; wi++) {
    const candidate = sortedWords[wi].text.toLowerCase();

    let bestScore = -1;
    let bestRow = 0;
    let bestCol = 0;
    let bestDir = Direction.Horizontal;

    for (const pw of placed) {
      const placedLower = pw.text.toLowerCase();
      const newDir = pw.dir === Direction.Horizontal ? Direction.Vertical : Direction.Horizontal;

      for (let ci = 0; ci < candidate.length; ci++) {
        for (let pj = 0; pj < placedLower.length; pj++) {
          if (candidate[ci] !== placedLower[pj]) continue;

          const placedCellR = pw.dir === Direction.Horizontal ? pw.startRow : pw.startRow + pj;
          const placedCellC = pw.dir === Direction.Horizontal ? pw.startCol + pj : pw.startCol;

          let startRow: number;
          let startCol: number;
          if (newDir === Direction.Horizontal) {
            startRow = placedCellR;
            startCol = placedCellC - ci;
          } else {
            startRow = placedCellR - ci;
            startCol = placedCellC;
          }

          const { ok, intersections } = canPlace(candidate, startRow, startCol, newDir, occupiedCells);
          if (ok) {
            const candLen = candidate.length;
            const endR = newDir === Direction.Horizontal ? startRow : startRow + candLen - 1;
            const endC = newDir === Direction.Horizontal ? startCol + candLen - 1 : startCol;
            const newRows = Math.max(curMaxRow, endR) - Math.min(curMinRow, startRow) + 1;
            const newCols = Math.max(curMaxCol, endC) - Math.min(curMinCol, startCol) + 1;

            if (newRows > newCols) continue;

            const widthBonus = (newCols - newRows) * 5;
            const randomJitter = Math.random() * 4;
            const score = intersections * 10 + candidate.length + widthBonus + randomJitter;
            if (score > bestScore) {
              bestScore = score;
              bestRow = startRow;
              bestCol = startCol;
              bestDir = newDir;
            }
          }
        }
      }
    }

    if (bestScore > 0) {
      placed.push({ text: sortedWords[wi].text, startRow: bestRow, startCol: bestCol, dir: bestDir });
      isPlaced[wi] = true;

      const candLen = candidate.length;
      const endR = bestDir === Direction.Horizontal ? bestRow : bestRow + candLen - 1;
      const endC = bestDir === Direction.Horizontal ? bestCol + candLen - 1 : bestCol;
      curMinRow = Math.min(curMinRow, bestRow);
      curMaxRow = Math.max(curMaxRow, endR);
      curMinCol = Math.min(curMinCol, bestCol);
      curMaxCol = Math.max(curMaxCol, endC);

      for (let i = 0; i < candidate.length; i++) {
        const r = bestDir === Direction.Horizontal ? bestRow : bestRow + i;
        const c = bestDir === Direction.Horizontal ? bestCol + i : bestCol;
        occupiedCells.set(cellKey(r, c), candidate[i]);
      }
    }
  }

  let minRow = Infinity;
  let minCol = Infinity;
  let maxRow = -Infinity;
  let maxCol = -Infinity;

  for (const pw of placed) {
    const endRow = pw.dir === Direction.Horizontal ? pw.startRow : pw.startRow + pw.text.length - 1;
    const endCol = pw.dir === Direction.Horizontal ? pw.startCol + pw.text.length - 1 : pw.startCol;
    minRow = Math.min(minRow, pw.startRow);
    minCol = Math.min(minCol, pw.startCol);
    maxRow = Math.max(maxRow, endRow);
    maxCol = Math.max(maxCol, endCol);
  }

  result.gridRows = maxRow - minRow + 1;
  result.gridCols = maxCol - minCol + 1;

  for (const pw of placed) {
    const cp: CrosswordPlacement = {
      gridRow: pw.startRow - minRow,
      gridCol: pw.startCol - minCol,
      dir: pw.dir
    };
    result.placements.push(cp);

    const match = sortedWords.find((w) => w.text.toLowerCase() === pw.text.toLowerCase());
    if (match) result.placedWords.push(match);
  }

  const cellOwners = new Map<string, [number, number][]>();

  for (let wi = 0; wi < result.placements.length; wi++) {
    const cp = result.placements[wi];
    const word = result.placedWords[wi].text;

    for (let ci = 0; ci < word.length; ci++) {
      const r = cp.dir === Direction.Horizontal ? cp.gridRow : cp.gridRow + ci;
      const c = cp.dir === Direction.Horizontal ? cp.gridCol + ci : cp.gridCol;
      const key = cellKey(r, c);
      if (!cellOwners.has(key)) cellOwners.set(key, []);
      cellOwners.get(key)!.push([wi, ci]);
    }
  }

  for (const [key, owners] of cellOwners.entries()) {
    if (owners.length > 1) {
      result.sharedCells.set(key, owners);
    }
  }

  return result;
}

export function generateCrossword(words: WordInfo[]): CrosswordResult {
  if (words.length === 0) {
    return { placedWords: [], placements: [], sharedCells: new Map(), gridRows: 0, gridCols: 0 };
  }

  const NUM_TRIALS = 20;
  let bestResult: CrosswordResult | null = null;
  let bestScore = -Infinity;

  for (let trial = 0; trial < NUM_TRIALS; trial++) {
    const result = generateCrosswordTrial(words);
    const placed = result.placedWords.length;
    const ratio = result.gridCols / Math.max(result.gridRows, 1);
    const score = placed * 1000 + ratio * 100 - result.gridRows * 10;
    if (score > bestScore) {
      bestScore = score;
      bestResult = result;
    }
  }

  const final = bestResult!;
  console.log(
    `Crossword: placed ${final.placedWords.length} of ${words.length} words into a ` +
    `${final.gridRows}x${final.gridCols} grid with ${final.sharedCells.size} intersections ` +
    `(best of ${NUM_TRIALS} trials).`
  );

  return final;
}

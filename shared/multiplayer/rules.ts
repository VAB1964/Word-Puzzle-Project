import {
  addCreditToParticipant,
  createPositionCredit,
  emptyScore,
  totalCreditValue
} from "./scoring";
import type {
  HintKind,
  Participant,
  PuzzleDefinition,
  PuzzleRuntime,
  ScoreBreakdown
} from "./types";

export const HINT_COSTS: Record<HintKind, number> = {
  letter: 5,
  random: 10,
  "full-word": 15,
  "first-of-each": 20
};

export const GEM_HINT_REWARDS: Record<number, number> = {
  2: 2,
  3: 4,
  4: 6
};

export interface PuzzleActionResult {
  changed: boolean;
  kind: "word" | "bonus" | "hint" | "none";
  solvedWords: string[];
  claimedBonusWord?: string;
  pointsAwarded: number;
  scoreAwarded: ScoreBreakdown;
  hintCreditsAwarded: number;
  error?: string;
}

export const cellKey = (row: number, col: number) => `${row},${col}`;

export const createPuzzleRuntime = (puzzle: PuzzleDefinition): PuzzleRuntime => ({
  visibleCells: {},
  completedWordIds: [],
  credits: Object.fromEntries(puzzle.words.map((word) => [word.id, word.cells.map(() => null)])),
  claimedBonusWords: {},
  failedGuesses: [],
  skipped: false
});

const normalizeGuess = (guess: string) => guess.trim().toLowerCase();
const recordFailedGuess = (runtime: PuzzleRuntime, guess: string) => {
  const failed = runtime.failedGuesses ?? (runtime.failedGuesses = []);
  if (!failed.includes(guess)) failed.push(guess);
};

const bonusHintAward = (length: number) => {
  if (length === 3) return 1;
  if (length === 4) return 2;
  if (length === 5) return 3;
  return length >= 6 ? 4 : 0;
};

const getParticipant = (participants: Participant[], actorId: string) =>
  participants.find((participant) => participant.id === actorId);

const isWordVisible = (puzzle: PuzzleDefinition, runtime: PuzzleRuntime, wordId: string) => {
  const word = puzzle.words.find((candidate) => candidate.id === wordId);
  return Boolean(word && word.cells.every((cell) => runtime.visibleCells[cellKey(cell.row, cell.col)]));
};

const awardPosition = (
  puzzle: PuzzleDefinition,
  runtime: PuzzleRuntime,
  participants: Participant[],
  actorId: string,
  wordId: string,
  position: number,
  scoreAwarded?: ScoreBreakdown
) => {
  const word = puzzle.words.find((candidate) => candidate.id === wordId);
  const actor = getParticipant(participants, actorId);
  const ledger = runtime.credits[wordId];
  if (!word || !actor || !ledger || position < 0 || position >= word.cells.length || ledger[position]) {
    return 0;
  }
  const credit = createPositionCredit(word, actorId, position);
  ledger[position] = credit;
  addCreditToParticipant(actor, credit);
  if (scoreAwarded) addToBreakdown(scoreAwarded, credit.base, credit.gem, credit.bonus);
  return totalCreditValue(credit);
};

const revealPosition = (
  puzzle: PuzzleDefinition,
  runtime: PuzzleRuntime,
  actorId: string,
  wordId: string,
  position: number
) => {
  const word = puzzle.words.find((candidate) => candidate.id === wordId);
  if (!word || position < 0 || position >= word.cells.length) return false;
  const cell = word.cells[position];
  const key = cellKey(cell.row, cell.col);
  if (runtime.visibleCells[key]) return false;
  runtime.visibleCells[key] = {
    letter: word.answer[position].toUpperCase(),
    ownerId: actorId
  };
  return true;
};

const completeVisibleWords = (
  puzzle: PuzzleDefinition,
  runtime: PuzzleRuntime,
  participants: Participant[],
  actorId: string
) => {
  const solved: string[] = [];
  const scoreAwarded = emptyScore();
  let hints = 0;
  const completed = new Set(runtime.completedWordIds);

  for (const word of puzzle.words) {
    if (completed.has(word.id) || !isWordVisible(puzzle, runtime, word.id)) continue;
    for (let position = 0; position < word.cells.length; position += 1) {
      awardPosition(puzzle, runtime, participants, actorId, word.id, position, scoreAwarded);
    }
    completed.add(word.id);
    runtime.completedWordIds.push(word.id);
    solved.push(word.id);
    hints += GEM_HINT_REWARDS[word.rarity] ?? 0;
  }

  const actor = getParticipant(participants, actorId);
  if (actor && hints > 0) actor.hintCredits += hints;
  return { solved, scoreAwarded, hints };
};

export const submitGuess = (
  puzzle: PuzzleDefinition,
  runtime: PuzzleRuntime,
  participants: Participant[],
  actorId: string,
  rawGuess: string
): PuzzleActionResult => {
  const actor = getParticipant(participants, actorId);
  if (!actor) return failure("Participant not found.");
  const guess = normalizeGuess(rawGuess);
  if (guess.length < 3 || !/^[a-z]+$/.test(guess)) return failure("Enter a valid word.");

  const word = puzzle.words.find((candidate) => candidate.answer.toLowerCase() === guess);
  if (word) {
    if (runtime.completedWordIds.includes(word.id)) return failure("That word is already complete.");
    const scoreAwarded = emptyScore();
    for (let position = 0; position < word.cells.length; position += 1) {
      revealPosition(puzzle, runtime, actorId, word.id, position);
      awardPosition(puzzle, runtime, participants, actorId, word.id, position, scoreAwarded);
    }
    const completed = completeVisibleWords(puzzle, runtime, participants, actorId);
    mergeBreakdown(scoreAwarded, completed.scoreAwarded);
    return {
      changed: true,
      kind: "word",
      solvedWords: completed.solved,
      pointsAwarded: scoreAwarded.total,
      scoreAwarded,
      hintCreditsAwarded: completed.hints
    };
  }

  if (puzzle.bonusWords.includes(guess)) {
    if (runtime.claimedBonusWords[guess]) return failure("That bonus word was already claimed.");
    runtime.claimedBonusWords[guess] = actorId;
    const award = bonusHintAward(guess.length);
    actor.hintCredits += award;
    return {
      changed: true,
      kind: "bonus",
      solvedWords: [],
      claimedBonusWord: guess,
      pointsAwarded: 0,
      scoreAwarded: emptyScore(),
      hintCreditsAwarded: award
    };
  }

  recordFailedGuess(runtime, guess);
  return failure(`The word "${guess.toUpperCase()}" is not in the puzzle and is not a bonus word.`);
};

export interface HintRequest {
  hint: HintKind;
  wordId?: string;
  position?: number;
}

export const useHint = (
  puzzle: PuzzleDefinition,
  runtime: PuzzleRuntime,
  participants: Participant[],
  actorId: string,
  request: HintRequest,
  random: () => number = Math.random,
  enabledPowerUps?: Partial<Record<HintKind, boolean>>
): PuzzleActionResult => {
  const actor = getParticipant(participants, actorId);
  if (!actor) return failure("Participant not found.");
  if (enabledPowerUps && enabledPowerUps[request.hint] === false) {
    return failure("That power-up is disabled for this session.");
  }
  const cost = HINT_COSTS[request.hint];
  if (actor.hintCredits < cost) return failure("Not enough hint credits.");

  const unfinished = puzzle.words.filter((word) => !runtime.completedWordIds.includes(word.id));
  const targets: Array<{ wordId: string; position: number }> = [];
  const unrevealed = (wordId: string) => {
    const word = puzzle.words.find((candidate) => candidate.id === wordId);
    if (!word) return [];
    return word.cells
      .map((cell, position) => ({ cell, position }))
      .filter(({ cell }) => !runtime.visibleCells[cellKey(cell.row, cell.col)]);
  };

  if (request.hint === "letter") {
    const word = unfinished.find((candidate) => candidate.id === request.wordId);
    const position = request.position;
    if (
      word &&
      position !== undefined &&
      unrevealed(word.id).some((candidate) => candidate.position === position)
    ) {
      targets.push({ wordId: word.id, position });
    }
  } else if (request.hint === "random") {
    for (const word of unfinished) {
      const choices = unrevealed(word.id);
      if (choices.length > 0) {
        const index = Math.min(choices.length - 1, Math.floor(random() * choices.length));
        targets.push({ wordId: word.id, position: choices[index].position });
      }
    }
  } else if (request.hint === "full-word") {
    const word =
      unfinished.find((candidate) => candidate.id === request.wordId) ??
      unfinished[unfinished.length - 1];
    if (word) {
      for (const choice of unrevealed(word.id)) {
        targets.push({ wordId: word.id, position: choice.position });
      }
    }
  } else {
    for (const word of unfinished) {
      const choice = unrevealed(word.id)[0];
      if (choice) targets.push({ wordId: word.id, position: choice.position });
    }
  }

  if (targets.length === 0) return failure("That hint has no eligible target.");

  actor.hintCredits -= cost;
  const scoreAwarded = emptyScore();
  for (const target of targets) {
    revealPosition(puzzle, runtime, actorId, target.wordId, target.position);
    awardPosition(
      puzzle,
      runtime,
      participants,
      actorId,
      target.wordId,
      target.position,
      scoreAwarded
    );
  }
  const completed = completeVisibleWords(puzzle, runtime, participants, actorId);
  mergeBreakdown(scoreAwarded, completed.scoreAwarded);
  return {
    changed: true,
    kind: "hint",
    solvedWords: completed.solved,
    pointsAwarded: scoreAwarded.total,
    scoreAwarded,
    hintCreditsAwarded: completed.hints
  };
};

export const puzzleIsComplete = (puzzle: PuzzleDefinition, runtime: PuzzleRuntime) =>
  runtime.completedWordIds.length === puzzle.words.length;

const failure = (error: string): PuzzleActionResult => ({
  changed: false,
  kind: "none",
  solvedWords: [],
  pointsAwarded: 0,
  scoreAwarded: emptyScore(),
  hintCreditsAwarded: 0,
  error
});

const addToBreakdown = (score: ScoreBreakdown, base: number, gem: "none" | "emerald" | "diamond" | "ruby", bonus: number) => {
  score.letters += base;
  if (gem !== "none") score[gem] += bonus;
  score.total = score.letters + score.emerald + score.diamond + score.ruby;
};

const mergeBreakdown = (target: ScoreBreakdown, source: ScoreBreakdown) => {
  target.letters += source.letters;
  target.emerald += source.emerald;
  target.diamond += source.diamond;
  target.ruby += source.ruby;
  target.total = target.letters + target.emerald + target.diamond + target.ruby;
};

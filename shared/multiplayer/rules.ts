import {
  addCreditToParticipant,
  createPositionCredit,
  emptyScore,
  scoreTotal,
  totalCreditValue
} from "./scoring";
import type {
  HintKind,
  MysteryRewardPresentation,
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
  wordScores: Record<string, ScoreBreakdown>;
  mysteryRewards: MysteryRewardPresentation[];
  retainTurn: boolean;
  freePowerUsed: boolean;
  error?: string;
  needsPuzzleReview?: boolean;
}

export const cellKey = (row: number, col: number) => `${row},${col}`;

export const createPuzzleRuntime = (puzzle: PuzzleDefinition): PuzzleRuntime => ({
  visibleCells: {},
  completedWordIds: [],
  credits: Object.fromEntries(puzzle.words.map((word) => [word.id, word.cells.map(() => null)])),
  claimedBonusWords: {},
  failedGuesses: [],
  claimedMysteryGems: {},
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

const canReplacePuzzleWord = (
  puzzle: PuzzleDefinition,
  runtime: PuzzleRuntime,
  wordId: string,
  replacement: string
) => {
  const word = puzzle.words.find((candidate) => candidate.id === wordId);
  if (!word || word.answer.length !== replacement.length || runtime.completedWordIds.includes(wordId)) {
    return false;
  }

  for (let position = 0; position < word.cells.length; position += 1) {
    const cell = word.cells[position];
    const visible = runtime.visibleCells[cellKey(cell.row, cell.col)];
    if (visible && visible.letter.toLowerCase() !== replacement[position]) return false;

    if (puzzle.mode !== "Crossword") continue;
    for (const crossing of puzzle.words) {
      if (crossing.id === wordId) continue;
      const crossingPosition = crossing.cells.findIndex(
        (candidate) => candidate.row === cell.row && candidate.col === cell.col
      );
      if (
        crossingPosition >= 0 &&
        crossing.answer[crossingPosition].toLowerCase() !== replacement[position]
      ) {
        return false;
      }
    }
  }
  return true;
};

const compareCasualWords = (
  left: PuzzleDefinition["words"][number],
  right: PuzzleDefinition["words"][number]
) =>
  left.answer.length - right.answer.length ||
  left.answer.localeCompare(right.answer, undefined, { sensitivity: "base" });

const keepsCasualWordsSorted = (
  puzzle: PuzzleDefinition,
  wordId: string,
  replacement: string
) => {
  const answers = puzzle.words.map((word) => ({
    ...word,
    answer: word.id === wordId ? replacement : word.answer
  }));
  return answers.every(
    (word, index) => index === 0 || compareCasualWords(answers[index - 1], word) <= 0
  );
};

const promoteBonusWord = (
  puzzle: PuzzleDefinition,
  runtime: PuzzleRuntime,
  guess: string
) => {
  const word = puzzle.words.find(
    (candidate) =>
      canReplacePuzzleWord(puzzle, runtime, candidate.id, guess) &&
      (puzzle.mode !== "Casual" || keepsCasualWordsSorted(puzzle, candidate.id, guess))
  );
  if (!word) return null;

  const previousAnswer = word.answer.toLowerCase();
  word.answer = guess;
  // These fields describe the old answer. The worker fills the new word's
  // dictionary details when it publishes a completed word.
  word.pos = undefined;
  word.definition = undefined;
  word.sentence = undefined;
  puzzle.bonusWords = puzzle.bonusWords.map((candidate) =>
    candidate === guess ? previousAnswer : candidate
  );
  return word;
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
  const wordScores: Record<string, ScoreBreakdown> = {};
  let hints = 0;
  const completed = new Set(runtime.completedWordIds);

  for (const word of puzzle.words) {
    if (completed.has(word.id) || !isWordVisible(puzzle, runtime, word.id)) continue;
    const wordScore = emptyScore();
    for (let position = 0; position < word.cells.length; position += 1) {
      awardPosition(puzzle, runtime, participants, actorId, word.id, position, wordScore);
    }
    mergeBreakdown(scoreAwarded, wordScore);
    wordScores[word.id] = wordScore;
    completed.add(word.id);
    runtime.completedWordIds.push(word.id);
    solved.push(word.id);
    hints += GEM_HINT_REWARDS[word.rarity] ?? 0;
  }

  const actor = getParticipant(participants, actorId);
  if (actor && hints > 0) actor.hintCredits += hints;
  return { solved, scoreAwarded, wordScores, hints };
};

export const submitGuess = (
  puzzle: PuzzleDefinition,
  runtime: PuzzleRuntime,
  participants: Participant[],
  actorId: string,
  rawGuess: string,
  dictionaryWords?: ReadonlySet<string>,
  includeBonusWordsWhenPossible = false,
  random: () => number = Math.random
): PuzzleActionResult => {
  const actor = getParticipant(participants, actorId);
  if (!actor) return failure("Participant not found.");
  const guess = normalizeGuess(rawGuess);
  if (guess.length < 3 || !/^[a-z]+$/.test(guess)) return failure("Enter a valid word.");

  let word = puzzle.words.find((candidate) => candidate.answer.toLowerCase() === guess);
  if (!word && includeBonusWordsWhenPossible && puzzle.bonusWords.includes(guess)) {
    word = promoteBonusWord(puzzle, runtime, guess) ?? undefined;
  }
  if (word) {
    if (runtime.completedWordIds.includes(word.id)) return failure("That word is already complete.");
    const scoreAwarded = emptyScore();
    for (let position = 0; position < word.cells.length; position += 1) {
      revealPosition(puzzle, runtime, actorId, word.id, position);
      awardPosition(puzzle, runtime, participants, actorId, word.id, position, scoreAwarded);
    }
    const primaryScoreAwarded = { ...scoreAwarded };
    const completed = completeVisibleWords(puzzle, runtime, participants, actorId);
    mergeBreakdown(scoreAwarded, completed.scoreAwarded);
    const wordScores = completed.wordScores;
    const primaryScore = wordScores[word.id] ?? emptyScore();
    mergeBreakdown(primaryScore, primaryScoreAwarded);
    wordScores[word.id] = primaryScore;
    const result: PuzzleActionResult = {
      changed: true,
      kind: "word",
      solvedWords: completed.solved,
      pointsAwarded: scoreAwarded.total,
      scoreAwarded,
      hintCreditsAwarded: completed.hints,
      wordScores,
      mysteryRewards: [],
      retainTurn: false,
      freePowerUsed: false
    };
    resolveMysteryRewards(puzzle, runtime, participants, actorId, result, random);
    return result;
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
      hintCreditsAwarded: award,
      wordScores: {},
      mysteryRewards: [],
      retainTurn: false,
      freePowerUsed: false
    };
  }

  recordFailedGuess(runtime, guess);
  if (dictionaryWords?.has(guess)) {
    return {
      ...failure("This is a valid word but wasn't included as part of the puzzle or bonus words"),
      needsPuzzleReview: true
    };
  }
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
  if (request.hint === "letter" && request.wordId && request.position !== undefined) {
    const requestedWord = puzzle.words.find((word) => word.id === request.wordId);
    const requestedCell = requestedWord?.cells[request.position];
    const targetsMysteryGem = requestedCell && (puzzle.mysteryGems ?? []).some(
      (gem) => gem.cell.row === requestedCell.row && gem.cell.col === requestedCell.col
    );
    if (targetsMysteryGem) {
      return failure("Mystery gems can only be uncovered by solving their word.");
    }
  }
  const cost = HINT_COSTS[request.hint];
  const bankedFreePowers = actor.freePowerTokens ?? 0;
  const usesFreePower = bankedFreePowers > 0;
  if (!usesFreePower && actor.hintCredits < cost) return failure("Not enough hint credits.");

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

  if (usesFreePower) actor.freePowerTokens = bankedFreePowers - 1;
  else actor.hintCredits -= cost;
  const scoreAwarded = emptyScore();
  const wordScores: Record<string, ScoreBreakdown> = {};
  for (const target of targets) {
    revealPosition(puzzle, runtime, actorId, target.wordId, target.position);
    const wordScore = wordScores[target.wordId] ?? (wordScores[target.wordId] = emptyScore());
    awardPosition(
      puzzle,
      runtime,
      participants,
      actorId,
      target.wordId,
      target.position,
      wordScore
    );
  }
  Object.values(wordScores).forEach((score) => mergeBreakdown(scoreAwarded, score));
  const completed = completeVisibleWords(puzzle, runtime, participants, actorId);
  mergeBreakdown(scoreAwarded, completed.scoreAwarded);
  for (const [wordId, score] of Object.entries(completed.wordScores)) {
    const aggregate = wordScores[wordId] ?? (wordScores[wordId] = emptyScore());
    mergeBreakdown(aggregate, score);
  }
  const result: PuzzleActionResult = {
    changed: true,
    kind: "hint",
    solvedWords: completed.solved,
    pointsAwarded: scoreAwarded.total,
    scoreAwarded,
    hintCreditsAwarded: completed.hints,
    wordScores,
    mysteryRewards: [],
    retainTurn: false,
    freePowerUsed: usesFreePower
  };
  resolveMysteryRewards(puzzle, runtime, participants, actorId, result, random);
  return result;
};

export const puzzleIsComplete = (puzzle: PuzzleDefinition, runtime: PuzzleRuntime) =>
  runtime.completedWordIds.length === puzzle.words.length;

const mergeWordScore = (
  wordScores: Record<string, ScoreBreakdown>,
  wordId: string,
  score: ScoreBreakdown
) => {
  const aggregate = wordScores[wordId] ?? (wordScores[wordId] = emptyScore());
  mergeBreakdown(aggregate, score);
};

const shuffleInPlace = <T>(items: T[], random: () => number) => {
  for (let index = items.length - 1; index > 0; index -= 1) {
    const swap = Math.floor(random() * (index + 1));
    [items[index], items[swap]] = [items[swap], items[index]];
  }
  return items;
};

const bloomGems = (
  puzzle: PuzzleDefinition,
  runtime: PuzzleRuntime,
  random: () => number
) => {
  const mysteryCells = new Set((puzzle.mysteryGems ?? []).map((gem) => cellKey(gem.cell.row, gem.cell.col)));
  const ranks = { none: 0, emerald: 1, ruby: 2, diamond: 3 } as const;
  const candidates = new Map<string, {
    cell: { row: number; col: number };
    refs: Array<{ word: PuzzleDefinition["words"][number]; position: number }>;
  }>();
  for (const word of puzzle.words) {
    word.cells.forEach((cell, position) => {
      const key = cellKey(cell.row, cell.col);
      if (mysteryCells.has(key) || runtime.visibleCells[key]) return;
      const candidate = candidates.get(key) ?? { cell, refs: [] };
      candidate.refs.push({ word, position });
      candidates.set(key, candidate);
    });
  }

  const upgradeable = [...candidates.values()].filter((candidate) =>
    candidate.refs.every(({ word, position }) => (word.gems[position] ?? "none") !== "diamond")
  );
  return shuffleInPlace(upgradeable, random).slice(0, 3).map((target) => {
    const strongest = [...target.refs].sort((left, right) =>
      ranks[right.word.gems[right.position] ?? "none"] - ranks[left.word.gems[left.position] ?? "none"]
    )[0];
    const current = strongest.word.gems[strongest.position] ?? "none";
    strongest.word.gems[strongest.position] =
      current === "none" ? "emerald" : current === "emerald" ? "ruby" : "diamond";
    return target.cell;
  });
};

const applyLetterBurst = (
  puzzle: PuzzleDefinition,
  runtime: PuzzleRuntime,
  participants: Participant[],
  actorId: string,
  result: PuzzleActionResult,
  random: () => number
) => {
  const hiddenByLetter = new Map<string, Map<string, { row: number; col: number }>>();
  for (const word of puzzle.words) {
    word.cells.forEach((cell, position) => {
      const key = cellKey(cell.row, cell.col);
      if (runtime.visibleCells[key]) return;
      const letter = word.answer[position].toUpperCase();
      const cells = hiddenByLetter.get(letter) ?? new Map();
      cells.set(key, cell);
      hiddenByLetter.set(letter, cells);
    });
  }
  const letters = [...hiddenByLetter.keys()];
  if (letters.length === 0) return { letter: "", cells: [] as Array<{ row: number; col: number }> };
  const letter = letters[Math.min(letters.length - 1, Math.floor(random() * letters.length))];
  const targets = hiddenByLetter.get(letter)!;
  const burstScores: Record<string, ScoreBreakdown> = {};

  for (const word of puzzle.words) {
    word.cells.forEach((cell, position) => {
      if (word.answer[position].toUpperCase() !== letter || !targets.has(cellKey(cell.row, cell.col))) return;
      revealPosition(puzzle, runtime, actorId, word.id, position);
      const wordScore = burstScores[word.id] ?? (burstScores[word.id] = emptyScore());
      awardPosition(puzzle, runtime, participants, actorId, word.id, position, wordScore);
    });
  }

  for (const [wordId, score] of Object.entries(burstScores)) {
    mergeWordScore(result.wordScores, wordId, score);
    mergeBreakdown(result.scoreAwarded, score);
    result.pointsAwarded += score.total;
  }
  const completed = completeVisibleWords(puzzle, runtime, participants, actorId);
  for (const [wordId, score] of Object.entries(completed.wordScores)) {
    mergeWordScore(result.wordScores, wordId, score);
  }
  mergeBreakdown(result.scoreAwarded, completed.scoreAwarded);
  result.pointsAwarded += completed.scoreAwarded.total;
  result.hintCreditsAwarded += completed.hints;
  for (const wordId of completed.solved) {
    if (!result.solvedWords.includes(wordId)) result.solvedWords.push(wordId);
  }
  return { letter, cells: [...targets.values()] };
};

const resolveMysteryRewards = (
  puzzle: PuzzleDefinition,
  runtime: PuzzleRuntime,
  participants: Participant[],
  actorId: string,
  result: PuzzleActionResult,
  random: () => number
) => {
  const actor = getParticipant(participants, actorId);
  const mysteryGems = puzzle.mysteryGems ?? [];
  if (!actor || mysteryGems.length === 0) return;
  const claimedMysteryGems = runtime.claimedMysteryGems ?? (runtime.claimedMysteryGems = {});
  actor.freePowerTokens ??= 0;
  const queue = [...result.solvedWords];
  const inspected = new Set<string>();

  while (queue.length > 0) {
    const wordId = queue.shift()!;
    if (inspected.has(wordId)) continue;
    inspected.add(wordId);
    const mystery = mysteryGems.find(
      (gem) => gem.wordId === wordId && !claimedMysteryGems[gem.id]
    );
    if (!mystery) continue;
    claimedMysteryGems[mystery.id] = { actorId, effect: mystery.effect, at: Date.now() };
    const reward: MysteryRewardPresentation = {
      id: mystery.id,
      effect: mystery.effect,
      wordId,
      cell: mystery.cell
    };

    if (mystery.effect === "triple-word") {
      const wordScore = result.wordScores[wordId] ?? emptyScore();
      const extraPoints = wordScore.total * 2;
      if (extraPoints > 0) {
        actor.score.bonus = (actor.score.bonus ?? 0) + extraPoints;
        actor.score.total = scoreTotal(actor.score);
        result.scoreAwarded.bonus += extraPoints;
        result.scoreAwarded.total = scoreTotal(result.scoreAwarded);
        wordScore.bonus += extraPoints;
        wordScore.total = scoreTotal(wordScore);
        result.pointsAwarded += extraPoints;
      }
      reward.extraPoints = extraPoints;
    } else if (mystery.effect === "extra-turn") {
      result.retainTurn = true;
    } else if (mystery.effect === "free-power") {
      actor.freePowerTokens += 1;
    } else if (mystery.effect === "gem-bloom") {
      reward.affectedCells = bloomGems(puzzle, runtime, random);
    } else if (mystery.effect === "letter-burst") {
      const before = new Set(result.solvedWords);
      const burst = applyLetterBurst(puzzle, runtime, participants, actorId, result, random);
      reward.letter = burst.letter;
      reward.affectedCells = burst.cells;
      result.solvedWords.filter((id) => !before.has(id)).forEach((id) => queue.push(id));
    }
    result.mysteryRewards.push(reward);
  }
};

const failure = (error: string): PuzzleActionResult => ({
  changed: false,
  kind: "none",
  solvedWords: [],
  pointsAwarded: 0,
  scoreAwarded: emptyScore(),
  hintCreditsAwarded: 0,
  wordScores: {},
  mysteryRewards: [],
  retainTurn: false,
  freePowerUsed: false,
  error
});

const addToBreakdown = (score: ScoreBreakdown, base: number, gem: "none" | "emerald" | "diamond" | "ruby", bonus: number) => {
  score.letters += base;
  if (gem !== "none") score[gem] += bonus;
  score.total = scoreTotal(score);
};

const mergeBreakdown = (target: ScoreBreakdown, source: ScoreBreakdown) => {
  target.letters += source.letters;
  target.emerald += source.emerald;
  target.diamond += source.diamond;
  target.ruby += source.ruby;
  target.bonus += source.bonus;
  target.total = scoreTotal(target);
};

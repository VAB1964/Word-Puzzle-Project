import type {
  GemType,
  Participant,
  PositionCredit,
  PuzzleWordDefinition,
  ScoreBreakdown
} from "./types";

export const GEM_BONUS: Record<GemType, number> = {
  none: 0,
  emerald: 5,
  ruby: 10,
  diamond: 15
};

export const emptyScore = (): ScoreBreakdown => ({
  letters: 0,
  emerald: 0,
  diamond: 0,
  ruby: 0,
  bonus: 0,
  total: 0
});

export const scoreTotal = (score: ScoreBreakdown) =>
  score.letters + score.emerald + score.diamond + score.ruby + (score.bonus ?? 0);

export const gemForPosition = (word: PuzzleWordDefinition, position: number): GemType => {
  if (position < 0 || position >= word.gems.length) return "none";
  return word.gems[position] ?? "none";
};

export const positionValue = (word: PuzzleWordDefinition, position: number) => {
  const gem = gemForPosition(word, position);
  return { base: 1, gem, bonus: GEM_BONUS[gem], total: 1 + GEM_BONUS[gem] };
};

export const createPositionCredit = (
  word: PuzzleWordDefinition,
  ownerId: string,
  position: number
): PositionCredit => {
  const value = positionValue(word, position);
  return { ownerId, base: value.base, gem: value.gem, bonus: value.bonus };
};

export const addCreditToParticipant = (participant: Participant, credit: PositionCredit) => {
  participant.score.letters += credit.base;
  if (credit.gem !== "none") {
    participant.score[credit.gem] += credit.bonus;
  }
  participant.score.total = scoreTotal(participant.score);
};

/** Consumes a player's banked hint credits at the end of a session. */
export const awardUnusedHintCredits = (participant: Participant) => {
  const award = Math.max(0, Math.floor(participant.hintCredits));
  participant.score.bonus = (participant.score.bonus ?? 0) + award;
  participant.hintCredits = 0;
  participant.score.total = scoreTotal(participant.score);
  return award;
};

export const totalCreditValue = (credit: PositionCredit) => credit.base + credit.bonus;

export const wordMaximumScore = (word: PuzzleWordDefinition) =>
  word.answer.split("").reduce((total, _, position) => total + positionValue(word, position).total, 0);

export const puzzleMaximumScore = (words: PuzzleWordDefinition[]) =>
  words.reduce((total, word) => total + wordMaximumScore(word), 0);

export const scoreIsConsistent = (score: ScoreBreakdown) =>
  score.total === scoreTotal(score);

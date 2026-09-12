import type {
  GemType,
  Participant,
  PositionCredit,
  PuzzleWordDefinition,
  ScoreBreakdown
} from "./types";

export const GEM_BONUS: Record<GemType, number> = {
  none: 0,
  diamond: 5,
  ruby: 10,
  emerald: 15
};

export const emptyScore = (): ScoreBreakdown => ({
  letters: 0,
  emerald: 0,
  diamond: 0,
  ruby: 0,
  total: 0
});

export const gemForRarity = (rarity: number): GemType => {
  if (rarity === 2) return "emerald";
  if (rarity === 3) return "ruby";
  if (rarity === 4) return "diamond";
  return "none";
};

export const positionValue = (word: PuzzleWordDefinition) => {
  const gem = gemForRarity(word.rarity);
  return { base: 1, gem, bonus: GEM_BONUS[gem], total: 1 + GEM_BONUS[gem] };
};

export const createPositionCredit = (
  word: PuzzleWordDefinition,
  ownerId: string
): PositionCredit => {
  const value = positionValue(word);
  return { ownerId, base: value.base, gem: value.gem, bonus: value.bonus };
};

export const addCreditToParticipant = (participant: Participant, credit: PositionCredit) => {
  participant.score.letters += credit.base;
  if (credit.gem !== "none") {
    participant.score[credit.gem] += credit.bonus;
  }
  participant.score.total =
    participant.score.letters +
    participant.score.emerald +
    participant.score.diamond +
    participant.score.ruby;
};

export const totalCreditValue = (credit: PositionCredit) => credit.base + credit.bonus;

export const wordMaximumScore = (word: PuzzleWordDefinition) =>
  word.answer.length * positionValue(word).total;

export const puzzleMaximumScore = (words: PuzzleWordDefinition[]) =>
  words.reduce((total, word) => total + wordMaximumScore(word), 0);

export const scoreIsConsistent = (score: ScoreBreakdown) =>
  score.total === score.letters + score.emerald + score.diamond + score.ruby;

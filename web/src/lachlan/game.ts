import { Assets } from "../assets";
import { loadProcessedWordList, subWords } from "../data/words";
import {
  buildAcceptedWords,
  LACHLAN_WORDS,
  loadLachlanProgress,
  saveLachlanProgress,
  type LachlanWord
} from "./data";

type FeedbackKind = "neutral" | "good" | "error";

const shuffleLetters = (word: string) => {
  const letters = word.toUpperCase().split("");
  for (let index = letters.length - 1; index > 0; index -= 1) {
    const swapIndex = Math.floor(Math.random() * (index + 1));
    [letters[index], letters[swapIndex]] = [letters[swapIndex], letters[index]];
  }
  return letters;
};

export class LachlanGame {
  private index = loadLachlanProgress();
  private acceptedWords = new Set<string>();
  private foundBonusWords = new Set<string>();
  private letters: string[] = [];
  private selectedIndices: number[] = [];
  private dragging = false;
  private sentenceOpen = false;
  private dictionaryWords: string[] = [];
  private audioContext: AudioContext | null = null;

  constructor(private readonly root: HTMLElement) {
    this.root.addEventListener("pointerdown", this.handlePointerDown);
    this.root.addEventListener("pointerover", this.handlePointerOver);
    window.addEventListener("pointerup", this.handlePointerUp);
    window.addEventListener("keydown", this.handleKeyDown);
    void this.initialize();
  }

  private async initialize() {
    try {
      const dictionary = await loadProcessedWordList(Assets.wordsCsv);
      this.dictionaryWords = dictionary.map((entry) => entry.text);
    } catch (error) {
      console.warn("The bonus-word dictionary could not be loaded.", error);
    }
    this.startCurrentWord();
  }

  private get current(): LachlanWord | null {
    return LACHLAN_WORDS[this.index] ?? null;
  }

  private startCurrentWord() {
    const current = this.current;
    this.selectedIndices = [];
    this.foundBonusWords.clear();
    this.sentenceOpen = false;
    if (!current) {
      this.renderComplete();
      return;
    }

    this.letters = shuffleLetters(current.word);
    const dictionary = this.dictionaryWords.length
      ? subWords(
          current.word,
          this.dictionaryWords.map((text) => ({
            text,
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
          })),
          true
        ).map((entry) => entry.text)
      : [];
    this.acceptedWords = buildAcceptedWords(current.word, dictionary);
    this.render();
  }

  private render() {
    const current = this.current;
    if (!current) {
      this.renderComplete();
      return;
    }

    this.root.innerHTML = `
      <main class="lachlan-game" aria-labelledby="lachlan-title">
        <header class="lachlan-header">
          <a class="lachlan-home" href="/">Back to VAB Games</a>
          <div>
            <p class="lachlan-eyebrow">Halloween Spelling</p>
            <h1 id="lachlan-title">Lachlan's Word List</h1>
          </div>
          <p class="lachlan-progress">Word <strong>${this.index + 1}</strong> of ${LACHLAN_WORDS.length}</p>
        </header>

        <section class="lachlan-board">
          <div class="lachlan-target-frame" aria-label="The framed row is your spelling word">
            <span class="lachlan-target-label">Spelling Word</span>
            <div class="lachlan-target-tiles">
              ${current.word
                .split("")
                .map(
                  (_, tileIndex) => `
                    <span class="lachlan-target-tile">
                      <img src="${Assets.lachlan.pumpkin}" alt="" aria-hidden="true">
                      <b>${this.sentenceOpen ? current.word[tileIndex].toUpperCase() : ""}</b>
                    </span>`
                )
                .join("")}
            </div>
          </div>

          <div class="lachlan-play-area">
            <div class="lachlan-guess" aria-live="polite">${this.currentGuess || "&nbsp;"}</div>
            <div class="lachlan-wheel" role="group" aria-label="Letter wheel">
              <svg class="lachlan-path" viewBox="0 0 100 100" aria-hidden="true">
                <polyline points="${this.pathPoints}" />
              </svg>
              ${this.letters
                .map(
                  (letter, letterIndex) => `
                    <button
                      type="button"
                      class="lachlan-letter ${this.selectedIndices.includes(letterIndex) ? "selected" : ""}"
                      style="--letter-index:${letterIndex};--letter-count:${this.letters.length}"
                      data-letter-index="${letterIndex}"
                      aria-pressed="${this.selectedIndices.includes(letterIndex)}"
                      aria-label="Choose ${letter}"
                    >${letter}</button>`
                )
                .join("")}
              <button type="button" class="lachlan-shuffle" data-action="shuffle" aria-label="Shuffle letters">↝</button>
            </div>
            <div class="lachlan-actions">
              <button type="button" data-action="clear">Clear</button>
              <button type="button" class="primary" data-action="submit">Check Word</button>
            </div>
          </div>

          <aside class="lachlan-bonus-panel">
            <h2>Bonus Words</h2>
            <p>Make other words before you solve the spelling word.</p>
            <div class="lachlan-bonus-list">
              ${
                this.foundBonusWords.size
                  ? [...this.foundBonusWords].sort().map((word) => `<span>${word.toUpperCase()}</span>`).join("")
                  : "<em>No bonus words yet</em>"
              }
            </div>
          </aside>
        </section>

        <p class="lachlan-feedback" data-feedback aria-live="assertive">Spell the framed word to continue.</p>

        ${
          this.sentenceOpen
            ? `<div class="lachlan-modal-layer" role="dialog" aria-modal="true" aria-labelledby="sentence-title">
                <section class="lachlan-sentence-popup">
                  <img src="${Assets.lachlan.pumpkin}" alt="" aria-hidden="true">
                  <p id="sentence-title">Great spelling!</p>
                  <strong>${current.word.toUpperCase()}</strong>
                  <blockquote>${current.sentence}</blockquote>
                  <button type="button" class="primary" data-action="next">Next Word</button>
                </section>
              </div>`
            : ""
        }
      </main>`;

    this.bindClickHandlers();
    if (this.sentenceOpen) {
      requestAnimationFrame(() => this.root.querySelector<HTMLButtonElement>("[data-action='next']")?.focus());
    }
  }

  private renderComplete() {
    this.root.innerHTML = `
      <main class="lachlan-game lachlan-complete">
        <section>
          <img src="${Assets.lachlan.pumpkin}" alt="" aria-hidden="true">
          <p class="lachlan-eyebrow">Halloween Spelling</p>
          <h1>Amazing work, Lachlan!</h1>
          <p>You spelled all ${LACHLAN_WORDS.length} words.</p>
          <button type="button" class="primary" data-action="restart">Start Again</button>
          <a href="/">Back to VAB Games</a>
        </section>
      </main>`;
    this.bindClickHandlers();
  }

  private get currentGuess() {
    return this.selectedIndices.map((index) => this.letters[index]).join("");
  }

  private get pathPoints() {
    return this.selectedIndices
      .map((index) => {
        const angle = (index / this.letters.length) * Math.PI * 2 - Math.PI / 2;
        return `${50 + Math.cos(angle) * 35},${50 + Math.sin(angle) * 35}`;
      })
      .join(" ");
  }

  private selectLetter(index: number) {
    if (this.sentenceOpen || this.selectedIndices.includes(index)) return;
    this.selectedIndices.push(index);
    this.playTone(this.selectedIndices.length);
    this.render();
  }

  private submitGuess() {
    if (this.sentenceOpen) return;
    const current = this.current;
    if (!current) return;
    const guess = this.currentGuess.toLowerCase();
    if (guess.length < 2) {
      this.setFeedback("Choose at least two letters.", "error");
      return;
    }
    if (guess === current.word) {
      this.sentenceOpen = true;
      this.playSuccess();
      this.render();
      return;
    }
    if (this.acceptedWords.has(guess)) {
      const alreadyFound = this.foundBonusWords.has(guess);
      this.foundBonusWords.add(guess);
      this.selectedIndices = [];
      this.render();
      this.setFeedback(alreadyFound ? "You already found that bonus word." : "Bonus word found!", "good");
      return;
    }
    this.selectedIndices = [];
    this.render();
    this.setFeedback("Try another word.", "error");
  }

  private nextWord() {
    if (!this.sentenceOpen) return;
    this.index += 1;
    saveLachlanProgress(this.index);
    this.startCurrentWord();
  }

  private setFeedback(message: string, kind: FeedbackKind) {
    const feedback = this.root.querySelector<HTMLElement>("[data-feedback]");
    if (!feedback) return;
    feedback.textContent = message;
    feedback.dataset.kind = kind;
  }

  private bindClickHandlers() {
    this.root.querySelectorAll<HTMLElement>("[data-letter-index]").forEach((button) => {
      button.onclick = () => this.selectLetter(Number(button.dataset.letterIndex));
    });
    const clearButton = this.root.querySelector<HTMLElement>("[data-action='clear']");
    if (clearButton) clearButton.onclick = () => {
      this.selectedIndices = [];
      this.render();
    };
    const shuffleButton = this.root.querySelector<HTMLElement>("[data-action='shuffle']");
    if (shuffleButton) shuffleButton.onclick = () => {
      this.letters = shuffleLetters(this.current?.word ?? "");
      this.selectedIndices = [];
      this.render();
    };
    const submitButton = this.root.querySelector<HTMLElement>("[data-action='submit']");
    if (submitButton) submitButton.onclick = () => this.submitGuess();
    const nextButton = this.root.querySelector<HTMLElement>("[data-action='next']");
    if (nextButton) nextButton.onclick = () => this.nextWord();
    const restartButton = this.root.querySelector<HTMLElement>("[data-action='restart']");
    if (restartButton) restartButton.onclick = () => {
      this.index = 0;
      saveLachlanProgress(0);
      this.startCurrentWord();
    };
  }

  private handlePointerDown = (event: PointerEvent) => {
    const button = (event.target as HTMLElement).closest<HTMLElement>("[data-letter-index]");
    if (!button || this.sentenceOpen) return;
    event.preventDefault();
    this.dragging = true;
    this.selectLetter(Number(button.dataset.letterIndex));
  };

  private handlePointerOver = (event: PointerEvent) => {
    if (!this.dragging) return;
    const button = (event.target as HTMLElement).closest<HTMLElement>("[data-letter-index]");
    if (button) this.selectLetter(Number(button.dataset.letterIndex));
  };

  private handlePointerUp = () => {
    if (!this.dragging) return;
    this.dragging = false;
  };

  private handleKeyDown = (event: KeyboardEvent) => {
    if (this.sentenceOpen) {
      if (event.key === "Enter") this.nextWord();
      return;
    }
    if (event.key === "Enter") {
      this.submitGuess();
      return;
    }
    if (event.key === "Backspace" || event.key === "Escape") {
      this.selectedIndices = [];
      this.render();
      return;
    }
    if (!/^[a-z]$/i.test(event.key)) return;
    const index = this.letters.findIndex(
      (letter, letterIndex) =>
        letter === event.key.toUpperCase() && !this.selectedIndices.includes(letterIndex)
    );
    if (index >= 0) this.selectLetter(index);
  };

  private playTone(position: number) {
    try {
      this.audioContext ??= new AudioContext();
      const oscillator = this.audioContext.createOscillator();
      const gain = this.audioContext.createGain();
      oscillator.frequency.value = 300 + position * 45;
      gain.gain.setValueAtTime(0.06, this.audioContext.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.001, this.audioContext.currentTime + 0.09);
      oscillator.connect(gain).connect(this.audioContext.destination);
      oscillator.start();
      oscillator.stop(this.audioContext.currentTime + 0.1);
    } catch {
      // Sound is optional.
    }
  }

  private playSuccess() {
    this.playTone(5);
    window.setTimeout(() => this.playTone(7), 100);
  }
}

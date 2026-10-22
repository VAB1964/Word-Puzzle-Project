export const Assets = {
  lachlan: {
    background: new URL("../../assets/halloween/background.svg", import.meta.url).toString(),
    pumpkin: new URL("../../assets/halloween/pumpkin.svg", import.meta.url).toString()
  },
  livingLibrary: {
    libraryLadder: new URL("../../assets/living-library/ladder-background.webp", import.meta.url).toString(),
    libraryBackground: new URL("../../assets/living-library/gameplay-background.webp", import.meta.url).toString(),
    libraryWheel: new URL("../../assets/living-library/letter-wheel.svg", import.meta.url).toString(),
    libraryTileEmpty: new URL("../../assets/living-library/tile-empty.svg", import.meta.url).toString(),
    libraryTileSolved: new URL("../../assets/living-library/tile-solved.svg", import.meta.url).toString(),
    libraryBook0: new URL("../../assets/living-library/book-emerald.webp", import.meta.url).toString(),
    libraryBook1: new URL("../../assets/living-library/book-sapphire.webp", import.meta.url).toString(),
    libraryBook2: new URL("../../assets/living-library/book-ruby.webp", import.meta.url).toString(),
    libraryBook3: new URL("../../assets/living-library/book-amethyst.webp", import.meta.url).toString(),
  },
  menuBackground: new URL("../../assets/cozy-library/paper.svg", import.meta.url).toString(),
  mainBackground: new URL("../../assets/cozy-library/desktop.png", import.meta.url).toString(),
  sapphire: new URL("../../assets/cozy-library/emerald.svg", import.meta.url).toString(),
  ruby: new URL("../../assets/cozy-library/ruby.svg", import.meta.url).toString(),
  diamond: new URL("../../assets/cozy-library/diamond.svg", import.meta.url).toString(),
  fonts: {
    arialBold: new URL("../../fonts/arialbd.ttf", import.meta.url).toString()
  },
  sounds: {
    select: new URL("../../assets/sounds/select_letter.wav", import.meta.url).toString(),
    place: new URL("../../assets/sounds/place_letter.wav", import.meta.url).toString(),
    win: new URL("../../assets/sounds/puzzle_solved.wav", import.meta.url).toString(),
    click: new URL("../../assets/sounds/button_click.wav", import.meta.url).toString(),
    hintUsed: new URL("../../assets/sounds/button_click.wav", import.meta.url).toString(),
    error: new URL("../../assets/sounds/hint_used.mp3", import.meta.url).toString()
  },
  music: [
    new URL("../../assets/music/track1.mp3", import.meta.url).toString(),
    new URL("../../assets/music/track2.mp3", import.meta.url).toString(),
    new URL("../../assets/music/track3.mp3", import.meta.url).toString(),
    new URL("../../assets/music/track4.mp3", import.meta.url).toString(),
    new URL("../../assets/music/track5.mp3", import.meta.url).toString()
  ],
  voice: {
    wordFound: Array.from({ length: 15 }, (_, i) =>
      new URL(`../../assets/voice/word_found_${String(i + 1).padStart(2, "0")}.wav`, import.meta.url).toString()
    ),
    rareWord: Array.from({ length: 8 }, (_, i) =>
      new URL(`../../assets/voice/rare_word_${String(i + 1).padStart(2, "0")}.wav`, import.meta.url).toString()
    ),
    puzzleSolved: Array.from({ length: 10 }, (_, i) =>
      new URL(`../../assets/voice/puzzle_solved_${String(i + 1).padStart(2, "0")}.wav`, import.meta.url).toString()
    ),
    sessionComplete: Array.from({ length: 10 }, (_, i) =>
      new URL(`../../assets/voice/session_complete_${String(i + 1).padStart(2, "0")}.wav`, import.meta.url).toString()
    ),
  },
  wordsCsv: new URL("../../words_processed.csv", import.meta.url).toString()
};

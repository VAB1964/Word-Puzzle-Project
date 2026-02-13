export const Assets = {
  menuBackground: new URL("../../assets/MenuBackground.png", import.meta.url).toString(),
  menuButton: new URL("../../assets/MenuButton.png", import.meta.url).toString(),
  mainBackground: new URL("../../assets/BackgroundandFrame.png", import.meta.url).toString(),
  hintFrame: new URL("../../assets/HintButtonFrame.png", import.meta.url).toString(),
  hintIndicator: new URL("../../assets/LightOn_small.png", import.meta.url).toString(),
  scrambleButton: new URL("../../assets/ScrambleButton.png", import.meta.url).toString(),
  gridButton: new URL("../../assets/Button.png", import.meta.url).toString(),
  circularLetterFrame: new URL("../../assets/CircularLetterFrame.png", import.meta.url).toString(),
  sapphire: new URL("../../assets/emerald.png", import.meta.url).toString(),
  ruby: new URL("../../assets/ruby.png", import.meta.url).toString(),
  diamond: new URL("../../assets/diamond.png", import.meta.url).toString(),
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
  wordsCsv: new URL("../../words_processed.csv", import.meta.url).toString()
};

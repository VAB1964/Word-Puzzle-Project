import type { Color } from "./types";

export interface ColorTheme {
  winBg: Color;
  decorBase: Color;
  decorAccent1: Color;
  decorAccent2: Color;
  gridEmptyTile: Color;
  gridFilledTile: Color;
  gridLetter: Color;
  wheelBg: Color;
  wheelOutline: Color;
  letterCircleNormal: Color;
  letterCircleHighlight: Color;
  letterTextNormal: Color;
  letterTextHighlight: Color;
  dragLine: Color;
  continueButton: Color;
  hudTextGuess: Color;
  hudTextFound: Color;
  hudTextSolved: Color;
  solvedOverlayBg: Color;
  scoreBarBg: Color;
  scoreTextLabel: Color;
  scoreTextValue: Color;
  menuBg: Color;
  menuTitleText: Color;
  menuButtonNormal: Color;
  menuButtonHover: Color;
  menuButtonText: Color;
}

export const adjustColorBrightness = (color: Color, factor: number): Color => {
  const safe = Math.max(0, factor);
  return {
    r: Math.min(255, Math.round(color.r * safe)),
    g: Math.min(255, Math.round(color.g * safe)),
    b: Math.min(255, Math.round(color.b * safe)),
    a: color.a ?? 255
  };
};

const c = (r: number, g: number, b: number, a = 255): Color => ({ r, g, b, a });

export const loadThemes = (): ColorTheme[] => {
  const themes: ColorTheme[] = [];

  const theme1: ColorTheme = {
    winBg: c(30, 37, 51),
    decorBase: c(45, 52, 67),
    decorAccent1: c(208, 232, 242),
    decorAccent2: c(155, 211, 255),
    gridEmptyTile: c(100, 110, 120, 180),
    gridFilledTile: c(155, 211, 255),
    gridLetter: c(255, 190, 70),
    wheelBg: c(45, 52, 67, 150),
    wheelOutline: c(155, 211, 255),
    letterCircleNormal: c(240, 240, 240, 220),
    letterCircleHighlight: c(34, 161, 57, 230),
    letterTextNormal: c(30, 37, 51),
    letterTextHighlight: c(255, 255, 255),
    dragLine: c(0, 105, 255, 200),
    continueButton: c(100, 200, 100),
    hudTextGuess: c(155, 211, 255),
    hudTextFound: c(255, 100, 0),
    hudTextSolved: c(155, 211, 255),
    solvedOverlayBg: c(40, 50, 70, 210),
    scoreBarBg: c(45, 52, 67, 230),
    scoreTextLabel: c(255, 190, 70),
    scoreTextValue: c(255, 190, 70),
    menuBg: c(40, 50, 70, 210),
    menuTitleText: c(208, 232, 242),
    menuButtonNormal: c(80, 90, 110),
    menuButtonHover: c(120, 135, 150),
    menuButtonText: c(255, 255, 255)
  };
  themes.push(theme1);

  const theme2: ColorTheme = {
    winBg: c(46, 34, 53),
    decorBase: c(69, 48, 78),
    decorAccent1: c(236, 150, 127),
    decorAccent2: c(255, 199, 138),
    gridEmptyTile: c(100, 80, 90, 180),
    gridFilledTile: c(217, 95, 67),
    gridLetter: c(255, 255, 255),
    wheelBg: c(69, 48, 78, 150),
    wheelOutline: c(236, 150, 127),
    letterCircleNormal: c(255, 255, 255, 220),
    letterCircleHighlight: c(217, 95, 67, 230),
    letterTextNormal: c(46, 34, 53),
    letterTextHighlight: c(255, 255, 255),
    dragLine: c(247, 156, 41, 200),
    continueButton: c(217, 95, 67),
    hudTextGuess: c(255, 199, 138),
    hudTextFound: c(255, 199, 138),
    hudTextSolved: c(247, 156, 41),
    solvedOverlayBg: c(69, 48, 78, 220),
    scoreBarBg: c(69, 48, 78, 230),
    scoreTextLabel: c(255, 190, 70),
    scoreTextValue: c(255, 190, 70),
    menuBg: c(69, 48, 78, 210),
    menuTitleText: c(236, 150, 127),
    menuButtonNormal: c(100, 80, 90),
    menuButtonHover: c(130, 110, 120),
    menuButtonText: c(255, 255, 255)
  };
  themes.push(theme2);

  const theme3: ColorTheme = {
    winBg: c(61, 44, 33),
    decorBase: c(94, 68, 54),
    decorAccent1: c(245, 235, 220),
    decorAccent2: c(218, 145, 70),
    gridEmptyTile: c(188, 169, 147, 180),
    gridFilledTile: c(218, 145, 70),
    gridLetter: c(245, 235, 220),
    wheelBg: c(138, 104, 73, 150),
    wheelOutline: c(245, 235, 220),
    letterCircleNormal: c(245, 235, 220, 220),
    letterCircleHighlight: c(218, 145, 70, 230),
    letterTextNormal: c(61, 44, 33),
    letterTextHighlight: c(61, 44, 33),
    dragLine: c(245, 235, 220, 200),
    continueButton: c(218, 145, 70),
    hudTextGuess: c(245, 235, 220),
    hudTextFound: c(245, 235, 220),
    hudTextSolved: c(245, 235, 220),
    solvedOverlayBg: c(94, 68, 54, 210),
    scoreBarBg: c(94, 68, 54, 230),
    scoreTextLabel: c(255, 190, 70),
    scoreTextValue: c(255, 190, 70),
    menuBg: c(94, 68, 54, 210),
    menuTitleText: c(245, 235, 220),
    menuButtonNormal: c(138, 104, 73),
    menuButtonHover: c(168, 148, 121),
    menuButtonText: c(245, 235, 220)
  };
  themes.push(theme3);

  const theme5: ColorTheme = {
    winBg: c(25, 20, 35),
    decorBase: c(40, 35, 50),
    decorAccent1: c(0, 255, 255),
    decorAccent2: c(255, 0, 255),
    gridEmptyTile: c(60, 60, 70, 180),
    gridFilledTile: c(255, 0, 255),
    gridLetter: c(255, 255, 255),
    wheelBg: c(50, 50, 60, 150),
    wheelOutline: c(0, 255, 255),
    letterCircleNormal: c(70, 60, 80, 220),
    letterCircleHighlight: c(0, 255, 255, 230),
    letterTextNormal: c(255, 255, 0),
    letterTextHighlight: c(25, 20, 35),
    dragLine: c(255, 255, 0, 200),
    continueButton: c(255, 0, 255),
    hudTextGuess: c(0, 255, 255),
    hudTextFound: c(0, 255, 255),
    hudTextSolved: c(255, 255, 0),
    solvedOverlayBg: c(40, 35, 50, 220),
    scoreBarBg: c(40, 35, 50, 230),
    scoreTextLabel: c(255, 190, 70),
    scoreTextValue: c(255, 190, 70),
    menuBg: c(40, 35, 50, 210),
    menuTitleText: c(0, 255, 255),
    menuButtonNormal: c(70, 60, 80),
    menuButtonHover: c(100, 90, 110),
    menuButtonText: c(255, 255, 0)
  };
  themes.push(theme5);

  const theme6: ColorTheme = {
    winBg: c(40, 40, 40),
    decorBase: c(70, 70, 70),
    decorAccent1: c(150, 150, 150),
    decorAccent2: c(220, 220, 220),
    gridEmptyTile: c(100, 100, 100, 180),
    gridFilledTile: c(180, 180, 180),
    gridLetter: c(255, 255, 255),
    wheelBg: c(60, 60, 60, 150),
    wheelOutline: c(180, 180, 180),
    letterCircleNormal: c(190, 190, 190, 220),
    letterCircleHighlight: c(100, 150, 200, 230),
    letterTextNormal: c(0, 0, 0),
    letterTextHighlight: c(255, 255, 255),
    dragLine: c(100, 150, 200, 200),
    continueButton: c(100, 150, 200),
    hudTextGuess: c(255, 255, 255),
    hudTextFound: c(255, 255, 255),
    hudTextSolved: c(100, 150, 200),
    solvedOverlayBg: c(70, 70, 70, 210),
    scoreBarBg: c(70, 70, 70, 230),
    scoreTextLabel: c(255, 190, 70),
    scoreTextValue: c(255, 190, 70),
    menuBg: c(70, 70, 70, 210),
    menuTitleText: c(220, 220, 220),
    menuButtonNormal: c(100, 100, 100),
    menuButtonHover: c(130, 130, 130),
    menuButtonText: c(255, 255, 255)
  };
  themes.push(theme6);

  const theme7: ColorTheme = {
    winBg: c(105, 184, 110),
    decorBase: c(245, 247, 22),
    decorAccent1: c(255, 190, 200, 230),
    decorAccent2: c(255, 255, 200),
    gridEmptyTile: c(190, 205, 195, 180),
    gridFilledTile: c(90, 170, 100),
    gridLetter: c(190, 205, 195),
    wheelBg: c(180, 220, 190, 150),
    wheelOutline: c(110, 160, 120),
    letterCircleNormal: c(240, 255, 245, 220),
    letterCircleHighlight: c(255, 190, 200, 230),
    letterTextNormal: c(40, 70, 45),
    letterTextHighlight: c(40, 70, 45),
    dragLine: c(80, 180, 90, 200),
    continueButton: c(90, 170, 100),
    hudTextGuess: c(50, 100, 55),
    hudTextFound: c(50, 100, 55),
    hudTextSolved: c(0, 180, 0),
    solvedOverlayBg: c(180, 220, 190, 210),
    scoreBarBg: c(170, 210, 180, 230),
    scoreTextLabel: c(255, 190, 70),
    scoreTextValue: c(255, 190, 70),
    menuBg: c(180, 220, 190, 220),
    menuTitleText: c(40, 70, 45),
    menuButtonNormal: c(140, 190, 150),
    menuButtonHover: c(110, 160, 120),
    menuButtonText: c(40, 70, 45)
  };
  themes.push(theme7);

  const theme8: ColorTheme = {
    winBg: c(15, 25, 45),
    decorBase: c(20, 35, 60),
    decorAccent1: c(35, 55, 85),
    decorAccent2: c(0, 130, 140),
    gridEmptyTile: c(50, 60, 80, 180),
    gridFilledTile: c(0, 180, 170),
    gridLetter: c(210, 245, 255),
    wheelBg: c(40, 60, 90, 150),
    wheelOutline: c(80, 120, 180),
    letterCircleNormal: c(190, 210, 225, 220),
    letterCircleHighlight: c(0, 180, 170, 230),
    letterTextNormal: c(15, 25, 45),
    letterTextHighlight: c(15, 25, 45),
    dragLine: c(0, 200, 190, 200),
    continueButton: c(0, 150, 130),
    hudTextGuess: c(180, 230, 240),
    hudTextFound: c(180, 230, 240),
    hudTextSolved: c(50, 220, 210),
    solvedOverlayBg: c(25, 40, 65, 210),
    scoreBarBg: c(35, 55, 85, 230),
    scoreTextLabel: c(255, 190, 70),
    scoreTextValue: c(255, 190, 70),
    menuBg: c(25, 40, 65, 220),
    menuTitleText: c(210, 245, 255),
    menuButtonNormal: c(50, 80, 120),
    menuButtonHover: c(80, 120, 180),
    menuButtonText: c(210, 245, 255)
  };
  themes.push(theme8);

  const theme9: ColorTheme = {
    winBg: c(78, 156, 199),
    decorBase: c(225, 225, 215),
    decorAccent1: c(205, 205, 195),
    decorAccent2: c(185, 185, 175),
    gridEmptyTile: c(154, 199, 198, 180),
    gridFilledTile: c(255, 255, 255),
    gridLetter: c(0, 0, 0),
    wheelBg: c(215, 215, 205, 150),
    wheelOutline: c(140, 140, 130),
    letterCircleNormal: c(255, 255, 255, 220),
    letterCircleHighlight: c(190, 210, 240, 230),
    letterTextNormal: c(0, 0, 0),
    letterTextHighlight: c(0, 0, 0),
    dragLine: c(80, 80, 80, 200),
    continueButton: c(90, 90, 90),
    hudTextGuess: c(181, 179, 152),
    hudTextFound: c(40, 40, 40),
    hudTextSolved: c(0, 100, 0),
    solvedOverlayBg: c(215, 215, 205, 210),
    scoreBarBg: c(205, 205, 195, 230),
    scoreTextLabel: c(255, 190, 70),
    scoreTextValue: c(255, 190, 70),
    menuBg: c(215, 215, 205, 220),
    menuTitleText: c(20, 20, 20),
    menuButtonNormal: c(170, 170, 160),
    menuButtonHover: c(140, 140, 130),
    menuButtonText: c(20, 20, 20)
  };
  themes.push(theme9);

  const theme10: ColorTheme = {
    winBg: c(55, 30, 15),
    decorBase: c(85, 50, 25),
    decorAccent1: c(125, 70, 35),
    decorAccent2: c(190, 80, 30),
    gridEmptyTile: c(100, 70, 45, 180),
    gridFilledTile: c(200, 100, 30),
    gridLetter: c(255, 245, 210),
    wheelBg: c(75, 45, 20, 150),
    wheelOutline: c(140, 90, 50),
    letterCircleNormal: c(240, 230, 200, 220),
    letterCircleHighlight: c(230, 130, 40, 230),
    letterTextNormal: c(55, 30, 15),
    letterTextHighlight: c(55, 30, 15),
    dragLine: c(230, 130, 40, 200),
    continueButton: c(180, 90, 25),
    hudTextGuess: c(255, 225, 180),
    hudTextFound: c(255, 225, 180),
    hudTextSolved: c(255, 150, 0),
    solvedOverlayBg: c(85, 50, 25, 210),
    scoreBarBg: c(85, 50, 25, 230),
    scoreTextLabel: c(255, 190, 70),
    scoreTextValue: c(255, 190, 70),
    menuBg: c(85, 50, 25, 220),
    menuTitleText: c(255, 245, 210),
    menuButtonNormal: c(125, 70, 35),
    menuButtonHover: c(155, 90, 45),
    menuButtonText: c(255, 245, 210)
  };
  themes.push(theme10);

  return themes;
};

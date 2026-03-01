import type { Color, Rect } from "./types";

export const REF_W = 1000;
export const REF_H = 800;

export const GUESS_DISPLAY_GAP = 40.0;
export const GUESS_DISPLAY_OFFSET_Y = 20.0;
export const GUESS_TILE_SCALE = 1.25;
export const GUESS_LETTER_FONT_SCALE = 0.65;
export const HINT_BTN_OFFSET_X = 10;
export const HINT_BTN_OFFSET_Y = 5;
export const TILE_SIZE = 40.0;
export const TILE_PAD = 5.0;
export const GRID_TOP_MARGIN = 15.0;
export const GRID_WHEEL_GAP = 75.0;
export const COL_PAD = 10.0;
export const GRID_COLUMN_DIVIDER_WIDTH = 4.0;
export const GRID_COLUMN_DIVIDER_ROWS = 5;
export const WHEEL_BOTTOM_MARGIN = 70.0;

export const HUD_TEXT_OFFSET_Y = 15.0;
export const HUD_LINE_SPACING = 20.0;
export const HUD_AREA_MIN_HEIGHT = 25.0;
export const UI_SCALE_MODIFIER = 0.75;
export const GRID_TILE_RELATIVE_SCALE_WHEN_SHRUNK = 0.95;

export const SCRAMBLE_BTN_OFFSET_X = 80.0;
export const SCRAMBLE_BTN_OFFSET_Y = 125.0;
export const CONTINUE_BTN_OFFSET_Y = 80.0;
export const WORD_LENGTH = 5.0;
export const GRID_SIDE_MARGIN = 5.0;
export const MIN_WINDOW_HEIGHT = 800.0;
export const MIN_WINDOW_WIDTH = 1000.0;
export const SCORE_BAR_WIDTH = 300.0;
export const SCORE_BAR_HEIGHT = 30.0;
export const SCORE_BAR_TOP_MARGIN = 15.0;
export const SCRAMBLE_BTN_HEIGHT = 40.0;
export const HINT_BTN_HEIGHT = SCRAMBLE_BTN_HEIGHT;
export const MIN_WORD_LENGTH = 3;
export const HARD_MIN_WORD_LENGTH = 4;

export const EASY_MAX_SOLUTIONS = 7;
export const MEDIUM_MAX_SOLUTIONS = 12;
export const HARD_MAX_SOLUTIONS = 15;
export const MIN_DESIRED_GRID_WORDS = 5;

export const HINT_COST_REVEAL_FIRST = 2;
export const HINT_COST_REVEAL_RANDOM = 3;
export const HINT_COST_REVEAL_LAST = 5;
export const HINT_COST_REVEAL_FIRST_OF_EACH = 7;

export const WORDS_PER_HINT = 5;

export const SCORE_BAR_BOTTOM_MARGIN = 10.0;
export const METER_SCORE_GAP = 8.0;

export const PROGRESS_METER_WIDTH = 300.0;
export const PROGRESS_METER_HEIGHT = 20.0;
export const PROGRESS_METER_TOP_MARGIN = 15.0;
export const PROGRESS_METER_OUTLINE = 2.0;
export const PROGRESS_METER_HEIGHT_DESIGN = 10.0;

export const RETURN_BTN_WIDTH_DESIGN = 120.0;
export const RETURN_BTN_HEIGHT_DESIGN = 35.0;
export const TOP_BAR_PADDING_X_DESIGN = 20.0;
export const RETURN_BTN_FONT_SIZE_DESIGN = 16;

export const MENU_BUTTON_WIDTH_DESIGN = 250.0;
export const MENU_BUTTON_HEIGHT_DESIGN = 75.0;
export const MENU_PANEL_PADDING_DESIGN = 70.0;
export const MENU_BUTTON_SPACING_DESIGN = 10.0;
export const MENU_PANEL_EXTRA_WIDTH_DESIGN = 0.0;
export const MENU_PANEL_EXTRA_HEIGHT_DESIGN = 0.0;

export const EASY_PUZZLE_COUNT = 5;
export const MEDIUM_PUZZLE_COUNT = 7;
export const HARD_PUZZLE_COUNT = 10;

export const SCORE_FLOURISH_FONT_SIZE_BASE_DESIGN = 20.0;
export const SCORE_FLOURISH_LIFETIME_MIN_SEC = 1.8;
export const SCORE_FLOURISH_LIFETIME_MAX_SEC = 2.8;
export const SCORE_FLOURISH_VEL_Y_MIN_DESIGN = -50.0;
export const SCORE_FLOURISH_VEL_Y_MAX_DESIGN = -90.0;
export const SCORE_FLOURISH_VEL_X_RANGE_DESIGN = 20.0;
export const SCORE_FLOURISH_DURATION = 0.4;
export const SCORE_FLOURISH_SCALE = 1.3;
export const HINT_POINT_ANIM_FONT_SIZE_DESIGN = 20.0;
export const HINT_POINT_TEXT_FLOURISH_DURATION = 0.5;
export const HINT_POINT_ANIM_SPEED = 1.0;

export const HINT_POPUP_WIDTH_DESIGN = 300.0;
export const HINT_POPUP_HEIGHT_DESIGN = 180.0;
export const HINT_POPUP_PADDING_DESIGN = 40.0;
export const HINT_POPUP_LINE_SPACING_DESIGN = 7.0;

export const WORD_INFO_POPUP_MAX_WIDTH_DESIGN = 420.0;
export const WORD_INFO_POPUP_PADDING_DESIGN = 50.0;
export const WORD_INFO_POPUP_LINE_SPACING_DESIGN = 6.0;
export const WORD_INFO_POPUP_OFFSET_FROM_MOUSE_DESIGN = 12.0;

export const POPUP_SCREEN_MARGIN_DESIGN = 5.0;

export const POPUP_PADDING_BASE = 50.0;
export const MAJOR_COL_SPACING_BASE = 10.0;
export const MINOR_COL_SPACING_BASE = 8.0;
export const TITLE_BOTTOM_MARGIN_BASE = 6.0;
export const WORD_LINE_SPACING_BASE = 5.0;
export const POPUP_WORD_FONT_SIZE_BASE = 22;
export const POPUP_TITLE_FONT_SIZE_BASE = 26;
export const POPUP_MIN_TEXT_SCALE = 0.7;
export const POPUP_MAX_TEXT_SCALE = 1.35;
export const POPUP_CORNER_RADIUS_BASE = 10.0;
export const MAX_MINOR_COLS_PER_GROUP = 3;

export const POPUP_MAX_WIDTH_DESIGN_RATIO = 0.95;
export const POPUP_MAX_HEIGHT_DESIGN_RATIO = 0.95;
export const BONUS_POPUP_SCROLL_SPEED = 35.0;
export const POPUP_SCROLL_TOP_BUFFER = 0.0;
export const POPUP_SCROLL_BOTTOM_BUFFER = 20.0;

export const GRID_ZONE_RECT_DESIGN: Rect = { x: 90.0, y: 122.0, width: 815.0, height: 273.0 };
export const HINT_ZONE_RECT_DESIGN: Rect = { x: 110.0, y: 475.0, width: 245.0, height: 245.0 };
export const WHEEL_ZONE_RECT_DESIGN: Rect = { x: 374.0, y: 450.0, width: 285.0, height: 285.0 };
export const SCORE_ZONE_RECT_DESIGN: Rect = { x: 660.0, y: 500.0, width: 243.0, height: 195.0 };
export const TOP_BAR_ZONE_DESIGN: Rect = { x: 50.0, y: 10.0, width: 900.0, height: 30.0 };

export const HINT_BG_PADDING_X = 5.0;
export const HINT_BG_PADDING_Y = 2.0;
export const GRID_ZONE_PADDING_X_DESIGN = 2.0;
export const GRID_ZONE_PADDING_Y_DESIGN = 2.0;
export const WHEEL_ZONE_PADDING_DESIGN = 10.0;
export const SCALED_WHEEL_PADDING_DESIGN = 15.0;
export const LETTER_VISUAL_GAP_DESIGN = 15.0;
export const LETTER_INWARD_OFFSET_DESIGN = 2.0;
export const WHEEL_BG_EXTRA_PADDING_DESIGN = 5.0;
export const WHEEL_BG_PADDING_AROUND_LETTERS_DESIGN = 3.0;

export const LETTER_R = 15.0;
export const LETTER_R_BASE_DESIGN = 18.0;
export const MIN_LETTER_RADIUS_FACTOR = 0.08;
export const MAX_LETTER_RADIUS_FACTOR = 0.2;
export const WHEEL_LETTER_FONT_SIZE_BASE_DESIGN = 16.0;
export const WHEEL_LETTER_VISUAL_SCALE = 1.8;
export const WHEEL_TOUCH_SCALE_FACTOR = 1.4;
export const WHEEL_HIT_RADIUS_NON_SCALED_EXTRA = 8.0;
export const DEBUG_DRAW_WHEEL_HIT_AREAS = true;
export const WHEEL_R = 68.0;

export const SCORE_ZONE_PADDING_X_DESIGN = 15.0;
export const SCORE_ZONE_PADDING_Y_DESIGN = 26.0;
export const SCORE_LABEL_VALUE_GAP_DESIGN = 25.0;
export const SCORE_VALUE_BONUS_GAP_DESIGN = 10.0;

export const PUZZLE_SOLVED_TITLE_FONT_SIZE_DESIGN = 18.0;

export const SCORE_ZONE_LABEL_FONT_SIZE = 18;
export const SCORE_ZONE_VALUE_FONT_SIZE = 28;
export const SCORE_ZONE_BONUS_FONT_SIZE = 10;

export const GLOWING_TUBE_TEXT_COLOR: Color = { r: 255, g: 190, b: 70, a: 255 };

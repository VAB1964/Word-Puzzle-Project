# Cozy Library implementation handoff

## Request and approved design
Vince approved the Cozy Library mockup and asked to apply it while preserving all coded/gameplay elements. The implementation updates the web game in this repository, covering Casual and Crossword, menus, hints, score, gems, wheel, and popups. The desktop C++/SFML application has not been restyled or rebuilt. No production deployment or merge has been performed.

## Implemented
- New AI-generated walnut library desktop with books, lamp, and leaves confined to the perimeter (`assets/cozy-library/desktop.png`).
- New scalable parchment panel and emerald/ruby/diamond SVG assets in the same directory.
- Canvas-drawn cream tiles with dark lettering, sage solved states, selected wheel letters, matching buttons, and a green selection path.
- Existing theme mechanism now selects the library palette consistently between puzzles. Legacy definitions remain for later reuse.
- Existing four hint actions, exact costs (2/3/5/7), affordability indicators, bonus-word access, score, session progress, and voice toggle retained. Costs are visible on buttons.
- Larger score with thousands separators, explicit puzzle progress text, menu/popups recolored for parchment contrast.
- Disabled pre-existing neon wheel debug outlines. Guess preview fits between the board and expanded touch wheel. Scramble moved below expanded wheel; its existing input rectangle follows it.
- VAB navigation link moved to the bottom-left to avoid the game Menu button.
- Old artwork files remain for the native application; the web entry no longer loads obsolete UI textures.

## Validation completed
- `cd web && npx tsc --noEmit`: passed.
- `cd web && npm run build`: passed after final source edits.
- Offscreen rendering of the actual bundled Game class using the existing word database and @napi-rs/canvas: menus, Casual, Crossword (including a 20-word hard puzzle), expanded touch wheel, rules, hint tooltip, bonus list, solved popup, session summary.
- Direct calls to the real input/animation handlers passed assertions for: unaffordable hints, targeted letter hint and cost, random hint cost, scramble preserving the letters, word scoring, synchronization of crossword shared cells, Menu/Resume, Continue advancing the session.
- Rendering and logic checks used an offscreen canvas, NOT a browser. Native browser input/audio and a physical phone/tablet have not been tested.

## Remaining review / next steps
1. Review the actual game in a browser at desktop and phone/tablet sizes. Specifically inspect the guess preview while the touch wheel is enlarged, the bottom Scramble label and VAB link, large crosswords, and long bonus/definition popups.
2. Test actual pointer/keyboard/touch entry and audio in-browser. No gameplay rules or scoring formulas were intentionally changed.
3. Let Vince try the branch locally, or merge/deploy when authorized. Hosting is the existing VABGames/Cloudflare workflow; do not create a replacement Site or switch providers.
4. If Vince intended the native C++ executable as well, that is a separate remaining implementation; do not claim it was updated.

## Asset source
The desktop image was generated with the built-in image generation tool for this project. Brief: orthographic walnut library desk, central 86% clear for parchment UI, cropped antique books in left corners, brass lamp upper-right, restrained leaves, warm neutral brown, no text or UI. It is saved in the repository and needs no external service at runtime. Panels and gems are authored vector geometry; letters and scores remain code-rendered.

## Local test (from repository root)
```sh
git fetch origin
git switch codex/cozy-library-style
cd web
npm ci
npm run dev
```
Open the local URL printed by Vite, using `/wordpuzzle/` if necessary. The original scripts, dependency versions, word data, and hosting configuration are preserved.

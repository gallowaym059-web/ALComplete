# Changelog

## v1.0.0 - ALComplete
Completely new mod (formerly "AI Pathfinder").

- New idea: solve the whole level ahead of time, then autoplay it. No botting first.
- Solver steps the real game with checkpoints and backtracks from deaths, so orbs, pads,
  portals, triggers and moving objects are all handled by GD itself.
- Route styles: Simplest (fewest inputs) or Safest (look-ahead picks the longer-surviving input).
- Replay verification of the found path before it is played.
- New floating, draggable, snapping circle button with changeable icon.
- New animated, categorised menu (Eclipse / QOLmod style) with opacity, scale, accent colour.
- Visible above the pause menu.
- Mod ID is now `bedrock.alcomplete`.


## v1.1.0 — solver timing and validation
- Replaced level-progress-based replay timing with a logical physics tick counter.
- Reset the logical tick clock on level reset and restore it when loading a saved checkpoint.
- Restore the prior test-mode flag during solver cleanup instead of leaving it changed until a later reset.
- Added a repeatable structural validation script and wired it into CI before compilation.
- Updated project documentation to state known limitations and distinguish static checks from actual in-game testing.

## v1.1.1 - Experimental Ship prediction

- Ship route selection now runs ghost hold/release simulations even when Simplest route mode is selected.
- Extended Ship look-ahead to at least 360 physics ticks to help avoid late reactions near ceilings and spikes.
- Added structural validation for the Ship prediction path.
- This is a heuristic improvement; Geode compilation and in-game accuracy still require testing.


## v1.1.2
- Added predictive route choice for Wave in all route modes, including Simplest.
- Wave checks a minimum 240-tick lookahead; Ship keeps its 360-tick minimum.
- Static validation updated; compilation and in-game tests remain outstanding.


## v1.1.3
- Generalized predictive input selection to Cube, Ship, Ball, UFO, Wave, Robot, Spider, and Swing.
- Added mode-aware minimum lookahead horizons; Simplest route mode now checks whether the default input is safe instead of skipping prediction for non-Ship/Wave modes.
- This is still a heuristic using the existing ghost simulation. Physics parity, mode transitions, dual mode, compilation, and in-game behavior require testing.

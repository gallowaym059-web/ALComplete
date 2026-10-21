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


## v1.1.4 — classic-mode stability and UI fitting
- Predictive choice now requires a small survival advantage before switching away from the default input, reducing noisy input flips caused by approximate ghost predictions. The checkpoint/backtracking search remains enabled.
- Setting names are scaled to fit their column; changing slider/cycle values are clamped to their value column. Help/info text keeps a fixed-width label to wrap within the panel.
- These are heuristic/UI safeguards, not proof that every mode's physics is fully modeled. Geode compilation and in-game validation are still required.


## v1.1.5
- Disable approximate ghost-based route choice for Wave; rely on real-game checkpoint search to avoid false prediction flips.
- Increase checkpoint density for non-Cube modes to provide more nearby alternatives after deaths.
- Cube retains its existing checkpoint cadence.
- Faster/denser backtracking is a heuristic change; performance and behavior require in-game testing.


## v1.1.6 — frame-step-accurate search
- Solver search, checkpoint replay, and solution verification now advance exactly one 1/240-second physics tick per step, regardless of the old Ticks per step preference.
- Corrected the timestep calculation to remove the previous extra half-tick offset.
- Create a backtracking decision point every physics tick so single-frame-perfect input windows are not skipped by multi-tick advances.
- Removed the obsolete Ticks per step control from the settings menu; the saved field is retained for compatibility.
- This improves temporal granularity, but does not guarantee every level can be solved: checkpoint completeness, dual-mode/trigger determinism, and game-hook behavior still need in-game validation.

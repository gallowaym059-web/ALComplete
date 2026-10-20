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

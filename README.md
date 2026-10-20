# ALComplete

**ALComplete** means **Auto Levels Complete**. This is an experimental Geode mod for Geometry Dash 2.2081. It searches for a survivable sequence of player inputs by stepping the real game, backtracking after deaths, optionally verifying the found route, and replaying it.

> **Status: experimental.** This is not a universal AI and cannot guarantee completion of every level. Platformer levels are currently unsupported. Complex triggers, dual sections, moving objects, and physics differences can cause a search or replay to fail. A successful static check is not proof of in-game correctness.

## Features currently present in the source

- Draggable floating circular launcher and categorized in-game panel.
- Solver controls for Solve, Solve + Play, Play, and Stop.
- Appearance controls for menu opacity/scale, button size/opacity, accent colour, and button icon.
- Solver route options, replay verification, time budget, checkpoint limit, and progress HUD.
- Optional safe mode intended to mark assisted runs as test runs.
- Android ARM64 and Windows build workflow for Geometry Dash 2.2081.

## Build

The mod ID is `bedrock.alcomplete`. Geode mod IDs do not include the `.geode` filename extension; the built package will have that extension.

The easiest path is to push this repository to GitHub and run **Actions → Build ALComplete**. The workflow first runs `scripts/validate_project.py`, then invokes the Geode build action for Android64 and Windows. Downloadable artifacts are only available if the build action completes successfully.

For a local build, install a compatible Geode SDK and set `GEODE_SDK` to its path before configuring CMake.

## Validation

Run:

```sh
python scripts/validate_project.py
```

This checks metadata, required files, UI controls, and important solver-clock invariants. It does **not** compile the C++ or test the mod inside Geometry Dash. Those require the Geode SDK and actual in-game testing.

## Important implementation note

Replay timing must use a deterministic logical physics-tick counter, not the level's displayed progress percentage. ALComplete now tracks logical ticks across simulation steps, resets, and checkpoint restores. This fixes a fundamental replay-clock issue, but additional solver correctness work and gameplay testing are still required.


### v1.1.2 prediction changes
- Wave now uses predictive hold/release comparisons even when Simplest route mode is selected.
- Wave prediction uses a minimum 240-physics-tick lookahead; Ship retains its 360-tick minimum.
- This is a heuristic, not a complete physics proof. Ghost simulation may diverge from live gameplay, and actual Android build/gameplay testing is still required.


### v1.1.3 classic-mode prediction
Predictive input comparison now runs in all supported classic modes: Cube, Ship, Ball, UFO, Wave, Robot, Spider, and Swing. Minimum lookahead horizons are 90 ticks for Cube, 120 for Ball/UFO/Robot/Spider, 180 for Swing, 240 for Wave, and 360 for Ship (the configured lookahead can be longer). These are heuristic starting points, not validated optimal values. The current ghost simulation may diverge from live gameplay, especially around mode portals, triggers, dual mode, and complex physics. Platformer remains unsupported.

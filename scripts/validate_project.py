#!/usr/bin/env python3
"""Fast structural checks for ALComplete; not a substitute for a Geode build or game test."""
import json
import re
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
errors = []

def require(condition, message):
    if not condition:
        errors.append(message)

try:
    meta = json.loads((root / "mod.json").read_text(encoding="utf-8"))
except Exception as exc:
    print(f"FAIL: mod.json could not be parsed: {exc}")
    sys.exit(1)

require(meta.get("id") == "bedrock.alcomplete", "unexpected mod ID")
require(meta.get("gd", {}).get("android") == "2.2081", "Android target must be GD 2.2081")
require(meta.get("gd", {}).get("win") == "2.2081", "Windows target must be GD 2.2081")
require(meta.get("geode") == "5.11.0", "metadata SDK version must be 5.11.0")

for path in ("CMakeLists.txt", "src/Solver.cpp", "src/Hooks.cpp", "src/Hub.cpp", ".github/workflows/build.yml"):
    require((root / path).is_file(), f"missing required file: {path}")

solver = (root / "src/Solver.cpp").read_text(encoding="utf-8")
require("uint32_t tick = 0;" in solver, "logical physics tick counter missing")
require("g.tick += static_cast<uint32_t>(std::max(0, ticks));" in solver,
        "logical physics tick counter is not advanced by simulated steps")
require("g.tick = 0;" in solver, "logical physics tick counter is not reset at level start")
require("g.tick = n.tick;" in solver, "logical physics tick counter is not restored with checkpoints")
require("m_gameState.m_currentProgress" not in solver,
        "solver is incorrectly using level progress as a replay clock")

hub = (root / "src/Hub.cpp").read_text(encoding="utf-8")
for feature in ("Solver", "Route", "Visuals", "Appearance", "Safety", "Diagnostics"):
    require(re.search(r'Cat\s+\w+\s*\{\s*"' + re.escape(feature) + r'"', hub) is not None,
            f"expected UI category not found: {feature}")
for feature in ("Menu opacity", "Menu scale", "Button size", "Button opacity", "Button icon"):
    require(feature in hub, f"expected appearance control missing: {feature}")

if errors:
    for error in errors:
        print(f"FAIL: {error}")
    sys.exit(1)
print("PASS: metadata, required files, replay-clock regression checks, and key UI controls")
print("NOTE: this is a structural check only; Geode compilation and in-game behavior remain unverified")

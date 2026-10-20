#!/usr/bin/env python3

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ERRORS = []


def fail(message):
    ERRORS.append(message)


def check_file(relative_path):
    path = ROOT / relative_path
    if not path.is_file():
        fail(f"Missing required file: {relative_path}")
        return None
    return path


def main():
    metadata_path = check_file("mod.json")
    check_file("CMakeLists.txt")
    check_file("src/Solver.cpp")
    check_file("src/Solver.hpp")
    check_file("src/Sim.cpp")
    check_file("src/Sim.hpp")
    check_file("src/Predict.cpp")
    check_file("src/Predict.hpp")

    if metadata_path:
        try:
            metadata = json.loads(
                metadata_path.read_text(encoding="utf-8")
            )
        except (json.JSONDecodeError, OSError) as exc:
            fail(f"Invalid mod.json: {exc}")
            metadata = {}

        if metadata.get("id") != "bedrock.alcomplete":
            fail("Unexpected mod ID")

        if "2.2081" not in str(metadata.get("gd", "")):
            fail("Expected Geometry Dash 2.2081 metadata")

        if not metadata.get("version"):
            fail("Missing mod version")

    solver_path = ROOT / "src/Solver.cpp"

    if solver_path.is_file():
        try:
            solver = solver_path.read_text(encoding="utf-8")
        except OSError as exc:
            fail(f"Cannot read Solver.cpp: {exc}")

#pragma once
#include <Geode/Geode.hpp>
#include <functional>
#include <string>

namespace al::solver {
    using StepFn = std::function<void(float)>;

    enum class Phase { Idle, Solving, Replaying, Verifying, Playing };

    struct Status {
        Phase phase = Phase::Idle;
        float percent = 0.f;
        float best = 0.f;
        uint64_t backtracks = 0;
        double elapsed = 0.0;
        bool hasSolution = false;
        size_t solutionInputs = 0;
        size_t solutionClicks = 0;
        std::string message;   // result of the last run
    };

    Status status();
    bool busy();                 // true while the solver owns the game update
    bool blocksInput();          // real taps are ignored while busy
    bool silent();               // solving/replaying/verifying: mute SFX and song triggers

    void requestSolve(bool autoplayAfter);
    void requestPlay();
    void requestStop();
    void clearSolution();

    // GJBaseGameLayer::update hook
    void onFrame(PlayLayer* pl, float dt, StepFn const& step);

    // Hook helpers. Return true when the hook must swallow the call.
    bool onDeath(GameObject* culprit, PlayLayer* pl);
    bool onLevelEnd();
    void onResetLevel(PlayLayer* pl);
    void onQuit();
}

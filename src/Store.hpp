#pragma once
#include <Geode/Geode.hpp>

namespace al {
    // Every user-facing option. Edited from the in-game menu, persisted with saved values.
    struct Store {
        // appearance
        float menuOpacity = 0.95f;
        float menuScale = 1.f;
        float buttonScale = 1.f;
        float buttonOpacity = 1.f;
        float glow = 1.f;
        bool snapEdge = true;
        bool animations = true;
        int accent = 0;
        int icon = 0;
        float hubX = 0.93f, hubY = 0.62f;       // button position, as screen fractions
        float panelX = 0.5f, panelY = 0.5f;     // menu position, as screen fractions

        // solver
        float budgetMs = 10.f;
        int ticksPerStep = 4;
        int maxCheckpoints = 400;
        int lookahead = 60;
        int giveUpSec = 0;
        int route = 0;                           // 0 = Simplest, 1 = Safest
        bool autoSolve = false;
        bool autoPlay = true;
        bool verify = true;

        // visuals
        bool showPath = true;
        bool showDeaths = true;
        bool showHud = true;
        bool showRing = true;

        // safety
        bool safeMode = true;
        bool restorePractice = true;

        // diagnostics
        bool verbose = false;

        void load();
        void save() const;
    };

    Store& store();
}

#pragma once
#include <Geode/Geode.hpp>

namespace al {
    // How many ticks the player survives if the jump button is held / released from now on.
    // Returns `ticks + 1` when it survives the whole window.
    int survival(GJBaseGameLayer* layer, bool currentlyHeld, bool hold, int ticks);
}

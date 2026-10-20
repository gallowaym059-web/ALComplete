#pragma once
#include <Geode/Geode.hpp>

namespace al {
    // Invisible fake players used only for the Safest-route look-ahead.
    struct Sim {
        PlayerObject* fake1 = nullptr;
        PlayerObject* fake2 = nullptr;
        bool active = false;   // true while a fake player is being stepped
        bool dead = false;     // set by the destroyPlayer hook

        void create(PlayLayer* pl);
        void destroy();
        bool owns(PlayerObject* p) const { return p && (p == fake1 || p == fake2); }
        static void clearLogs(PlayerObject* p);
    };

    Sim& sim();

    // Fake players only collide with solids, hazards, animated hazards and slopes, plus
    // speed/size portals (applied by hand). Everything else is switched off during look-ahead.
    bool collidable(GameObject* obj);
    void handlePortal(PlayerObject* p, int objectID);
}

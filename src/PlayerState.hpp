#pragma once
#include <Geode/Geode.hpp>
#include <type_traits>
#include "PlayerFields.inc"

namespace al {
    // Full physics snapshot of a PlayerObject (used by the Safest-route look-ahead).
    struct PlayerState {
#define AL_DECL(f) std::remove_cvref_t<decltype(PlayerObject::f)> f;
        AL_PLAYER_FIELDS(AL_DECL)
#undef AL_DECL
        cocos2d::CCPoint pos;
        float rot = 0.f;
        bool held = false;

        void save(PlayerObject* p, bool isHeld);
        void load(PlayerObject* p) const;
    };
}

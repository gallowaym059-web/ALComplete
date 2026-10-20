#include "PlayerState.hpp"

using namespace geode::prelude;

namespace al {
    void PlayerState::save(PlayerObject* p, bool isHeld) {
#define AL_SAVE(f) f = p->f;
        AL_PLAYER_FIELDS(AL_SAVE)
#undef AL_SAVE
        pos = p->getPosition();
        rot = p->getRotation();
        held = isHeld;
    }

    void PlayerState::load(PlayerObject* p) const {
#define AL_LOAD(f) p->f = f;
        AL_PLAYER_FIELDS(AL_LOAD)
#undef AL_LOAD
        p->setPosition(pos);
        p->setRotation(rot);
        p->m_holdingButtons[static_cast<int>(PlayerButton::Jump)] = held;
    }
}

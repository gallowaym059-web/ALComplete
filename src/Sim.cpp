#include "Sim.hpp"
#include <unordered_set>

using namespace geode::prelude;

namespace al {
    Sim& sim() { static Sim s; return s; }

    static PlayerObject* makeFake(PlayLayer* pl) {
        auto p = PlayerObject::create(1, 1, pl, pl, true);
        p->retain();
        p->setPosition({0, 105});
        p->setVisible(false);
        p->m_playEffects = false;
        pl->m_objectLayer->addChild(p);
        return p;
    }

    void Sim::create(PlayLayer* pl) {
        destroy();
        fake1 = makeFake(pl);
        fake2 = makeFake(pl);
    }

    void Sim::destroy() {
        for (auto* p : {fake1, fake2}) {
            if (!p) continue;
            p->removeFromParent();
            p->release();
        }
        fake1 = fake2 = nullptr;
        active = dead = false;
    }

    void Sim::clearLogs(PlayerObject* p) {
        if (p->m_collisionLogTop) p->m_collisionLogTop->removeAllObjects();
        if (p->m_collisionLogBottom) p->m_collisionLogBottom->removeAllObjects();
        if (p->m_collisionLogLeft) p->m_collisionLogLeft->removeAllObjects();
        if (p->m_collisionLogRight) p->m_collisionLogRight->removeAllObjects();
    }

    namespace {
        const std::unordered_set<int> kBaseTypes = {0, 2, 25, 47};   // solid, hazard, slope, animated hazard
        const std::unordered_set<int> kPortalIDs = {101, 99, 200, 201, 202, 203, 1334};
    }

    bool collidable(GameObject* obj) {
        return kBaseTypes.contains(static_cast<int>(obj->m_objectType)) || kPortalIDs.contains(obj->m_objectID);
    }

    void handlePortal(PlayerObject* p, int id) {
        switch (id) {
            case 101: p->togglePlayerScale(true, true);  p->updatePlayerScale(); break;
            case 99:  p->togglePlayerScale(false, true); p->updatePlayerScale(); break;
            case 200: p->m_playerSpeed = 0.7f; break;
            case 201: p->m_playerSpeed = 0.9f; break;
            case 202: p->m_playerSpeed = 1.1f; break;
            case 203: p->m_playerSpeed = 1.3f; break;
            case 1334: p->m_playerSpeed = 1.6f; break;
            default: break;
        }
    }
}

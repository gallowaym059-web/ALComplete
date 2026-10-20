// ALComplete hooks: connects the solver, the fake-player look-ahead and the menu to the game.
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/HardStreak.hpp>

#include "Hub.hpp"
#include "LevelOverlay.hpp"
#include "Sim.hpp"
#include "Solver.hpp"
#include "Store.hpp"

using namespace geode::prelude;

$execute {
    listenForSettingChanges<bool>("show-button", [](bool v) {
        if (auto h = al::Hub::get()) h->setShown(v);
    });
}

// The floating button lives in the overlay, so it also shows above the pause menu.
class $modify(ALMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        al::Hub::ensure();
        return true;
    }
};

class $modify(ALPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        al::solver::onQuit();
        al::sim().destroy();
        al::lvl::reset();
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        al::sim().create(this);
        al::lvl::init(this);
        if (al::store().autoSolve) al::solver::requestSolve(al::store().autoPlay);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        al::solver::onResetLevel(this);
    }

    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        auto& s = al::sim();
        if (s.owns(player)) { s.dead = true; return; }   // a look-ahead fake died: harmless
        if (s.active) return;
        if (al::solver::onDeath(obj, this)) return;       // the solver handles its own deaths
        PlayLayer::destroyPlayer(player, obj);
    }

    void levelComplete() {
        if (al::solver::onLevelEnd()) return;
        PlayLayer::levelComplete();
    }

    void playEndAnimationToPos(CCPoint pos) {
        if (al::sim().active) return;
        if (al::solver::onLevelEnd()) return;
        PlayLayer::playEndAnimationToPos(pos);
    }

    void playPlatformerEndAnimationToPos(CCPoint pos, bool instant) {
        if (al::sim().active) return;
        if (al::solver::onLevelEnd()) return;
        PlayLayer::playPlatformerEndAnimationToPos(pos, instant);
    }

    void onQuit() {
        al::solver::onQuit();
        al::sim().destroy();
        al::lvl::reset();
        PlayLayer::onQuit();
    }
};

class $modify(ALGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        if (al::solver::blocksInput()) return;   // real taps are ignored while ALComplete drives
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void update(float dt) {
        auto pl = PlayLayer::get();
        if (pl && static_cast<GJBaseGameLayer*>(pl) == this && al::solver::busy()) {
            al::solver::onFrame(pl, dt, [this](float d) { this->GJBaseGameLayer::update(d); });
            return;
        }
        GJBaseGameLayer::update(dt);
    }

    // ---- look-ahead isolation: fakes only touch solids, hazards and slopes ----
    void collisionCheckObjects(PlayerObject* p, gd::vector<GameObject*>* objects, int count, float dt) {
        if (!al::sim().active) return GJBaseGameLayer::collisionCheckObjects(p, objects, count, dt);

        thread_local std::vector<GameObject*> disabled;
        disabled.clear();
        for (auto* obj : *objects) {
            if (!obj || al::collidable(obj)) continue;
            if (obj->m_isDisabled || obj->m_isDisabled2) continue;
            disabled.push_back(obj);
            obj->m_isDisabled = true;
            obj->m_isDisabled2 = true;
        }
        GJBaseGameLayer::collisionCheckObjects(p, objects, count, dt);
        for (auto* obj : disabled) {
            obj->m_isDisabled = false;
            obj->m_isDisabled2 = false;
        }
    }

    bool canBeActivatedByPlayer(PlayerObject* p, EffectGameObject* o) {
        if (al::sim().active) {
            al::handlePortal(p, o->m_objectID);
            return false;
        }
        return GJBaseGameLayer::canBeActivatedByPlayer(p, o);
    }

    void playerTouchedRing(PlayerObject* p, RingObject* o) {
        if (al::sim().active) return;
        GJBaseGameLayer::playerTouchedRing(p, o);
    }

    void playerTouchedTrigger(PlayerObject* p, EffectGameObject* o) {
        if (al::sim().active) { al::handlePortal(p, o->m_objectID); return; }
        GJBaseGameLayer::playerTouchedTrigger(p, o);
    }

    // No audio chaos while the solver races through the level.
    void activateSFXTrigger(SFXTriggerGameObject* o) {
        if (al::sim().active || al::solver::silent()) return;
        GJBaseGameLayer::activateSFXTrigger(o);
    }

    void activateSongEditTrigger(SongTriggerGameObject* o) {
        if (al::sim().active || al::solver::silent()) return;
        GJBaseGameLayer::activateSongEditTrigger(o);
    }

    void gameEventTriggered(GJGameEvent e, int a, int b) {
        if (al::sim().active) return;
        GJBaseGameLayer::gameEventTriggered(e, a, b);
    }
};

class $modify(ALPlayer, PlayerObject) {
    void playSpiderDashEffect(CCPoint a, CCPoint b) {
        if (!al::sim().active) PlayerObject::playSpiderDashEffect(a, b);
    }
    void incrementJumps() {
        if (!al::sim().active) PlayerObject::incrementJumps();
    }
    void ringJump(RingObject* o, bool skip) {
        if (al::sim().active) return;
        PlayerObject::ringJump(o, skip);
    }
};

class $modify(ALStreak, HardStreak) {
    void addPoint(CCPoint p) {
        if (!al::sim().active) HardStreak::addPoint(p);
    }
};

class $modify(ALGameObject, GameObject) {
    void playShineEffect() {
        if (!al::sim().active) GameObject::playShineEffect();
    }
};

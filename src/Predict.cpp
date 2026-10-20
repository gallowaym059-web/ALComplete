#include "Predict.hpp"
#include "PlayerState.hpp"
#include "Sim.hpp"

using namespace geode::prelude;

namespace al {
    int survival(GJBaseGameLayer* L, bool held, bool hold, int ticks) {
        auto& S = sim();
        if (!S.fake1) return ticks + 1;
        bool dual = L->m_gameState.m_isDualMode && L->m_player2 && S.fake2;

        PlayerState a;
        a.save(L->m_player1, held);
        a.load(S.fake1);
        if (dual) {
            PlayerState b;
            b.save(L->m_player2, held);
            b.load(S.fake2);
        }

        constexpr float dt = 1.f / 240.f;
        int survived = ticks + 1;
        S.active = true;
        for (int i = 0; i < ticks && survived > ticks; ++i) {
            for (auto* p : {S.fake1, dual ? S.fake2 : nullptr}) {
                if (!p) continue;
                S.dead = false;
                Sim::clearLogs(p);
                L->checkCollisions(p, dt, false);
                if (S.dead) { survived = i; break; }
                if (i == 0 && hold != held) {
                    if (hold) p->pushButton(PlayerButton::Jump);
                    else p->releaseButton(PlayerButton::Jump);
                }
                p->update(dt);
                p->updateRotation(dt);
                p->updatePlayerScale();
                if (S.dead) { survived = i; break; }
            }
        }
        S.active = false;
        return survived;
    }
}

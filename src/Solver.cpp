// ALComplete solver.
//
// The level is solved by running the *real* game: save a checkpoint, pick an input, step
// the game, and if the player dies, load the checkpoint and try the other input. Dead
// states are remembered, so the search never repeats itself. Because the real game is
// stepped, orbs, pads, portals, triggers and moving objects are all handled by GD itself.
// The path found is then verified by replaying it from the start, and finally autoplayed.
#include "Solver.hpp"
#include "LevelOverlay.hpp"
#include "Predict.hpp"
#include "Store.hpp"

#include <Geode/ui/Notification.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <unordered_set>

using namespace geode::prelude;

namespace al::solver {
    namespace {
        using Clock = std::chrono::steady_clock;
        constexpr double TPS = 240.0;

        struct Input { uint32_t tick; bool down; };

        enum class Mode { Cube, Ship, Ball, Ufo, Wave, Robot, Spider, Swing };

        Mode detectMode(PlayerObject* p) {
            if (p->m_isShip) return Mode::Ship;
            if (p->m_isBird) return Mode::Ufo;
            if (p->m_isBall) return Mode::Ball;
            if (p->m_isDart) return Mode::Wave;
            if (p->m_isRobot) return Mode::Robot;
            if (p->m_isSpider) return Mode::Spider;
            if (p->m_isSwing) return Mode::Swing;
            return Mode::Cube;
        }

        // Modes where holding the button is the natural default.
        bool continuous(Mode m) { return m == Mode::Ship || m == Mode::Wave || m == Mode::Swing; }

        struct Node {
            uint32_t tick;
            CheckpointObject* cp;    // nullptr once dropped to save memory
            uint64_t hash;
            bool heldBefore;
            bool first;              // input tried first
            uint8_t tried;           // 0 = first choice running, 1 = second choice running
            size_t inputsSize;
            size_t pathSize;
        };

        struct G {
            Phase phase = Phase::Idle;
            bool startReq = false, playReq = false, stopReq = false, autoplayAfter = false;
            bool died = false, levelEnded = false, botInput = false, held = false;
            bool enabledPractice = false;
            bool testModeSet = false, prevTestMode = false;

            Store cfg;
            std::vector<Node> stack;
            size_t firstLive = 0;
            std::vector<Input> inputs;
            std::unordered_set<uint64_t> dead;

            std::vector<Input> const* src = nullptr;   // inputs being replayed
            size_t replayPos = 0;
            uint32_t replayTarget = 0;
            uint32_t verifyDeadline = 0;
            uint32_t pathEndTick = 0;
            double carry = 0.0;

            float percent = 0.f, best = 0.f;
            uint64_t backtracks = 0;
            Clock::time_point startedAt;
            int frame = 0;
            // Logical physics ticks. Game progress is a percentage, not a replay clock.
            uint32_t tick = 0;

            std::vector<Input> solution;
            std::vector<CCPoint> path, deaths;
            std::string message;
        } g;

        constexpr size_t kMaxDead = 4'000'000;
        constexpr size_t kMaxDeathMarks = 300;

        uint32_t curTick(PlayLayer*) { return g.tick; }
        double elapsed() { return std::chrono::duration<double>(Clock::now() - g.startedAt).count(); }

        void toast(std::string const& text, bool ok) {
            Loader::get()->queueInMainThread([text, ok] {
                Notification::create(text, ok ? NotificationIcon::Success : NotificationIcon::Error, 3.5f)->show();
            });
        }

        void sendButton(PlayLayer* pl, bool down) {
            g.botInput = true;
            pl->handleButton(down, 1, true);
            g.botInput = false;
        }

        void setHeld(PlayLayer* pl, bool want) {
            if (want == g.held) return;
            sendButton(pl, want);
            g.inputs.push_back({curTick(pl), want});
            g.held = want;
        }

        void clearHolds(PlayLayer* pl, bool held) {
            pl->m_queuedButtons.clear();
            if (pl->m_player1) pl->m_player1->m_holdingButtons[1] = held;
            if (pl->m_player2) pl->m_player2->m_holdingButtons[1] = held;
            g.held = held;
        }

        void resetToStart(PlayLayer* pl) {
            pl->resetLevel();
            g.tick = 0;
            clearHolds(pl, false);
        }

        bool reachedEnd(PlayLayer* pl) {
            return g.levelEnded || pl->m_levelEndAnimationStarted || pl->getCurrentPercent() >= 100.f;
        }

        void stepOnce(PlayLayer* pl, StepFn const& step, int ticks) {
            g.died = false;
            pl->m_extraDelta = 0.0;
            step(static_cast<float>((ticks + 0.5) / TPS));   // advance the real game's physics
            g.tick += static_cast<uint32_t>(std::max(0, ticks));
        }

        void releaseCheckpoints() {
            for (auto& n : g.stack) if (n.cp) n.cp->release();
            g.stack.clear();
            g.firstLive = 0;
        }

        void applySafeMode(PlayLayer* pl) {
            if (!g.cfg.safeMode) return;
            if (!g.testModeSet) { g.prevTestMode = pl->m_isTestMode; g.testModeSet = true; }
            pl->m_isTestMode = true;
        }

        uint64_t mix(uint64_t h, uint64_t v) { return h ^ (v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2)); }
        uint64_t bits(float f) { uint32_t u; std::memcpy(&u, &f, 4); return u; }

        uint64_t hashPlayer(uint64_t h, PlayerObject* p) {
            if (!p) return h;
            h = mix(h, bits(p->getPositionX()));
            h = mix(h, bits(p->getPositionY()));
            h = mix(h, bits(p->getRotation()));
            h = mix(h, bits((float)p->m_yVelocity));
            h = mix(h, bits(p->m_vehicleSize));
            h = mix(h, bits(p->m_playerSpeed));
            h = mix(h, bits(p->m_gravityMod));
            h = mix(h, (uint64_t)detectMode(p));
            uint64_t flags = (uint64_t(p->m_isUpsideDown) << 0) | (uint64_t(p->m_isOnGround) << 1) |
                (uint64_t(p->m_isSideways) << 2) | (uint64_t(p->m_isGoingLeft) << 3) |
                (uint64_t(p->m_isDashing) << 4) | (uint64_t(p->m_isOnSlope) << 5);
            return mix(h, flags);
        }

        uint64_t hashState(PlayLayer* pl, bool held) {
            uint64_t h = 1469598103934665603ULL;
            h = mix(h, curTick(pl));
            h = mix(h, held);
            h = hashPlayer(h, pl->m_player1);
            if (pl->m_gameState.m_isDualMode) h = hashPlayer(h, pl->m_player2);
            return h;
        }

        // ---- route choice -------------------------------------------------------------
        bool chooseFirst(PlayLayer* pl) {
            bool def = continuous(detectMode(pl->m_player1)) ? g.held : false;
            if (g.cfg.route == 0) return def;                       // Simplest: change nothing unless forced
            int keep = survival(pl, g.held, def, g.cfg.lookahead);  // Safest: whichever lives longer first
            if (keep > g.cfg.lookahead) return def;
            int other = survival(pl, g.held, !def, g.cfg.lookahead);
            return other > keep ? !def : def;
        }

        // ---- finishing ----------------------------------------------------------------
        void restoreAfterSolve(PlayLayer* pl) {
            releaseCheckpoints();
            g.dead.clear();
            sendButton(pl, false);
            g.held = false;
            if (g.enabledPractice && g.cfg.restorePractice && pl->m_isPracticeMode) pl->togglePracticeMode(false);
            if (g.testModeSet) {
                pl->m_isTestMode = g.prevTestMode;
                g.testModeSet = false;
            }
            pl->removeAllCheckpoints();
        }

        void beginPlay(PlayLayer* pl) {
            if (g.solution.empty()) return;
            g.cfg = store();
            pl->removeAllCheckpoints();
            resetToStart(pl);
            applySafeMode(pl);
            g.src = &g.solution;
            g.replayPos = 0;
            g.carry = 0.0;
            g.percent = 0.f;
            g.startedAt = Clock::now();
            g.phase = Phase::Playing;
            lvl::clear();
        }

        void finishSolve(PlayLayer* pl, bool ok, std::string const& reason, int verified) {
            restoreAfterSolve(pl);
            if (ok) {
                g.solution = g.inputs;
                size_t clicks = std::count_if(g.solution.begin(), g.solution.end(), [](Input const& i) { return i.down; });
                g.message = fmt::format("Solved in {:.0f}s: {} inputs, {} clicks.{}", elapsed(), g.solution.size(), clicks,
                                        verified == 0 ? " (verification failed, may desync)" : "");
                toast(g.message, true);
                if (g.cfg.verbose) log::info("ALComplete: {}", g.message);
                resetToStart(pl);
                if (g.autoplayAfter) beginPlay(pl);
                else g.phase = Phase::Idle;
            } else {
                g.message = fmt::format("{} Best progress {:.1f}%.", reason.empty() ? "No path found." : reason, g.best);
                toast(g.message, false);
                log::info("ALComplete: {}", g.message);
                g.phase = Phase::Idle;
                resetToStart(pl);
            }
            if (g.phase != Phase::Playing) {
                g.levelEnded = false;
                lvl::draw(g.path, g.deaths);
            }
        }

        void startVerification(PlayLayer* pl) {
            g.phase = Phase::Verifying;
            g.levelEnded = false;
            g.src = &g.inputs;
            g.replayPos = 0;
            g.verifyDeadline = g.pathEndTick + static_cast<uint32_t>(TPS * 3);
            releaseCheckpoints();
            resetToStart(pl);
        }

        void onPathFound(PlayLayer* pl) {
            g.pathEndTick = curTick(pl);
            if (g.cfg.verify) startVerification(pl);
            else finishSolve(pl, true, "", -1);
        }

        // ---- search -------------------------------------------------------------------
        void backtrack(PlayLayer* pl);

        void pushNode(PlayLayer* pl) {
            uint64_t h = hashState(pl, g.held);
            if (g.dead.count(h)) { backtrack(pl); return; }

            Node n{curTick(pl), pl->createCheckpoint(), h, g.held, false, 0, g.inputs.size(), g.path.size()};
            if (n.cp) n.cp->retain();
            n.first = chooseFirst(pl);
            g.stack.push_back(n);

            while (g.stack.size() - g.firstLive > (size_t)g.cfg.maxCheckpoints) {
                auto& old = g.stack[g.firstLive];
                if (old.cp) old.cp->release();
                old.cp = nullptr;
                g.firstLive++;
            }
            setHeld(pl, n.first);
        }

        void backtrack(PlayLayer* pl) {
            g.backtracks++;
            g.deaths.push_back(pl->m_player1->getPosition());
            if (g.deaths.size() > kMaxDeathMarks) g.deaths.erase(g.deaths.begin());
            if (g.dead.size() > kMaxDead) g.dead.clear();

            while (!g.stack.empty() && g.stack.back().tried >= 1) {
                auto& b = g.stack.back();
                g.dead.insert(b.hash);
                if (b.cp) b.cp->release();
                g.stack.pop_back();
            }
            if (g.firstLive > g.stack.size()) g.firstLive = g.stack.size();

            if (g.stack.empty()) { finishSolve(pl, false, "The level looks impossible with these settings.", -1); return; }

            auto& n = g.stack.back();
            n.tried = 1;
            g.inputs.resize(n.inputsSize);
            if (g.path.size() > n.pathSize) g.path.resize(n.pathSize);

            if (n.cp) {
                pl->loadFromCheckpoint(n.cp);
                g.tick = n.tick;
                g.levelEnded = false;
                clearHolds(pl, n.heldBefore);
                setHeld(pl, !n.first);
            } else {
                g.phase = Phase::Replaying;        // checkpoint dropped: replay from the start
                g.src = &g.inputs;
                g.replayTarget = n.tick;
                g.replayPos = 0;
                g.levelEnded = false;
                resetToStart(pl);
            }
        }

        void applyScheduled(PlayLayer* pl) {
            while (g.src && g.replayPos < g.src->size() && (*g.src)[g.replayPos].tick <= curTick(pl)) {
                auto const& in = (*g.src)[g.replayPos];
                sendButton(pl, in.down);
                g.held = in.down;
                g.replayPos++;
            }
        }

        void searchStep(PlayLayer* pl, StepFn const& step) {
            stepOnce(pl, step, g.cfg.ticksPerStep);
            if (g.phase != Phase::Solving) return;
            if (reachedEnd(pl)) { g.path.push_back(pl->m_player1->getPosition()); onPathFound(pl); return; }
            if (g.died) { backtrack(pl); return; }

            g.path.push_back(pl->m_player1->getPosition());
            g.percent = pl->getCurrentPercent();
            g.best = std::max(g.best, g.percent);
            if (!g.stack.empty() && curTick(pl) >= g.stack.back().tick + (uint32_t)g.cfg.ticksPerStep) pushNode(pl);
        }

        void replayStep(PlayLayer* pl, StepFn const& step) {
            if (curTick(pl) >= g.replayTarget) {
                bool bad = curTick(pl) != g.replayTarget;
                if (!bad) {
                    applyScheduled(pl);
                    bad = g.held != g.stack.back().heldBefore;
                }
                if (bad) { finishSolve(pl, false, "Replay desynced (the level may be non-deterministic).", -1); return; }
                auto& n = g.stack.back();
                n.cp = pl->createCheckpoint();
                if (n.cp) n.cp->retain();
                g.firstLive = g.stack.size() - 1;
                g.phase = Phase::Solving;
                setHeld(pl, !n.first);
                return;
            }
            applyScheduled(pl);
            stepOnce(pl, step, g.cfg.ticksPerStep);
            if (g.died || reachedEnd(pl)) finishSolve(pl, false, "Replay desynced (the level may be non-deterministic).", -1);
        }

        void verifyStep(PlayLayer* pl, StepFn const& step) {
            applyScheduled(pl);
            stepOnce(pl, step, g.cfg.ticksPerStep);
            if (reachedEnd(pl)) { finishSolve(pl, true, "", 1); return; }
            if (g.died || curTick(pl) > g.verifyDeadline) finishSolve(pl, true, "", 0);
        }

        void startSolve(PlayLayer* pl) {
            if (pl->m_isPlatformer) {
                g.message = "Platformer levels are not supported yet.";
                toast(g.message, false);
                return;
            }
            releaseCheckpoints();
            g.cfg = store();
            g.inputs.clear(); g.path.clear(); g.deaths.clear(); g.dead.clear();
            g.best = g.percent = 0.f;
            g.backtracks = 0;
            g.died = g.levelEnded = false;
            g.startedAt = Clock::now();
            g.frame = 0;

            g.enabledPractice = !pl->m_isPracticeMode;
            if (g.enabledPractice) pl->togglePracticeMode(true);
            pl->removeAllCheckpoints();
            applySafeMode(pl);
            resetToStart(pl);

            g.phase = Phase::Solving;
            g.path.push_back(pl->m_player1->getPosition());
            pushNode(pl);
        }

        void playStep(PlayLayer* pl, StepFn const& step, float dt) {
            g.carry += dt;
            int ticks = std::min(24, (int)(g.carry * TPS));
            g.carry -= ticks / TPS;
            g.carry = std::min(g.carry, 1.0 / 30.0);
            for (int i = 0; i < ticks && g.phase == Phase::Playing; ++i) {
                applyScheduled(pl);
                stepOnce(pl, step, 1);
            }
            g.percent = pl->getCurrentPercent();
        }
    }

    // ------------------------------------------------------------------------------------
    Status status() {
        Status s;
        s.phase = g.phase;
        s.percent = g.percent;
        s.best = g.best;
        s.backtracks = g.backtracks;
        s.elapsed = g.phase == Phase::Idle ? 0.0 : elapsed();
        s.hasSolution = !g.solution.empty();
        s.solutionInputs = g.solution.size();
        s.solutionClicks = std::count_if(g.solution.begin(), g.solution.end(), [](Input const& i) { return i.down; });
        s.message = g.message;
        return s;
    }

    bool busy() { return g.phase != Phase::Idle || g.startReq || g.playReq; }
    bool blocksInput() { return busy() && !g.botInput; }
    bool silent() { return g.phase == Phase::Solving || g.phase == Phase::Replaying || g.phase == Phase::Verifying; }

    void requestSolve(bool autoplayAfter) {
        g.startReq = true;
        g.stopReq = false;
        g.autoplayAfter = autoplayAfter;
    }
    void requestPlay() { if (!g.solution.empty()) { g.playReq = true; g.stopReq = false; } }
    void requestStop() { g.stopReq = true; }
    void clearSolution() { g.solution.clear(); g.message.clear(); }

    void onFrame(PlayLayer* pl, float dt, StepFn const& step) {
        if (g.stopReq) {
            g.stopReq = false;
            g.startReq = g.playReq = false;
            if (g.phase == Phase::Playing) {
                g.phase = Phase::Idle;
                sendButton(pl, false);
                g.message = "Autoplay stopped.";
            } else if (g.phase != Phase::Idle) {
                finishSolve(pl, false, "Stopped.", -1);
            }
            return;
        }
        if (g.startReq) { g.startReq = false; startSolve(pl); }
        if (g.playReq) { g.playReq = false; beginPlay(pl); }
        if (g.phase == Phase::Idle) {
            // nothing running; let the game continue normally next frame
            return;
        }

        if (g.phase == Phase::Playing) {
            playStep(pl, step, dt);
            return;
        }

        if (g.cfg.giveUpSec > 0 && elapsed() > g.cfg.giveUpSec) {
            finishSolve(pl, false, "Gave up (time limit).", -1);
            return;
        }

        auto t0 = Clock::now();
        while (g.phase != Phase::Idle && g.phase != Phase::Playing) {
            auto ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
            if (ms >= g.cfg.budgetMs) break;
            switch (g.phase) {
                case Phase::Solving: searchStep(pl, step); break;
                case Phase::Replaying: replayStep(pl, step); break;
                case Phase::Verifying: verifyStep(pl, step); break;
                default: break;
            }
        }
        if (g.phase == Phase::Solving || g.phase == Phase::Replaying) {
            if (++g.frame % 6 == 0) lvl::draw(g.path, g.deaths);
        }
    }

    bool onDeath(GameObject* culprit, PlayLayer* pl) {
        if (g.phase == Phase::Playing) {
            if (culprit == pl->m_anticheatSpike) return false;
            g.phase = Phase::Idle;
            g.message = fmt::format("Autoplay died at {:.1f}%. Try a smaller Ticks per step and solve again.", g.percent);
            toast(g.message, false);
            return false;                         // let the real death happen
        }
        if (g.phase == Phase::Idle) return false;
        if (culprit != pl->m_anticheatSpike) g.died = true;
        return true;
    }

    bool onLevelEnd() {
        if (g.phase == Phase::Playing) {
            g.phase = Phase::Idle;
            g.message = "Level complete!";
            toast(g.message, true);
            return false;                         // let the real completion happen
        }
        if (g.phase == Phase::Idle) return false;
        g.levelEnded = true;
        return true;
    }

    void onResetLevel(PlayLayer* pl) {
        if (g.phase == Phase::Idle && g.testModeSet) {
            pl->m_isTestMode = g.prevTestMode;
            g.testModeSet = false;
        }
    }

    void onQuit() {
        releaseCheckpoints();
        g.phase = Phase::Idle;
        g.startReq = g.playReq = g.stopReq = false;
        g.testModeSet = false;
        g.path.clear();
        g.deaths.clear();
        g.dead.clear();
        g.inputs.clear();
    }
}

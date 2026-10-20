#include "Hub.hpp"
#include "Store.hpp"
#include "Solver.hpp"
#include "UiKit.hpp"

#include <Geode/ui/Notification.hpp>
#include <Geode/ui/OverlayManager.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>

using namespace geode::prelude;
using namespace al::ui;

namespace al {
    namespace {
        constexpr float PW = 400.f, PH = 272.f, HEADER = 34.f, SIDE = 104.f, ROWH = 27.f, TABH = 28.f;
        constexpr float BTN_R = 22.f;
        constexpr float kPi = 3.14159265f;
        Hub* s_inst = nullptr;

        float easeOutBack(float t) {
            t = std::clamp(t, 0.f, 1.f);
            constexpr float c1 = 1.70158f, c3 = c1 + 1.f;
            float u = t - 1.f;
            return 1.f + c3 * u * u * u + c1 * u * u;
        }
        float smooth(float dt, float rate) { return 1.f - std::exp(-dt * rate); }

        struct IconDef { char const* name; char const* frame; };
        const IconDef kIcons[] = {
            {"ALComplete", nullptr}, {"Play", "GJ_playBtn2_001.png"}, {"Star", "GJ_bigStar_001.png"},
            {"Diamond", "GJ_bigDiamond_001.png"}, {"Moon", "GJ_moonsIcon_001.png"},
            {"Info", "GJ_infoIcon_001.png"}, {"Custom icon.png", nullptr},
        };
        constexpr int kIconCount = (int)(sizeof(kIcons) / sizeof(kIcons[0]));

        std::string fmtNum(float v, bool isInt) {
            char buf[32];
            if (isInt) std::snprintf(buf, sizeof buf, "%d", (int)std::lround(v));
            else std::snprintf(buf, sizeof buf, "%.2f", v);
            return buf;
        }
    }

    Hub* Hub::get() { return s_inst; }

    void Hub::ensure() {
        if (s_inst) return;
        auto h = new Hub();
        if (h && h->init()) {
            h->autorelease();
            s_inst = h;
            OverlayManager::get()->addChild(h);
        } else {
            CC_SAFE_DELETE(h);
        }
    }

    void Hub::setShown(bool shown) {
        m_shown = shown;
        if (m_btn) m_btn->setVisible(shown);
        if (!shown) m_openTarget = false;
    }

    // ---------------------------------------------------------------------------------------
    bool Hub::init() {
        if (!CCLayer::init()) return false;
        this->setTouchMode(kCCTouchesOneByOne);
        this->setTouchEnabled(true);

        buildCats();
        buildPanel();

        m_hud = CCNode::create();
        m_hudDraw = CCDrawNode::create();
        ui::blend(m_hudDraw);
        m_hud->addChild(m_hudDraw, 0);
        m_hudLbl = ui::label(m_hud, "", "chatFont.fnt", 0.55f, {255, 255, 255}, ccp(0, 0), ccp(0.5f, 0.5f), 2);
        m_hud->setVisible(false);
        this->addChild(m_hud, 1);

        m_btn = CCNode::create();
        m_btnDraw = CCDrawNode::create();
        ui::blend(m_btnDraw);
        m_btn->addChild(m_btnDraw, 0);
        this->addChild(m_btn, 5);

        auto& st = store();
        auto win = CCDirector::get()->getWinSize();
        m_btnTarget = ccp(win.width * st.hubX, win.height * st.hubY);
        m_btn->setPosition(m_btnTarget);
        rebuildIcon();
        applyPositions();
        applyAppearance();
        setShown(Mod::get()->getSettingValue<bool>("show-button"));

        this->scheduleUpdate();
        return true;
    }

    void Hub::registerWithTouchDispatcher() {
        CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(this, -1000, true);
    }

    // ---------------------------------------------------------------------------------------
    std::string Hub::statusLine() {
        auto s = solver::status();
        char buf[160];
        switch (s.phase) {
            case solver::Phase::Solving:
                std::snprintf(buf, sizeof buf, "Solving %.1f%%  (best %.1f%%)  %llu backtracks  %.0fs",
                              s.percent, s.best, (unsigned long long)s.backtracks, s.elapsed);
                return buf;
            case solver::Phase::Replaying:
                std::snprintf(buf, sizeof buf, "Replaying to a saved point...  best %.1f%%", s.best);
                return buf;
            case solver::Phase::Verifying: return "Verifying the path from the start...";
            case solver::Phase::Playing:
                std::snprintf(buf, sizeof buf, "Autoplaying  %.1f%%", s.percent);
                return buf;
            default: break;
        }
        if (!s.message.empty()) return s.message;
        return s.hasSolution ? "Solution ready. Tap Play." : "Idle. Open a level and tap Solve + Play.";
    }

    void Hub::doSolve(bool autoplay) {
        if (!PlayLayer::get()) {
            Notification::create("Open a level first", NotificationIcon::Warning, 2.f)->show();
            return;
        }
        solver::requestSolve(autoplay);
        m_openTarget = false;   // get out of the way so you can watch
    }

    void Hub::buildCats() {
        auto toggle = [](std::string n, bool* b, std::function<void()> cb = nullptr) {
            Row r; r.type = RT::Toggle; r.name = std::move(n); r.b = b; r.onChange = std::move(cb); return r;
        };
        auto slider = [](std::string n, float* f, float mn, float mx, float step, std::function<void()> cb = nullptr) {
            Row r; r.type = RT::Slider; r.name = std::move(n); r.f = f; r.mn = mn; r.mx = mx; r.step = step;
            r.onChange = std::move(cb); return r;
        };
        auto islider = [](std::string n, int* i, float mn, float mx, float step, std::function<std::string()> vt = nullptr) {
            Row r; r.type = RT::Slider; r.name = std::move(n); r.i = i; r.mn = mn; r.mx = mx; r.step = step;
            r.valueText = std::move(vt); return r;
        };
        auto info = [](std::function<std::string()> fn, int span = 1) {
            Row r; r.type = RT::Info; r.info = std::move(fn); r.span = span; return r;
        };
        auto appearance = [this] { applyAppearance(); };
        auto& st = store();

        // ---- Solver
        Cat solverCat{"Solver", {}};
        solverCat.rows.push_back(info([this] { return statusLine(); }, 2));
        {
            Row r; r.type = RT::Buttons;
            r.btns = {
                {"Solve+Play", [this] { doSolve(true); }, true},
                {"Solve", [this] { doSolve(false); }, false},
                {"Play", [this] {
                    if (!PlayLayer::get()) { Notification::create("Open a level first", NotificationIcon::Warning, 2.f)->show(); return; }
                    if (!solver::status().hasSolution) { Notification::create("Nothing solved yet", NotificationIcon::Warning, 2.f)->show(); return; }
                    solver::requestPlay();
                    m_openTarget = false;
                }, false},
                {"Stop", [] { solver::requestStop(); }, false},
            };
            solverCat.rows.push_back(std::move(r));
        }
        solverCat.rows.push_back(toggle("Auto-solve on level start", &st.autoSolve));
        solverCat.rows.push_back(toggle("Auto-play when solved", &st.autoPlay));
        solverCat.rows.push_back(slider("Time per frame (ms)", &st.budgetMs, 2.f, 30.f, 1.f));
        solverCat.rows.push_back(islider("Ticks per step", &st.ticksPerStep, 1.f, 8.f, 1.f));
        solverCat.rows.push_back(islider("Max checkpoints", &st.maxCheckpoints, 50.f, 2000.f, 50.f));
        m_cats.push_back(std::move(solverCat));

        // ---- Route
        Cat route{"Route", {}};
        {
            Row r; r.type = RT::Segment; r.name = "Route style"; r.i = &st.route; r.opts = {"Simplest", "Safest"};
            route.rows.push_back(std::move(r));
        }
        route.rows.push_back(info([] {
            return std::string("Simplest keeps your input unchanged unless a hazard forces a change, so it uses the fewest clicks. "
                               "Safest looks ahead and tries the input that survives longer first.");
        }, 3));
        route.rows.push_back(toggle("Verify path by replay", &st.verify));
        route.rows.push_back(islider("Safest look-ahead (ticks)", &st.lookahead, 20.f, 240.f, 10.f));
        route.rows.push_back(islider("Give up after", &st.giveUpSec, 0.f, 600.f, 10.f,
            [&st] { return st.giveUpSec == 0 ? std::string("Never") : std::to_string(st.giveUpSec) + "s"; }));
        route.rows.push_back(info([] {
            auto s = solver::status();
            return s.hasSolution ? "Last solution: " + std::to_string(s.solutionInputs) + " inputs, "
                                     + std::to_string(s.solutionClicks) + " clicks"
                                 : std::string("No solution stored yet");
        }));
        m_cats.push_back(std::move(route));

        // ---- Visuals
        Cat vis{"Visuals", {}};
        vis.rows.push_back(toggle("Show solver path", &st.showPath));
        vis.rows.push_back(toggle("Show death marks", &st.showDeaths));
        vis.rows.push_back(toggle("Show status HUD", &st.showHud));
        vis.rows.push_back(toggle("Show progress ring", &st.showRing));
        vis.rows.push_back(toggle("Animations", &st.animations));
        vis.rows.push_back(slider("Glow strength", &st.glow, 0.f, 1.5f, 0.05f));
        m_cats.push_back(std::move(vis));

        // ---- Appearance
        Cat app{"Appearance", {}};
        app.rows.push_back(slider("Menu opacity", &st.menuOpacity, 0.3f, 1.f, 0.05f, appearance));
        app.rows.push_back(slider("Menu scale", &st.menuScale, 0.6f, 1.4f, 0.05f, appearance));
        app.rows.push_back(slider("Button size", &st.buttonScale, 0.6f, 1.6f, 0.05f, appearance));
        app.rows.push_back(slider("Button opacity", &st.buttonOpacity, 0.3f, 1.f, 0.05f, appearance));
        app.rows.push_back(toggle("Snap button to edge", &st.snapEdge));
        {
            Row r; r.type = RT::Cycle; r.name = "Accent colour"; r.i = &st.accent; r.mx = (float)ui::accentCount();
            r.valueText = [&st] { return std::string(ui::accentName(st.accent)); };
            app.rows.push_back(std::move(r));
        }
        {
            Row r; r.type = RT::Cycle; r.name = "Button icon"; r.i = &st.icon; r.mx = (float)kIconCount;
            r.valueText = [&st] { return std::string(kIcons[std::clamp(st.icon, 0, kIconCount - 1)].name); };
            r.onChange = [this] { rebuildIcon(); };
            app.rows.push_back(std::move(r));
        }
        {
            Row r; r.type = RT::Buttons;
            r.btns = {{"Reset positions", [this] {
                auto& s = store();
                s.hubX = 0.93f; s.hubY = 0.62f; s.panelX = 0.5f; s.panelY = 0.5f;
                auto win = CCDirector::get()->getWinSize();
                m_btnTarget = ccp(win.width * s.hubX, win.height * s.hubY);
                applyPositions();
                s.save();
            }, false}};
            app.rows.push_back(std::move(r));
        }
        m_cats.push_back(std::move(app));

        // ---- Safety
        Cat safe{"Safety", {}};
        safe.rows.push_back(toggle("Safe Mode", &st.safeMode));
        safe.rows.push_back(toggle("Restore practice mode", &st.restorePractice));
        safe.rows.push_back(info([] {
            return std::string("Safe Mode marks assisted runs as test runs, so completions are not saved or submitted. "
                               "It is on by default. Turn it off only if you understand what that means for your progress.");
        }, 4));
        m_cats.push_back(std::move(safe));

        // ---- Diagnostics
        Cat diag{"Diagnostics", {}};
        diag.rows.push_back(toggle("Verbose logging", &st.verbose));
        {
            Row r; r.type = RT::Buttons;
            r.btns = {
                {"Log status", [this] { log::info("ALComplete status: {}", statusLine()); }, false},
                {"Clear solution", [] { solver::clearSolution(); }, false},
            };
            diag.rows.push_back(std::move(r));
        }
        diag.rows.push_back(info([this] { return statusLine(); }, 2));
        m_cats.push_back(std::move(diag));

        // ---- About
        Cat about{"About", {}};
        about.rows.push_back(info([] {
            return std::string("ALComplete = Auto Levels Complete.\n\n"
                               "Open a level, tap Solve + Play. The solver steps the real game, backtracks from every death, "
                               "verifies the path, then plays it.");
        }, 4));
        about.rows.push_back(info([] {
            return "Custom button icon: put icon.png in\n" + geode::utils::string::pathToString(Mod::get()->getConfigDir());
        }, 3));
        m_cats.push_back(std::move(about));
    }

    void Hub::buildPanel() {
        m_panel = CCNode::create();
        m_panel->setContentSize(CCSize(PW, PH));
        m_panel->setAnchorPoint(ccp(0.5f, 0.5f));
        m_panel->ignoreAnchorPointForPosition(false);
        m_panel->setVisible(false);

        m_bg = CCDrawNode::create();
        ui::blend(m_bg);
        m_panel->addChild(m_bg, 0);

        m_rows = CCNode::create();
        m_panel->addChild(m_rows, 1);

        // title + tab labels
        auto t = ui::label(m_panel, "ALComplete", "bigFont.fnt", 0.42f, {255, 255, 255}, ccp(14, PH - HEADER / 2), ccp(0, 0.5f), 3);
        m_lbls.push_back({t, 1.f});
        auto sub = ui::label(m_panel, "AUTO LEVELS COMPLETE", "chatFont.fnt", 0.42f, {150, 160, 185}, ccp(PW - 40, PH - HEADER / 2), ccp(1, 0.5f), 3);
        m_lbls.push_back({sub, 0.8f});

        for (size_t i = 0; i < m_cats.size(); ++i) {
            float cy = PH - HEADER - 10 - (i + 0.5f) * TABH;
            auto l = ui::label(m_panel, m_cats[i].name, "bigFont.fnt", 0.3f, {255, 255, 255}, ccp(20, cy), ccp(0, 0.5f), 3);
            m_tabLbls.push_back(l);
            m_lbls.push_back({l, 1.f});
            m_tabAnim.push_back(i == 0 ? 1.f : 0.f);
        }
        this->addChild(m_panel, 3);
        rebuildRows();
    }

    void Hub::selectCat(int idx) {
        if (idx == m_cat || idx < 0 || idx >= (int)m_cats.size()) return;
        m_cat = idx;
        rebuildRows();
        m_rows->setPositionX(store().animations ? 22.f : 0.f);   // slide-in
    }

    void Hub::rebuildRows() {
        // drop old row labels from the fade list
        m_lbls.erase(std::remove_if(m_lbls.begin(), m_lbls.end(), [this](Lbl const& l) {
            return l.l->getParent() == m_rows;
        }), m_lbls.end());
        m_rows->removeAllChildrenWithCleanup(true);
        m_rowsDraw = CCDrawNode::create();
        ui::blend(m_rowsDraw);
        m_rows->addChild(m_rowsDraw, 0);

        float x0 = SIDE + 10.f, cw = PW - SIDE - 20.f;
        float cursor = 0.f;
        for (auto& r : m_cats[m_cat].rows) {
            float h = r.span * ROWH;
            r.rect = CCRect(x0, PH - HEADER - 8.f - cursor - h, cw, h);
            cursor += h;
            r.cache.clear();
            r.anim = (r.type == RT::Toggle && r.b && *r.b) ? 1.f : 0.f;
            float cy = r.rect.origin.y + h / 2.f;
            switch (r.type) {
                case RT::Info:
                    r.nameLbl = nullptr;
                    r.valLbl = CCLabelBMFont::create("", "chatFont.fnt", (cw - 20.f) / 0.5f, kCCTextAlignmentLeft);
                    r.valLbl->setScale(0.5f);
                    r.valLbl->setAnchorPoint(ccp(0, 0.5f));
                    r.valLbl->setPosition(ccp(x0 + 10, cy));
                    r.valLbl->setColor({170, 178, 200});
                    m_rows->addChild(r.valLbl, 2);
                    m_lbls.push_back({r.valLbl, 0.9f});
                    break;
                case RT::Buttons: {
                    int n = (int)r.btns.size();
                    float bw = (cw - 20.f - (n - 1) * 6.f) / n;
                    for (int i = 0; i < n; ++i) {
                        auto& b = r.btns[i];
                        b.lbl = ui::label(m_rows, b.text, "bigFont.fnt", 0.27f, {255, 255, 255},
                                          ccp(x0 + 10 + i * (bw + 6) + bw / 2, cy), ccp(0.5f, 0.5f), 3);
                        float maxW = bw - 8.f;
                        float w = b.lbl->getContentSize().width * b.lbl->getScale();
                        if (w > maxW) b.lbl->setScale(b.lbl->getScale() * maxW / w);
                        m_lbls.push_back({b.lbl, 1.f});
                    }
                    break;
                }
                default: {
                    r.nameLbl = ui::label(m_rows, r.name, "chatFont.fnt", 0.5f, {232, 236, 248},
                                          ccp(x0 + 10, cy), ccp(0, 0.5f), 3);
                    m_lbls.push_back({r.nameLbl, 1.f});
                    if (r.type == RT::Slider || r.type == RT::Cycle) {
                        float right = x0 + cw;
                        bool cyc = r.type == RT::Cycle;
                        r.valLbl = ui::label(m_rows, "", "chatFont.fnt", 0.46f, {190, 200, 225},
                                             ccp(cyc ? right - 71 : right - 8, cy), ccp(cyc ? 0.5f : 1.f, 0.5f), 3);
                        m_lbls.push_back({r.valLbl, 1.f});
                    }
                    break;
                }
            }
        }
        refreshTexts();
    }

    void Hub::refreshTexts() {
        for (auto& r : m_cats[m_cat].rows) {
            std::string s;
            if (r.type == RT::Info && r.info) s = r.info();
            else if (r.type == RT::Slider || r.type == RT::Cycle) {
                if (r.valueText) s = r.valueText();
                else if (r.i) s = fmtNum((float)*r.i, true);
                else if (r.f) s = fmtNum(*r.f, false);
            } else continue;
            if (s != r.cache && r.valLbl) {
                r.cache = s;
                r.valLbl->setString(s.c_str());
                if (r.type == RT::Info) r.valLbl->setAnchorPoint(ccp(0, 0.5f));
            }
        }
    }

    void Hub::rebuildIcon() {
        if (m_icon) { m_icon->removeFromParent(); m_icon = nullptr; }
        auto& st = store();
        int idx = std::clamp(st.icon, 0, kIconCount - 1);
        CCSprite* spr = nullptr;
        if (idx == kIconCount - 1) {
            auto path = Mod::get()->getConfigDir() / "icon.png";
            std::error_code ec;
            if (std::filesystem::exists(path, ec)) spr = CCSprite::create(geode::utils::string::pathToString(path).c_str());
        } else if (idx > 0) {
            spr = CCSprite::createWithSpriteFrameName(kIcons[idx].frame);
        }
        if (!spr) spr = CCSprite::create("icon.png"_spr);
        if (!spr) return;
        float m = std::max(spr->getContentSize().width, spr->getContentSize().height);
        spr->setScale(BTN_R * 1.15f / m);
        m_icon = spr;
        m_btn->addChild(spr, 2);
    }

    void Hub::applyAppearance() {
        auto& st = store();
        if (m_btn) m_btn->setScale(st.buttonScale);
        if (m_icon) static_cast<CCSprite*>(m_icon)->setOpacity((GLubyte)(255 * st.buttonOpacity));
    }

    void Hub::applyPositions() {
        auto& st = store();
        auto win = CCDirector::get()->getWinSize();
        m_panel->setPosition(ccp(win.width * st.panelX, win.height * st.panelY));
    }

    void Hub::togglePanel() {
        m_openTarget = !m_openTarget;
        if (m_openTarget) {
            m_panel->setVisible(true);
            refreshTexts();
        }
    }

    // ---------------------------------------------------------------------------------------
    void Hub::update(float dt) {
        m_time += dt;
        auto& st = store();
        auto s = solver::status();

        // smooth follow of the circle
        CCPoint cur = m_btn->getPosition();
        float k = smooth(dt, m_drag == Drag::Button ? 30.f : 14.f);
        cur = cur + (m_btnTarget - cur) * k;
        m_btn->setPosition(cur);

        // press squish
        float pt = (m_drag == Drag::Button && !m_moved) ? 1.f : 0.f;
        m_pressAnim += (pt - m_pressAnim) * smooth(dt, 22.f);
        if (m_btn) m_btn->setScale(st.buttonScale * (1.f - 0.08f * m_pressAnim));

        // menu open / close
        float tgt = m_openTarget ? 1.f : 0.f;
        if (!st.animations) m_open = tgt;
        else {
            m_open += (tgt - m_open) * smooth(dt, 15.f);
            if (std::fabs(m_open - tgt) < 0.004f) m_open = tgt;
        }
        bool vis = m_open > 0.01f && m_shown;
        m_panel->setVisible(vis);
        if (vis) {
            m_panel->setScale(st.menuScale * (0.86f + 0.14f * easeOutBack(m_open)));
            m_rows->setPositionX(m_rows->getPositionX() * std::exp(-dt * 14.f));
            for (size_t i = 0; i < m_tabAnim.size(); ++i)
                m_tabAnim[i] += ((int)i == m_cat ? 1.f - m_tabAnim[i] : -m_tabAnim[i]) * smooth(dt, 14.f);
            float fade = std::clamp(m_open, 0.f, 1.f);
            for (auto& l : m_lbls) l.l->setOpacity((GLubyte)(255.f * fade * l.base * (0.35f + 0.65f * st.menuOpacity)));
            for (auto& r : m_cats[m_cat].rows)
                if (r.type == RT::Toggle && r.b) r.anim += ((*r.b ? 1.f : 0.f) - r.anim) * smooth(dt, 16.f);
            if (std::fmod(m_time, 0.1f) < dt) refreshTexts();
            drawPanel(fade);
            drawRows(fade);
        }

        // HUD pill: appears while working, and for a few seconds after a result
        if (s.message != m_lastMsg) { m_lastMsg = s.message; m_hudUntil = m_time + 5.f; }
        bool showHud = st.showHud && m_shown && (s.phase != solver::Phase::Idle || m_time < m_hudUntil);
        m_hud->setVisible(showHud);
        if (showHud) drawHud();

        drawButton();
    }

    // ---------------------------------------------------------------------------------------
    void Hub::drawButton() {
        auto& st = store();
        auto s = solver::status();
        auto A = ui::accentColor(st.accent);
        float bo = st.buttonOpacity;
        bool working = s.phase != solver::Phase::Idle;
        float pulse = 0.8f + 0.2f * std::sin(m_time * 2.4f);

        m_btnDraw->clear();
        CCPoint c = ccp(0, 0);
        ui::glowDot(m_btnDraw, c, BTN_R, 14.f, A, st.glow * bo * pulse * (working ? 1.6f : 1.f));
        ui::dot(m_btnDraw, c, BTN_R + 1.6f, withA(A, 0.28f * bo));
        ui::dot(m_btnDraw, c, BTN_R, rgba(10, 13, 22, 0.96f * bo));
        ui::dot(m_btnDraw, c, BTN_R - 3.f, rgba(24, 30, 46, 0.95f * bo));
        ui::arc(m_btnDraw, c, BTN_R - 1.f, 0, 360, 1.5f, withA(A, 0.22f * bo));
        float speed = working ? 260.f : 40.f;
        float ang = m_time * speed;
        ui::arc(m_btnDraw, c, BTN_R - 1.f, ang, 70, 2.6f, withA(A, 0.95f * bo));
        ui::arc(m_btnDraw, c, BTN_R - 1.f, ang + 180.f, 36, 2.6f, withA(A, 0.55f * bo));

        if (st.showRing && working) {
            float pct = std::clamp((s.phase == solver::Phase::Solving || s.phase == solver::Phase::Replaying ? s.best : s.percent) / 100.f, 0.f, 1.f);
            ui::arc(m_btnDraw, c, BTN_R + 5.5f, 0, 360, 3.f, withA(A, 0.16f * bo));
            if (pct > 0.003f) ui::arc(m_btnDraw, c, BTN_R + 5.5f, 90, -360.f * pct, 3.2f, withA(A, 1.f * bo));
        }
        if (m_icon) m_icon->setRotation(0);
    }

    void Hub::drawHud() {
        auto& st = store();
        auto A = ui::accentColor(st.accent);
        auto win = CCDirector::get()->getWinSize();
        std::string text = statusLine();
        if (text.size() > 78) text = text.substr(0, 75) + "...";
        m_hudLbl->setString(text.c_str());
        float w = m_hudLbl->getContentSize().width * m_hudLbl->getScale() + 40.f;
        float h = 22.f;
        m_hud->setPosition(ccp(win.width / 2.f, win.height - 22.f));
        m_hudLbl->setPosition(ccp(8.f, 0));
        m_hudDraw->clear();
        bool working = solver::status().phase != solver::Phase::Idle;
        ui::roundedRect(m_hudDraw, ccp(-w / 2 - 2, -h / 2 - 2), CCSize(w + 4, h + 4), 13, withA(A, 0.10f * st.glow));
        ui::roundedRect(m_hudDraw, ccp(-w / 2, -h / 2), CCSize(w, h), 11, rgba(10, 13, 22, 0.92f), 1.f, withA(A, 0.6f));
        float pulse = working ? 0.55f + 0.45f * std::sin(m_time * 6.f) : 0.9f;
        ui::glowDot(m_hudDraw, ccp(-w / 2 + 12, 0), 3.f, 6.f, A, pulse * st.glow);
        ui::dot(m_hudDraw, ccp(-w / 2 + 12, 0), 3.f, withA(A, pulse));
    }

    // ---------------------------------------------------------------------------------------
    void Hub::drawPanel(float fade) {
        auto& st = store();
        auto A = ui::accentColor(st.accent);
        float op = st.menuOpacity * fade;
        float g = st.glow;
        m_bg->clear();

        // outer glow + body
        ui::roundedRect(m_bg, ccp(-4, -4), CCSize(PW + 8, PH + 8), 19, withA(A, 0.05f * op * g));
        ui::roundedRect(m_bg, ccp(-2, -2), CCSize(PW + 4, PH + 4), 17, withA(A, 0.07f * op * g));
        ui::roundedRect(m_bg, ccp(0, 0), CCSize(PW, PH), 15, rgba(11, 13, 21, 0.97f * op), 1.2f, withA(A, 0.45f * op));
        // sidebar
        ui::roundedRect(m_bg, ccp(0, 0), CCSize(SIDE, PH - HEADER), 15, rgba(6, 8, 14, 0.55f * op), 0.f, {0, 0, 0, 0}, 1);
        ui::capsule(m_bg, ccp(SIDE, 8), ccp(SIDE, PH - HEADER - 8), 0.5f, rgba(255, 255, 255, 0.07f * op));
        // header
        ui::roundedRect(m_bg, ccp(0, PH - HEADER), CCSize(PW, HEADER), 15, rgba(19, 24, 38, 0.99f * op), 0.f, {0, 0, 0, 0}, 12);

        // header accent line with a travelling shimmer
        constexpr int SEG = 40;
        for (int i = 0; i < SEG; ++i) {
            float x0 = PW * i / SEG, x1 = PW * (i + 1) / SEG + 1.f;
            float a = 0.18f + 0.55f * std::sin((float)i / (SEG - 1) * kPi);
            ui::capsule(m_bg, ccp(x0, PH - HEADER), ccp(x1, PH - HEADER), 0.8f, withA(A, a * op));
        }
        float sx = std::fmod(m_time * 130.f, PW + 160.f) - 80.f;
        for (int j = 0; j < 3; ++j) {
            float a0 = std::clamp(sx - 18.f * (j + 1), 4.f, PW - 4.f), a1 = std::clamp(sx + 18.f * (j + 1), 4.f, PW - 4.f);
            if (a1 > a0) ui::capsule(m_bg, ccp(a0, PH - HEADER), ccp(a1, PH - HEADER), 0.8f + j * 1.1f, withA(A, (0.5f - j * 0.17f) * op * g));
        }

        // close button
        CCPoint cc = ccp(PW - 20, PH - HEADER / 2);
        ui::dot(m_bg, cc, 10.f, rgba(255, 255, 255, (m_closePress ? 0.18f : 0.07f) * op));
        ui::capsule(m_bg, cc + ccp(-3.5f, -3.5f), cc + ccp(3.5f, 3.5f), 0.9f, rgba(230, 236, 250, 0.9f * op));
        ui::capsule(m_bg, cc + ccp(-3.5f, 3.5f), cc + ccp(3.5f, -3.5f), 0.9f, rgba(230, 236, 250, 0.9f * op));

        // tabs
        for (size_t i = 0; i < m_cats.size(); ++i) {
            float y = PH - HEADER - 10 - (i + 1) * TABH + 2.f;
            float a = m_tabAnim[i];
            if (a > 0.01f) {
                ui::roundedRect(m_bg, ccp(8, y), CCSize(SIDE - 16, TABH - 4), 8, withA(A, 0.17f * a * op), 1.f, withA(A, 0.35f * a * op));
                ui::capsule(m_bg, ccp(10.5f, y + 6), ccp(10.5f, y + TABH - 10), 1.3f, withA(A, 0.95f * a * op));
                ui::glowDot(m_bg, ccp(10.5f, y + (TABH - 4) / 2), 1.f, 8.f, A, a * g);
            }
            m_tabLbls[i]->setColor(a > 0.5f ? ccColor3B{255, 255, 255} : ccColor3B{150, 160, 185});
        }

        // corner brackets (tech look)
        auto br = withA(A, 0.6f * op);
        float L = 12.f, o = 5.f;
        ui::capsule(m_bg, ccp(o, o), ccp(o + L, o), 0.7f, br);           ui::capsule(m_bg, ccp(o, o), ccp(o, o + L), 0.7f, br);
        ui::capsule(m_bg, ccp(PW - o, o), ccp(PW - o - L, o), 0.7f, br); ui::capsule(m_bg, ccp(PW - o, o), ccp(PW - o, o + L), 0.7f, br);
    }

    float Hub::sliderT(Row const& r) const {
        float v = r.i ? (float)*r.i : (r.f ? *r.f : 0.f);
        return std::clamp((v - r.mn) / std::max(0.0001f, r.mx - r.mn), 0.f, 1.f);
    }

    void Hub::drawRows(float fade) {
        auto& st = store();
        auto A = ui::accentColor(st.accent);
        float op = st.menuOpacity * fade;
        float g = st.glow;
        m_rowsDraw->clear();
        int ri = 0;
        for (auto& r : m_cats[m_cat].rows) {
            auto rc = r.rect;
            float right = rc.origin.x + rc.size.width;
            float cy = rc.origin.y + rc.size.height / 2.f;
            bool pressed = (ri == m_pressRow);
            ui::roundedRect(m_rowsDraw, ccp(rc.origin.x, rc.origin.y + 2), CCSize(rc.size.width, rc.size.height - 4), 8,
                            pressed ? withA(A, 0.13f * op) : rgba(255, 255, 255, 0.035f * op), 0.6f, rgba(255, 255, 255, 0.05f * op));
            switch (r.type) {
                case RT::Toggle: {
                    float a = r.anim;
                    CCPoint t0 = ccp(right - 42, cy), t1 = ccp(right - 20, cy);
                    ui::capsule(m_rowsDraw, t0, t1, 7.5f, ui::lerp(rgba(60, 66, 86, 0.95f * op), withA(A, 0.95f * op), a));
                    CCPoint kn = ccp(t0.x + (t1.x - t0.x) * a, cy);
                    if (a > 0.02f) ui::glowDot(m_rowsDraw, kn, 5.f, 8.f, A, a * g);
                    ui::dot(m_rowsDraw, kn, 5.8f, rgba(245, 248, 255, op));
                    break;
                }
                case RT::Slider: {
                    float x0 = right - 142, x1 = right - 54;
                    float t = sliderT(r);
                    float kx = x0 + (x1 - x0) * t;
                    ui::capsule(m_rowsDraw, ccp(x0, cy), ccp(x1, cy), 2.6f, rgba(60, 66, 86, 0.95f * op));
                    ui::capsule(m_rowsDraw, ccp(x0, cy), ccp(std::max(x0 + 0.1f, kx), cy), 2.6f, withA(A, 0.95f * op));
                    ui::glowDot(m_rowsDraw, ccp(kx, cy), 5.5f, 9.f, A, g * (m_drag == Drag::Slider ? 1.6f : 0.9f));
                    ui::dot(m_rowsDraw, ccp(kx, cy), 6.f, rgba(245, 248, 255, op));
                    break;
                }
                case RT::Segment: {
                    int n = (int)r.opts.size();
                    float start = right - 8 - n * 58;
                    for (int i = 0; i < n; ++i) {
                        bool sel = *r.i == i;
                        CCPoint o = ccp(start + i * 58 + 2, cy - 9);
                        ui::roundedRect(m_rowsDraw, o, CCSize(54, 18), 9, sel ? withA(A, 0.9f * op) : rgba(255, 255, 255, 0.07f * op));
                        if (sel) ui::glowDot(m_rowsDraw, ccp(o.x + 27, cy), 6.f, 14.f, A, 0.6f * g);
                    }
                    break;
                }
                case RT::Cycle: {
                    CCPoint l = ccp(right - 128, cy), rr = ccp(right - 14, cy);
                    ui::dot(m_rowsDraw, l, 9.f, rgba(255, 255, 255, 0.07f * op));
                    ui::dot(m_rowsDraw, rr, 9.f, rgba(255, 255, 255, 0.07f * op));
                    ui::triangle(m_rowsDraw, l + ccp(2.5f, 4.5f), l + ccp(2.5f, -4.5f), l + ccp(-3.5f, 0), withA(A, op));
                    ui::triangle(m_rowsDraw, rr + ccp(-2.5f, 4.5f), rr + ccp(-2.5f, -4.5f), rr + ccp(3.5f, 0), withA(A, op));
                    break;
                }
                case RT::Buttons: {
                    int n = (int)r.btns.size();
                    float bw = (rc.size.width - 20.f - (n - 1) * 6.f) / n;
                    for (int i = 0; i < n; ++i) {
                        auto& b = r.btns[i];
                        bool pr = pressed && m_pressBtn == i;
                        CCPoint o = ccp(rc.origin.x + 10 + i * (bw + 6), rc.origin.y + 5);
                        CCSize sz(bw, rc.size.height - 10);
                        if (b.primary) {
                            ui::glowDot(m_rowsDraw, ccp(o.x + bw / 2, cy), bw * 0.25f, 14.f, A, 0.7f * g);
                            ui::roundedRect(m_rowsDraw, o, sz, 8, withA(A, (pr ? 0.75f : 0.95f) * op));
                            b.lbl->setColor({8, 12, 20});
                        } else {
                            ui::roundedRect(m_rowsDraw, o, sz, 8, rgba(255, 255, 255, (pr ? 0.16f : 0.07f) * op), 1.f, withA(A, 0.45f * op));
                            b.lbl->setColor({235, 240, 252});
                        }
                    }
                    break;
                }
                default: break;
            }
            ++ri;
        }
    }

    // ---------------------------------------------------------------------------------------
    void Hub::sliderSet(Row& r, float lx) {
        float x0 = r.rect.origin.x + r.rect.size.width - 142, x1 = r.rect.origin.x + r.rect.size.width - 54;
        float t = std::clamp((lx - x0) / (x1 - x0), 0.f, 1.f);
        float v = r.mn + t * (r.mx - r.mn);
        if (r.step > 0.f) v = r.mn + std::round((v - r.mn) / r.step) * r.step;
        if (r.i) *r.i = (int)std::lround(v);
        else if (r.f) *r.f = v;
        if (r.onChange) r.onChange();
        refreshTexts();
    }

    bool Hub::beginPanel(CCPoint lp, CCPoint gp) {
        // header: close button or drag handle
        if (lp.y > PH - HEADER) {
            if (ccpDistance(lp, ccp(PW - 20, PH - HEADER / 2)) < 14.f) { m_closePress = true; return true; }
            m_drag = Drag::Panel;
            m_grab = m_panel->getPosition() - gp;
            return true;
        }
        // tabs
        if (lp.x < SIDE) {
            int idx = (int)std::floor((PH - HEADER - 10 - lp.y) / TABH);
            selectCat(idx);
            return true;
        }
        // rows
        auto& rows = m_cats[m_cat].rows;
        for (int i = 0; i < (int)rows.size(); ++i) {
            auto& r = rows[i];
            if (!r.rect.containsPoint(lp)) continue;
            float right = r.rect.origin.x + r.rect.size.width;
            switch (r.type) {
                case RT::Toggle:
                    if (r.b) { *r.b = !*r.b; if (r.onChange) r.onChange(); store().save(); }
                    break;
                case RT::Slider:
                    m_drag = Drag::Slider; m_sliderRow = i; sliderSet(r, lp.x);
                    break;
                case RT::Segment: {
                    int n = (int)r.opts.size();
                    float start = right - 8 - n * 58;
                    int idx = (int)std::floor((lp.x - start) / 58.f);
                    if (idx >= 0 && idx < n) { *r.i = idx; if (r.onChange) r.onChange(); store().save(); }
                    break;
                }
                case RT::Cycle: {
                    int n = std::max(1, (int)r.mx);
                    if (lp.x < right - 71) *r.i = (*r.i - 1 + n) % n;
                    else *r.i = (*r.i + 1) % n;
                    if (r.onChange) r.onChange();
                    store().save();
                    refreshTexts();
                    break;
                }
                case RT::Buttons: {
                    int n = (int)r.btns.size();
                    float bw = (r.rect.size.width - 20.f - (n - 1) * 6.f) / n;
                    int idx = (int)std::floor((lp.x - (r.rect.origin.x + 10)) / (bw + 6.f));
                    if (idx >= 0 && idx < n) { m_pressRow = i; m_pressBtn = idx; }
                    break;
                }
                default: break;
            }
            return true;
        }
        return true;   // swallow taps on empty panel space
    }

    bool Hub::ccTouchBegan(CCTouch* t, CCEvent*) {
        if (!m_shown) return false;
        CCPoint p = t->getLocation();
        m_start = p;
        m_moved = false;
        m_closePress = false;
        m_pressRow = m_pressBtn = -1;

        if (m_open > 0.5f && m_panel->isVisible()) {
            CCPoint lp = m_panel->convertToNodeSpace(p);
            if (CCRect(0, 0, PW, PH).containsPoint(lp)) return beginPanel(lp, p);
        }
        float r = BTN_R * store().buttonScale + 6.f;
        if (ccpDistance(p, m_btn->getPosition()) <= r) {
            m_drag = Drag::Button;
            m_grab = m_btn->getPosition() - p;
            return true;
        }
        return false;
    }

    void Hub::ccTouchMoved(CCTouch* t, CCEvent*) {
        CCPoint p = t->getLocation();
        if (ccpDistance(p, m_start) > 8.f) m_moved = true;
        auto win = CCDirector::get()->getWinSize();
        if (m_drag == Drag::Button && m_moved) {
            float r = BTN_R * store().buttonScale;
            CCPoint q = p + m_grab;
            m_btnTarget = ccp(std::clamp(q.x, r, win.width - r), std::clamp(q.y, r, win.height - r));
        } else if (m_drag == Drag::Panel) {
            float hw = PW * store().menuScale / 2.f, hh = PH * store().menuScale / 2.f;
            CCPoint q = p + m_grab;
            m_panel->setPosition(ccp(std::clamp(q.x, hw * 0.4f, win.width - hw * 0.4f), std::clamp(q.y, hh * 0.4f, win.height - hh * 0.4f)));
        } else if (m_drag == Drag::Slider && m_sliderRow >= 0) {
            CCPoint lp = m_panel->convertToNodeSpace(p);
            sliderSet(m_cats[m_cat].rows[m_sliderRow], lp.x);
        }
    }

    void Hub::ccTouchEnded(CCTouch* t, CCEvent*) {
        CCPoint p = t->getLocation();
        auto& st = store();
        auto win = CCDirector::get()->getWinSize();

        if (m_drag == Drag::Button) {
            if (!m_moved) togglePanel();
            else {
                if (st.snapEdge) {
                    float r = BTN_R * st.buttonScale + 4.f;
                    m_btnTarget.x = (m_btnTarget.x < win.width / 2.f) ? r : win.width - r;
                }
                st.hubX = m_btnTarget.x / win.width;
                st.hubY = m_btnTarget.y / win.height;
                st.save();
            }
        } else if (m_drag == Drag::Panel) {
            st.panelX = m_panel->getPositionX() / win.width;
            st.panelY = m_panel->getPositionY() / win.height;
            st.save();
        } else if (m_drag == Drag::Slider) {
            st.save();
        }

        if (m_closePress) {
            CCPoint lp = m_panel->convertToNodeSpace(p);
            if (ccpDistance(lp, ccp(PW - 20, PH - HEADER / 2)) < 16.f) m_openTarget = false;
        }
        if (m_pressRow >= 0 && m_pressBtn >= 0) {
            CCPoint lp = m_panel->convertToNodeSpace(p);
            auto& r = m_cats[m_cat].rows[m_pressRow];
            if (r.rect.containsPoint(lp)) {
                auto fn = r.btns[m_pressBtn].fn;   // copy: the callback may rebuild rows
                m_pressRow = m_pressBtn = -1;
                if (fn) fn();
            }
        }
        m_drag = Drag::None;
        m_closePress = false;
        m_pressRow = m_pressBtn = -1;
        m_sliderRow = -1;
    }

    void Hub::ccTouchCancelled(CCTouch* t, CCEvent* e) { ccTouchEnded(t, e); }
}

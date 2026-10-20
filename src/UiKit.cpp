#include "UiKit.hpp"
#include <cmath>
#include <vector>

using namespace geode::prelude;

namespace al::ui {
    namespace {
        struct Accent { char const* name; ccColor4F color; };
        const Accent kAccents[] = {
            {"Cyan",    rgba(0, 194, 255)},
            {"Violet",  rgba(139, 92, 246)},
            {"Emerald", rgba(34, 197, 94)},
            {"Amber",   rgba(245, 158, 11)},
            {"Rose",    rgba(236, 72, 153)},
            {"Crimson", rgba(239, 68, 68)},
            {"Ice",     rgba(180, 220, 255)},
        };
        constexpr float kPi = 3.14159265358979f;
    }

    int accentCount() { return (int)(sizeof(kAccents) / sizeof(kAccents[0])); }
    char const* accentName(int i) { return kAccents[((i % accentCount()) + accentCount()) % accentCount()].name; }
    ccColor4F accentColor(int i) { return kAccents[((i % accentCount()) + accentCount()) % accentCount()].color; }

    void blend(CCDrawNode* n) {
        ccBlendFunc bf{GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
        n->setBlendFunc(bf);
    }

    void roundedRect(CCDrawNode* n, CCPoint o, CCSize s, float r, ccColor4F fill, float bw, ccColor4F border, int corners) {
        r = std::min(r, std::min(s.width, s.height) / 2.f);
        std::vector<CCPoint> v;
        v.reserve(40);
        constexpr int N = 7;
        auto corner = [&](CCPoint c, float a0, bool round, CCPoint sharp) {
            if (!round) { v.push_back(sharp); return; }
            for (int i = 0; i <= N; ++i) {
                float a = (a0 + 90.f * i / N) * kPi / 180.f;
                v.push_back(ccp(c.x + std::cos(a) * r, c.y + std::sin(a) * r));
            }
        };
        corner(ccp(o.x + r, o.y + r), 180, corners & 1, ccp(o.x, o.y));
        corner(ccp(o.x + s.width - r, o.y + r), 270, corners & 2, ccp(o.x + s.width, o.y));
        corner(ccp(o.x + s.width - r, o.y + s.height - r), 0, corners & 4, ccp(o.x + s.width, o.y + s.height));
        corner(ccp(o.x + r, o.y + s.height - r), 90, corners & 8, ccp(o.x, o.y + s.height));
        n->drawPolygon(v.data(), (unsigned)v.size(), fill, bw, border);
    }

    void capsule(CCDrawNode* n, CCPoint a, CCPoint b, float r, ccColor4F c) { n->drawSegment(a, b, r, c); }
    void dot(CCDrawNode* n, CCPoint c, float r, ccColor4F col) { n->drawDot(c, r, col); }

    void glowDot(CCDrawNode* n, CCPoint c, float radius, float spread, ccColor4F color, float strength) {
        constexpr int L = 6;
        for (int i = 0; i < L; ++i) {
            float k = (float)(L - i) / L;                 // outer layers first
            n->drawDot(c, radius + spread * k, withA(color, strength * 0.07f * (1.f - k * 0.55f)));
        }
    }

    void arc(CCDrawNode* n, CCPoint c, float r, float startDeg, float sweepDeg, float width, ccColor4F color) {
        int segs = std::max(2, (int)(std::fabs(sweepDeg) / 6.f));
        CCPoint prev = ccp(c.x + std::cos(startDeg * kPi / 180.f) * r, c.y + std::sin(startDeg * kPi / 180.f) * r);
        for (int i = 1; i <= segs; ++i) {
            float a = (startDeg + sweepDeg * i / segs) * kPi / 180.f;
            CCPoint p = ccp(c.x + std::cos(a) * r, c.y + std::sin(a) * r);
            n->drawSegment(prev, p, width * 0.5f, color);
            prev = p;
        }
    }

    void triangle(CCDrawNode* n, CCPoint a, CCPoint b, CCPoint c, ccColor4F color) {
        CCPoint v[3] = {a, b, c};
        n->drawPolygon(v, 3, color, 0.f, {0, 0, 0, 0});
    }

    CCLabelBMFont* label(CCNode* parent, std::string const& str, char const* font, float scale, ccColor3B color,
                         CCPoint pos, CCPoint anchor, int z) {
        auto l = CCLabelBMFont::create(str.c_str(), font);
        l->setScale(scale);
        l->setColor(color);
        l->setAnchorPoint(anchor);
        l->setPosition(pos);
        parent->addChild(l, z);
        return l;
    }
}

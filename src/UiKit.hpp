#pragma once
#include <Geode/Geode.hpp>
#include <string>

// Small drawing toolkit for the ALComplete menu (rounded shapes, glows, arcs, labels).
namespace al::ui {
    using namespace cocos2d;

    inline ccColor4F rgba(int r, int g, int b, float a = 1.f) { return {r / 255.f, g / 255.f, b / 255.f, a}; }
    inline ccColor4F withA(ccColor4F c, float a) { c.a = a; return c; }
    inline ccColor4F scaleA(ccColor4F c, float k) { c.a *= k; return c; }
    inline ccColor4F lerp(ccColor4F a, ccColor4F b, float t) {
        return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
    }
    inline ccColor3B to3B(ccColor4F c) {
        return {(GLubyte)(c.r * 255), (GLubyte)(c.g * 255), (GLubyte)(c.b * 255)};
    }

    int accentCount();
    char const* accentName(int i);
    ccColor4F accentColor(int i);

    void blend(CCDrawNode* n);

    // corners: bit0 bottom-left, bit1 bottom-right, bit2 top-right, bit3 top-left (15 = all rounded)
    void roundedRect(CCDrawNode* n, CCPoint origin, CCSize size, float radius, ccColor4F fill,
                     float borderWidth = 0.f, ccColor4F border = {0, 0, 0, 0}, int corners = 15);
    void capsule(CCDrawNode* n, CCPoint a, CCPoint b, float radius, ccColor4F color);
    void dot(CCDrawNode* n, CCPoint c, float radius, ccColor4F color);
    void glowDot(CCDrawNode* n, CCPoint c, float radius, float spread, ccColor4F color, float strength);
    void arc(CCDrawNode* n, CCPoint center, float radius, float startDeg, float sweepDeg, float width, ccColor4F color);
    void triangle(CCDrawNode* n, CCPoint a, CCPoint b, CCPoint c, ccColor4F color);

    CCLabelBMFont* label(CCNode* parent, std::string const& str, char const* font, float scale, ccColor3B color,
                         CCPoint pos, CCPoint anchor, int z = 2);
}

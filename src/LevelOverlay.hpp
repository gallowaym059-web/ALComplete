#pragma once
#include <Geode/Geode.hpp>
#include <vector>

namespace al::lvl {
    void init(PlayLayer* pl);
    void reset();
    void clear();
    void draw(std::vector<cocos2d::CCPoint> const& path, std::vector<cocos2d::CCPoint> const& deaths);
}

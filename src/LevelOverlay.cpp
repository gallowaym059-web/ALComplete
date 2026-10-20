#include "LevelOverlay.hpp"
#include "Store.hpp"
#include "UiKit.hpp"

using namespace geode::prelude;

namespace al::lvl {
    static CCDrawNode* s_node = nullptr;

    void init(PlayLayer* pl) {
        s_node = CCDrawNode::create();
        ui::blend(s_node);
        pl->m_objectLayer->addChild(s_node, 500);
    }

    void reset() { s_node = nullptr; }
    void clear() { if (s_node) s_node->clear(); }

    void draw(std::vector<CCPoint> const& path, std::vector<CCPoint> const& deaths) {
        if (!s_node) return;
        s_node->clear();
        auto& st = store();
        auto accent = ui::accentColor(st.accent);
        if (st.showPath && path.size() > 1) {
            size_t stride = std::max<size_t>(1, path.size() / 900);
            CCPoint prev = path[0];
            for (size_t i = stride; i < path.size(); i += stride) {
                s_node->drawSegment(prev, path[i], 0.9f, ui::withA(accent, 0.9f));
                prev = path[i];
            }
        }
        if (st.showDeaths) {
            for (auto const& d : deaths) {
                s_node->drawSegment(d + ccp(-3, -3), d + ccp(3, 3), 0.7f, {1.f, 0.25f, 0.3f, 0.85f});
                s_node->drawSegment(d + ccp(-3, 3), d + ccp(3, -3), 0.7f, {1.f, 0.25f, 0.3f, 0.85f});
            }
        }
    }
}

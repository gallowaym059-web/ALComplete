#include "Store.hpp"

using namespace geode::prelude;

namespace al {
    Store& store() { static Store s; return s; }

#define AL_FIELDS(X) \
    X(menuOpacity) X(menuScale) X(buttonScale) X(buttonOpacity) X(glow) X(snapEdge) X(animations) \
    X(accent) X(icon) X(hubX) X(hubY) X(panelX) X(panelY) X(budgetMs) X(ticksPerStep) \
    X(maxCheckpoints) X(lookahead) X(giveUpSec) X(route) X(autoSolve) X(autoPlay) X(verify) \
    X(showPath) X(showDeaths) X(showHud) X(showRing) X(safeMode) X(restorePractice) X(verbose)

    void Store::load() {
        auto* m = Mod::get();
#define AL_LOAD(f) f = m->getSavedValue<std::remove_cvref_t<decltype(f)>>("s-" #f, f);
        AL_FIELDS(AL_LOAD)
#undef AL_LOAD
    }

    void Store::save() const {
        auto* m = Mod::get();
#define AL_SAVE(f) m->setSavedValue<std::remove_cvref_t<decltype(f)>>("s-" #f, f);
        AL_FIELDS(AL_SAVE)
#undef AL_SAVE
    }
}

$on_mod(Loaded) { al::store().load(); }

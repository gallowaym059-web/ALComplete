#pragma once
#include <Geode/Geode.hpp>
#include <functional>
#include <string>
#include <vector>

namespace al {
    // The floating, draggable circle + the animated categorised menu + the solver HUD.
    class Hub : public cocos2d::CCLayer {
    public:
        static Hub* get();
        static void ensure();
        void setShown(bool shown);

    private:
        enum class RT { Toggle, Slider, Segment, Cycle, Buttons, Info };
        enum class Drag { None, Button, Panel, Slider };

        struct Btn {
            std::string text;
            std::function<void()> fn;
            bool primary = false;
            cocos2d::CCLabelBMFont* lbl = nullptr;
        };
        struct Row {
            RT type = RT::Info;
            std::string name;
            bool* b = nullptr;
            float* f = nullptr;
            int* i = nullptr;
            float mn = 0.f, mx = 1.f, step = 0.f;
            std::vector<std::string> opts;
            std::vector<Btn> btns;
            std::function<std::string()> info;        // Info rows
            std::function<std::string()> valueText;   // Slider / Cycle value
            std::function<void()> onChange;
            int span = 1;
            cocos2d::CCRect rect;
            cocos2d::CCLabelBMFont* nameLbl = nullptr;
            cocos2d::CCLabelBMFont* valLbl = nullptr;
            std::string cache;
            float anim = 0.f;
        };
        struct Cat { std::string name; std::vector<Row> rows; };
        struct Lbl { cocos2d::CCLabelBMFont* l; float base; };

        bool init() override;
        void update(float dt) override;
        void registerWithTouchDispatcher() override;
        bool ccTouchBegan(cocos2d::CCTouch* t, cocos2d::CCEvent* e) override;
        void ccTouchMoved(cocos2d::CCTouch* t, cocos2d::CCEvent* e) override;
        void ccTouchEnded(cocos2d::CCTouch* t, cocos2d::CCEvent* e) override;
        void ccTouchCancelled(cocos2d::CCTouch* t, cocos2d::CCEvent* e) override;

        void buildCats();
        void buildPanel();
        void selectCat(int idx);
        void rebuildRows();
        void rebuildIcon();
        void applyAppearance();
        void applyPositions();
        void togglePanel();

        void drawButton();
        void drawPanel(float fade);
        void drawRows(float fade);
        void drawHud();
        void refreshTexts();

        void sliderSet(Row& r, float localX);
        float sliderT(Row const& r) const;
        bool beginPanel(cocos2d::CCPoint local, cocos2d::CCPoint global);
        void endPress(cocos2d::CCPoint local);
        void doSolve(bool autoplay);
        std::string statusLine();

        std::vector<Cat> m_cats;
        int m_cat = 0;

        cocos2d::CCNode* m_btn = nullptr;
        cocos2d::CCDrawNode* m_btnDraw = nullptr;
        cocos2d::CCNode* m_icon = nullptr;

        cocos2d::CCNode* m_panel = nullptr;
        cocos2d::CCDrawNode* m_bg = nullptr;
        cocos2d::CCNode* m_rows = nullptr;
        cocos2d::CCDrawNode* m_rowsDraw = nullptr;
        std::vector<Lbl> m_lbls;                   // fade-managed labels
        std::vector<cocos2d::CCLabelBMFont*> m_tabLbls;
        std::vector<float> m_tabAnim;

        cocos2d::CCNode* m_hud = nullptr;
        cocos2d::CCDrawNode* m_hudDraw = nullptr;
        cocos2d::CCLabelBMFont* m_hudLbl = nullptr;
        std::string m_lastMsg;
        float m_hudUntil = 0.f;

        cocos2d::CCPoint m_btnTarget;
        float m_time = 0.f;
        float m_open = 0.f;
        bool m_openTarget = false;
        bool m_shown = true;

        Drag m_drag = Drag::None;
        cocos2d::CCPoint m_start, m_grab;
        bool m_moved = false;
        bool m_closePress = false;
        int m_pressRow = -1, m_pressBtn = -1;
        int m_sliderRow = -1;
        float m_pressAnim = 0.f;
    };
}

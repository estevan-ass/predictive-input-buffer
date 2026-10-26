
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/utils/cocos.hpp>

#include <deque>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace pib {
    struct Stage {
        int all;
        int wave;
    };

    constexpr Stage STAGES[4] = {
        {2, 2}, {4, 4}, {8, 8}, {16, 8}
    };

    constexpr int OFF = 4;
    inline int g_stage = 0;
    inline bool g_consoleVisible = true;
    inline std::deque<std::string> g_messages;

    inline void addMessage(std::string const& message) {
        g_messages.push_back(message);
        while (g_messages.size() > 8) {
            g_messages.pop_front();
        }
        log::info("[InputBuffer] {}", message);
    }

    inline void logStage() {
        if (g_stage == OFF) {
            addMessage("F6 -> OFF (Raw Input)");
        } else {
            auto s = STAGES[g_stage];
            addMessage(
                "F6 -> STAGE " + std::to_string(g_stage + 1) +
                ": " + std::to_string(s.all) + "/" +
                std::to_string(s.wave) + " ticks"
            );
        }
    }
}

class PIBConsole : public CCLayer {
protected:
    CCLabelTTF* m_label = nullptr;
    CCLayerColor* m_background = nullptr;

public:
    static PIBConsole* create() {
        auto ret = new PIBConsole();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool init() override {
        if (!CCLayer::init()) return false;

        auto win = CCDirector::sharedDirector()->getWinSize();

        m_background = CCLayerColor::create(
            ccc4(0, 0, 0, 220),
            win.width * 0.58f,
            112.f
        );
        m_background->setAnchorPoint({0.f, 1.f});
        m_background->setPosition({8.f, win.height - 8.f});
        this->addChild(m_background, 0);

        m_label = CCLabelTTF::create(
            "Predictive Input Buffer",
            "Arial",
            12.f
        );
        m_label->setAnchorPoint({0.f, 1.f});
        m_label->setHorizontalAlignment(kCCTextAlignmentLeft);
        m_label->setVerticalAlignment(kCCVerticalTextAlignmentTop);
        m_label->setPosition({8.f, 102.f});
        m_background->addChild(m_label, 1);

        this->schedule(schedule_selector(PIBConsole::refresh), 0.2f);
        refresh(0.f);
        return true;
    }

    void refresh(float) {
        if (!m_label) return;

        std::string text = "PREDICTIVE INPUT BUFFER\n";
        text += "F6: change stage | F7: show/hide\n";

        for (auto const& msg : pib::g_messages) {
            text += "> " + msg + "\n";
        }

        m_label->setString(text.c_str());
    }
};

class $modify(PIBPlayLayer, PlayLayer) {
    struct Fields {
        PIBConsole* console = nullptr;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;

        auto console = PIBConsole::create();
        if (console) {
            this->addChild(console, 10000);
            m_fields->console = console;
        }
        return true;
    }
};

class $modify(PIBBaseGameLayer, GJBaseGameLayer) {
    struct Fields {
        std::deque<std::tuple<bool, int, bool, int>> queue;
        bool applying = false;
    };

    void handleButton(bool down, int button, bool isPlayer1) {
        auto f = m_fields.self();

        if (f->applying || pib::g_stage == pib::OFF) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        auto* player = isPlayer1 ? m_player1 : m_player2;
        bool isWave = player && player->m_isDart;
        auto stage = pib::STAGES[pib::g_stage];
        int delay = isWave ? stage.wave : stage.all;

        f->queue.push_back({down, button, isPlayer1, delay});
        pib::addMessage(
            std::string(down ? "PRESS queued +" : "RELEASE queued +") +
            std::to_string(delay) + " ticks"
        );
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        auto f = m_fields.self();

        for (auto it = f->queue.begin(); it != f->queue.end();) {
            auto& [down, button, player1, remaining] = *it;
            --remaining;

            if (remaining <= 0) {
                f->applying = true;
                GJBaseGameLayer::handleButton(down, button, player1);
                f->applying = false;
                it = f->queue.erase(it);
            } else {
                ++it;
            }
        }

        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }
};

class $modify(PIBKeyboard, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(
        enumKeyCodes key, bool down, bool repeat, double timestamp
    ) {
        if (down && !repeat && key == KEY_F6) {
            pib::g_stage = (pib::g_stage + 1) % 5;
            pib::logStage();
            return true;
        }

        if (down && !repeat && key == KEY_F7) {
            pib::g_consoleVisible = !pib::g_consoleVisible;
            return true;
        }

        return CCKeyboardDispatcher::dispatchKeyboardMSG(
            key, down, repeat, timestamp
        );
    }
};

$on_mod(Loaded) {
    pib::addMessage("v4.7 LOADED");
    pib::addMessage("F6: cycle stages | F7: console visibility");
    pib::addMessage("Starting at STAGE 1 (2/2 ticks)");
}

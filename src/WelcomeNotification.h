#include <Geode/Geode.hpp>
#include "SystemNotification.h"
#include "StreakData.h" 

using namespace geode::prelude;
bool g_hasWelcomed = false;
class $modify(WelcomeLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto am = GJAccountManager::sharedState();
        if (!g_hasWelcomed && am && am->m_accountID > 0 && !g_streakData.needsRegistration) {
            this->runAction(CCSequence::create(
                CCDelayTime::create(0.5f),
                CCCallFunc::create(this, callfunc_selector(WelcomeLayer::showWelcomeNotif)),
                nullptr
            ));         
            g_hasWelcomed = true;
        }

        return true;
    }
    void showWelcomeNotif() {
        auto am = GJAccountManager::sharedState();
        std::string playerName = am->m_username;
        if (playerName.empty()) playerName = "Dasher";

        SystemNotification::show(
            "Welcome!",
            fmt::format("Hi, {}", playerName),
            "face_ui.png"_spr
        );
    }
};
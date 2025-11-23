#pragma once
#include "StreakCommon.h"
#include "../StreakData.h"
#include "../FirebaseManager.h"
#include <Geode/ui/Popup.hpp>
#include "HistoryPopup.h"
#include "StatsPopups.h"
#include "CollectionPopups.h"
#include "MissionsPopup.h"
#include "AdminPopups.h"
#include "LeaderboardPopup.h"
#include "MailPopups.h"
#include "EventPopup.h"
#include "LevelProgressPopup.h"
#include "RoulettePopup.h"
#include "SendMessagePopup.h"

class InfoPopup : public Popup<> {
protected:
    CCLabelBMFont* m_streakLabel = nullptr;
    CCLayerGradient* m_barFg = nullptr;
    CCLabelBMFont* m_barText = nullptr;
    CCSprite* m_rachaIndicator = nullptr;

    void onCreateProfile(CCObject*) {
        auto am = GJAccountManager::sharedState();
        if (!am || am->m_accountID == 0) {
            FLAlertLayer::create("Error", "Invalid account data. Please log in again.", "OK")->show();
            return;
        }


        g_streakData.resetToDefault();
        g_streakData.needsRegistration = false;
        g_streakData.dailyUpdate();
        updatePlayerDataInFirebase();
        this->onClose(nullptr);
        FLAlertLayer::create("Success", "Profile created! Please open the menu again.", "OK")->show();
    }

    bool setup() override {
        auto am = GJAccountManager::sharedState();
        if (!am || am->m_accountID == 0) {
            this->setTitle("Error"); 
            auto winSize = m_mainLayer->getContentSize();
            auto errorLabel = CCLabelBMFont::create(
                "Please log in to\nGeometry Dash first!",
                "bigFont.fnt"
            );

            errorLabel->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);
            errorLabel->setScale(0.5f);
            errorLabel->setPosition(winSize / 2);
            errorLabel->setColor({ 255, 100, 100 });
            m_mainLayer->addChild(errorLabel);
            return true;
        }

        if (g_streakData.needsRegistration) {
            this->setTitle("Welcome!");
            auto winSize = m_mainLayer->getContentSize();
            auto infoLabel = CCLabelBMFont::create(
                "Join the Streak Mod!", 
                "goldFont.fnt"
            );

            infoLabel->setPosition({ winSize.width / 2, winSize.height / 2 + 25.f });
            infoLabel->setScale(0.7f); m_mainLayer->addChild(infoLabel);
            auto createBtn = CCMenuItemSpriteExtra::create(
                ButtonSprite::create(
                    "Create Profile",
                    0,
                    0,
                    "goldFont.fnt",
                    "GJ_button_01.png",
                    0,
                    1.0f
                ),

                this,
                menu_selector(InfoPopup::onCreateProfile)
            );

            auto menu = CCMenu::createWithItem(createBtn);
            menu->setPosition({
                winSize.width / 2,
                winSize.height / 2 - 35.f
                });
            m_mainLayer->addChild(menu);
            return true;
        }

        this->setTitle("My Streak");
        auto winSize = m_mainLayer->getContentSize();
        float centerY = winSize.height / 2 + 25;

        auto rachaSprite = CCSprite::create(g_streakData.getRachaSprite().c_str());
        if (rachaSprite) {
            rachaSprite->setScale(0.4f);
            auto rachaBtn = CCMenuItemSpriteExtra::create(rachaSprite, this, menu_selector(InfoPopup::onRachaClick));
            auto menuRacha = CCMenu::createWithItem(rachaBtn);
            menuRacha->setPosition({
                winSize.width / 2,
                centerY 
                }); 

            m_mainLayer->addChild(menuRacha, 3);
            rachaSprite->runAction(CCRepeatForever::create(
                CCSequence::create(CCMoveBy::create(
                    1.5f, { 0, 8 }),
                    CCMoveBy::create(1.5f, { 0, -8 }),
                    nullptr))
            );
        }

        m_streakLabel = CCLabelBMFont::create("Daily streak: ?", "goldFont.fnt");
        m_streakLabel->setScale(0.55f);
        m_streakLabel->setPosition({ winSize.width / 2, centerY - 60 });
        m_mainLayer->addChild(m_streakLabel);

        float barWidth = 140.0f;
        float barHeight = 16.0f;
        auto barBg = CCLayerColor::create(
            { 45, 45, 45, 255 },
            barWidth, barHeight
        );

        barBg->setPosition({
            winSize.width / 2 - barWidth / 2, centerY - 90
            });

        m_mainLayer->addChild(barBg, 1);
        m_barFg = CCLayerGradient::create(
            { 250, 225, 60, 255 },
            { 255, 165, 0, 255 }
        );

        m_barFg->setContentSize({ 0, barHeight });
        m_barFg->setPosition({ winSize.width / 2 - barWidth / 2, centerY - 90 });
        m_mainLayer->addChild(m_barFg, 2);
        auto border = CCLayerColor::create({ 255, 255, 255, 120 }, barWidth + 2, barHeight + 2);
        border->setPosition({ winSize.width / 2 - barWidth / 2 - 1, centerY - 91 });
        m_mainLayer->addChild(border, 4);
        auto outer = CCLayerColor::create({ 0, 0, 0, 70 }, barWidth + 6, barHeight + 6);
        outer->setPosition({ winSize.width / 2 - barWidth / 2 - 3, centerY - 93 }); 
        m_mainLayer->addChild(outer, 0);

        auto pointIcon = CCSprite::create("streak_point.png"_spr);
        pointIcon->setScale(0.12f);
        pointIcon->setPosition({ winSize.width / 2 - 35, centerY - 82 });
        m_mainLayer->addChild(pointIcon, 5);
        m_barText = CCLabelBMFont::create("? / ?", "bigFont.fnt");
        m_barText->setScale(0.45f);
        m_barText->setPosition({ winSize.width / 2 + 10, centerY - 82 });
        m_mainLayer->addChild(m_barText, 5);
        m_rachaIndicator = CCSprite::create("racha0.png"_spr);
        m_rachaIndicator->setScale(0.14f);
        m_rachaIndicator->setPosition({ winSize.width / 2 + barWidth / 2 + 20, centerY - 82 });
        m_mainLayer->addChild(m_rachaIndicator, 5);

        auto cornerMenu = CCMenu::create(); 
        cornerMenu->setPosition(0, 0);
        m_mainLayer->addChild(cornerMenu, 10);
        auto statsIcon = CCSprite::create("BtnStats.png"_spr);
        statsIcon->setScale(0.7f);

        auto statsBtn = CCMenuItemSpriteExtra::create(
            statsIcon,
            this,
            menu_selector(InfoPopup::onOpenStats
            ));

        statsBtn->setPosition({ winSize.width - 22, centerY }); 
        cornerMenu->addChild(statsBtn);
        auto rewardsIcon = CCSprite::create("RewardsBtn.png"_spr); 
        rewardsIcon->setScale(0.7f);

        auto rewardsBtn = CCMenuItemSpriteExtra::create(
            rewardsIcon,
            this, 
            menu_selector(InfoPopup::onOpenRewards
            ));

        rewardsBtn->setPosition({ winSize.width - 22, centerY - 37 });
        cornerMenu->addChild(rewardsBtn);
        auto missionsIcon = CCSprite::create("super_star_btn.png"_spr);
        missionsIcon->setScale(0.7f);

        auto missionsBtn = CCMenuItemSpriteExtra::create(
            missionsIcon, 
            this,
            menu_selector(InfoPopup::onOpenMissions
            )); 

        missionsBtn->setPosition({ 22, centerY });
        cornerMenu->addChild(missionsBtn);
        auto rouletteIcon = CCSprite::create("boton_ruleta.png"_spr); 
        rouletteIcon->setScale(0.7f); 

        auto rouletteBtn = CCMenuItemSpriteExtra::create(
            rouletteIcon, 
            this, 
            menu_selector(InfoPopup::onOpenRoulette
            ));

        rouletteBtn->setPosition({ 22, centerY - 37 }); 
        cornerMenu->addChild(rouletteBtn);
        auto infoIcon = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        infoIcon->setScale(0.6f); 
        auto infoBtn = CCMenuItemSpriteExtra::create(
            infoIcon, 
            this, 
            menu_selector(InfoPopup::onInfo
            ));

        infoBtn->setPosition({ winSize.width - 20, winSize.height - 20 });
        cornerMenu->addChild(infoBtn);

        auto bottomMenu = CCMenu::create();
        m_mainLayer->addChild(bottomMenu, 10);
        auto eventIcon = CCSprite::create("event_boton.png"_spr);
        if (!eventIcon || eventIcon->getContentSize().width == 0) eventIcon = CCSprite::createWithSpriteFrameName("GJ_top100Btn_001.png");
        eventIcon->setScale(0.7f);

        bottomMenu->addChild(CCMenuItemSpriteExtra::create(
            eventIcon, 
            this,
            menu_selector(InfoPopup::onOpenEvent
            )));

        auto historyIcon = CCSprite::create("historial_btn.png"_spr);
        historyIcon->setScale(0.7f);

        bottomMenu->addChild(CCMenuItemSpriteExtra::create(
            historyIcon,
            this,
            menu_selector(InfoPopup::onOpenHistory)
        ));

        auto levelProgIcon = CCSprite::create("level_progess_btn.png"_spr);
        if (!levelProgIcon) levelProgIcon = ButtonSprite::create("Lvls"); 
        else levelProgIcon->setScale(0.7f);

        bottomMenu->addChild(CCMenuItemSpriteExtra::create(
            levelProgIcon,
            this, 
            menu_selector(InfoPopup::onOpenLevelProgress
            )));

        auto topIcon = CCSprite::create("top_btn.png"_spr);
        if (!topIcon) topIcon = ButtonSprite::create("Top");
        else topIcon->setScale(0.7f);

        bottomMenu->addChild(CCMenuItemSpriteExtra::create(
            topIcon,
            this, 
            menu_selector(InfoPopup::onOpenLeaderboard
            )));

        auto msgIcon = CCSprite::create("msm.png"_spr);
        if (!msgIcon) msgIcon = CCSprite::createWithSpriteFrameName("GJ_chatBtn_001.png");
        msgIcon->setScale(0.7f);

        bottomMenu->addChild(CCMenuItemSpriteExtra::create(
            msgIcon,
            this, 
            menu_selector(InfoPopup::onOpenMessages
            )));

        auto redeemIcon = CCSprite::create("redemcode_btn.png"_spr);
        redeemIcon->setScale(0.7f);

        bottomMenu->addChild(CCMenuItemSpriteExtra::create(
            redeemIcon, 
            this, 
            menu_selector(InfoPopup::onRedeemCode
            )));

        auto kofiIcon = CCSprite::create("ko-fi_btn.png"_spr); 
        if (!kofiIcon) kofiIcon = ButtonSprite::create("Donate");
        else kofiIcon->setScale(0.7f); 

        bottomMenu->addChild(CCMenuItemSpriteExtra::create(
            kofiIcon,
            this, 
            menu_selector(InfoPopup::onOpenDonations
            )));

        bottomMenu->alignItemsHorizontallyWithPadding(5.0f);
        bottomMenu->setPosition({ winSize.width / 2, 25.f });

        this->updateDisplay();
        if (g_streakData.shouldShowAnimation()) {
            this->showStreakAnimation(g_streakData.currentStreak); 
            g_streakData.lastStreakAnimated = g_streakData.currentStreak;
            g_streakData.save();

        }

        return true;
    }

    void updateDisplay() {
        if (!m_mainLayer || !m_streakLabel || !m_barFg || !m_barText || !m_rachaIndicator) { 
            return;
        }

        int currentStreak = g_streakData.currentStreak;
        int pointsToday = g_streakData.streakPointsToday;
        int requiredPoints = g_streakData.getRequiredPoints();
        float percent = (requiredPoints > 0) ? std::min(static_cast<float>(pointsToday) / requiredPoints, 1.0f) : 0.f;
        float barWidth = 140.0f;
        float barHeight = 16.0f;


        m_streakLabel->setString(fmt::format("Daily streak: {}", currentStreak).c_str());
        m_barFg->setContentSize({
            barWidth * percent, barHeight
            });

        m_barText->setString(fmt::format("{}/{}", pointsToday, requiredPoints).c_str());
        std::string indicatorSpriteName = (pointsToday >= requiredPoints) ? g_streakData.getRachaSprite() : "racha0.png"_spr;
        if (auto newSprite = CCSprite::create(indicatorSpriteName.c_str())) {
        if (auto newTexture = newSprite->getTexture()) {
        m_rachaIndicator->setTexture(newTexture);
        m_rachaIndicator->setTextureRect(
            newSprite->getTextureRect(),
            newSprite->isTextureRectRotated(),
            newSprite->getContentSize()); 
        }
       }
     }

    void onOpenHistory(CCObject*) {
        HistoryPopup::create()->show();
    }

    void onOpenStats(CCObject*) {
        DayProgressPopup::create()->show();
    }

    void onOpenRewards(CCObject*) {
        RewardsPopup::create()->show();
    }

    void onRachaClick(CCObject*) {
        AllRachasPopup::create()->show();
    }

    void onOpenMissions(CCObject*) {
        MissionsPopup::create()->show(); 
    }

    void onRedeemCode(CCObject*) {
        RedeemCodePopup::create()->show();
    }

    void onOpenLeaderboard(CCObject*) { 
        LeaderboardPopup::create()->show();
    }

    void onOpenMessages(CCObject*) {
        SendMessagePopup::create()->show(); 
    }

    void onOpenDonations(CCObject*) { 
        DonationPopup::create()->show();
    }

    void onOpenEvent(CCObject*) { 
        EventPopup::create()->show();
    }

    void onOpenLevelProgress(CCObject*) { 
        LevelProgressPopup::create()->show();
    }

    void onInfo(CCObject*) {
        FLAlertLayer::create("About Streak!",
            "Complete rated levels to earn points!\nAut/Easy/Norm: 1pt\nHard: 3pt\nHarder: 4pt\nInsane: 5pt\nDemon: 6pt", 
            "OK")->show(); 
    }

    void onOpenRoulette(CCObject*) {
        if (g_streakData.currentStreak < 1) { 
            FLAlertLayer::create("Roulette Locked", "You need a streak of at least <cg>1 day</c>.", "OK")->show();
            return; 
        } 
        RoulettePopup::create()->show();
    }

    void showStreakAnimation(int streakLevel) {

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto bgLayer = CCLayerColor::create({ 0, 0, 0, 0 });
        bgLayer->setTag(110);
        this->addChild(bgLayer, 1000);
        bgLayer->runAction(CCFadeTo::create(0.5f, 200));
        auto contentLayer = CCLayer::create();
        contentLayer->setTag(111);
        contentLayer->ignoreAnchorPointForPosition(false);
        contentLayer->setAnchorPoint({ 0.5f, 0.5f });
        contentLayer->setPosition(winSize / 2);
        contentLayer->setContentSize(winSize);
        this->addChild(contentLayer, 1001);
        auto rachaSprite = CCSprite::create(g_streakData.getRachaSprite().c_str());


        if (rachaSprite) {
            rachaSprite->setPosition(winSize / 2); 
            rachaSprite->setScale(0.0f);
            contentLayer->addChild(rachaSprite, 2);
            rachaSprite->runAction(CCSequence::create(CCDelayTime::create(0.3f),
            CCEaseElasticOut::create(CCScaleTo::create(1.2f, 1.0f), 0.6f), nullptr));
            rachaSprite->runAction(CCSequence::create(CCDelayTime::create(1.5f),
                CCRepeatForever::create(CCSequence::create(CCMoveBy::create(1.5f,
                    { 0, 15.f }),
                    CCMoveBy::create(1.5f, { 0, -15.f }),
                    nullptr
                )),
                    nullptr
                ));
        }



        auto daysLabel = CCLabelBMFont::create(
            fmt::format("Day {}!",
                streakLevel).c_str(),
            "goldFont.fnt"
        );

        daysLabel->setPosition({
            winSize.width / 2,
            winSize.height / 2 - 100.f 
            });

        daysLabel->setScale(0.0f);
        contentLayer->addChild(daysLabel, 2);
        daysLabel->runAction(CCSequence::create(
            CCDelayTime::create(0.8f),
            CCEaseBackOut::create(
                CCScaleTo::create(
                    0.5f, 1.0f
                )
            ), 
            nullptr
        ));
        FMODAudioEngine::sharedEngine()->playEffect("achievement.mp3"_spr);


         contentLayer->runAction(CCSequence::create(CCDelayTime::create(0.5f),
            CCCallFunc::create(
                this,
                callfunc_selector(
                    InfoPopup::playExtraStreakSound
                )
            ),
             nullptr
         ));

         contentLayer->runAction(
             CCSequence::create(CCDelayTime::create(6.0f),
            CCCallFunc::create(
                this,
                callfunc_selector(
                    InfoPopup::onAnimationExit
                    )
                  ),
              nullptr)
              );
    }

    void playExtraStreakSound() {
        FMODAudioEngine::sharedEngine()->playEffect("mcsfx.mp3"_spr);
    }

    void onAnimationExit() {
        if (auto bgLayer = this->getChildByTag(110)) { 
            bgLayer->runAction(CCSequence::create(CCFadeOut::create(0.5f),
                CCRemoveSelf::create(), nullptr));
        }
        if (auto contentLayer = this->getChildByTag(111)) { 
            contentLayer->runAction(CCSequence::create(CCSpawn::create(CCFadeOut::create(0.5f),
                CCEaseBackIn::create(CCScaleTo::create(0.5f, 0.0f)), nullptr), 
                CCRemoveSelf::create(),
                nullptr));
        }
    }
    void onClose(CCObject* sender) override { 
        Popup<>::onClose(sender); 
    }

public:
    static InfoPopup* create() {
        auto ret = new InfoPopup();
        if (ret && ret->initAnchored(260.f, 220.f)) { ret->autorelease();
        return ret;
        }
        CC_SAFE_DELETE(ret); 
        return nullptr;
    }
};
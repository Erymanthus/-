#pragma once
#include "StreakCommon.h"
#include "../StreakData.h"
#include <Geode/ui/Popup.hpp>


class DonationPopup : public Popup<> {
protected:
    void onConfirmKofi(bool btn2) {
        if (btn2) cocos2d::CCApplication::sharedApplication()->openURL("https://ko-fi.com/streakservers");
    }

    void onGoToKofi(CCObject*) {
        createQuickPopup(
            "Wait!",
            "Please read the steps (<cb>click the info icon</c>) <cr>before</c> donating.", 
            "Cancel",
            "Continue to Ko-fi",
            [this](FLAlertLayer*,
                bool btn2) { 
                    this->onConfirmKofi(btn2);
            }
        );
    }
    void onOpenDiscordInfo(CCObject*) {
        createQuickPopup(
            "How to Claim Rewards",
            "Join Discord and <cy>open a ticket</c>.\n\n<cy>Discord Link:</c>\n<cg>https://discord.gg/vEPWBuFEn5</c>",
            "OK",
            "Join Discord",
            [](FLAlertLayer*,
                bool btn2) { 
                    if (btn2) cocos2d::CCApplication::sharedApplication()->openURL("https://discord.gg/vEPWBuFEn5");
            }
        );
    }
    bool setup() override {
        this->setTitle("Support the Mod");
        auto winSize = this->m_mainLayer->getContentSize();
        auto banner = CCSprite::create("support_banner.png"_spr);
        if (banner) {
            banner->setPosition({
                winSize.width / 2, winSize.height / 2 + 15.f
                }
            );
            banner->setScale(0.7f); m_mainLayer->addChild(banner); 
        }

        auto kofiBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create(
                "Go to Ko-fi",
                0,
                0,
                "goldFont.fnt", 
                "GJ_button_01.png",
                0,
                0.8f
                ),
            this, 
            menu_selector(DonationPopup::onGoToKofi)
        );
        auto menu = CCMenu::create(); 
        menu->setPosition(0, 0);
        kofiBtn->setPosition({
            winSize.width / 2, 40.f 
            }
        );
        menu->addChild(kofiBtn);
        auto infoBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName(
                "GJ_infoIcon_001.png"
            ),
            this,
            menu_selector(
                DonationPopup::onOpenDiscordInfo
            )
        );

        infoBtn->setPosition({ 35.f, 35.f });
        menu->addChild(infoBtn);
        m_mainLayer->addChild(menu);
        return true;
    }
public:
    static DonationPopup* create() {
        auto ret = new DonationPopup();
        if (ret && ret->initAnchored(280.f, 200.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret); 
        return nullptr;
    }
};


class AllRachasPopup : public Popup<> {
protected:
    bool setup() override {
        this->setTitle("All Streaks");
        auto winSize = m_mainLayer->getContentSize();
        float startX = 50.f;
        float y = winSize.height / 2 + 20.f;
        float spacing = 50.f;
        std::vector<std::tuple<std::string, int, int>> rachas = {

            { "racha1.png"_spr, 1, 2 },  { "racha2.png"_spr, 10, 3 }, { "racha3.png"_spr, 20, 4  },
            { "racha4.png"_spr, 30, 5 }, { "racha5.png"_spr, 40, 6 }, { "racha6.png"_spr, 50, 7  },
            { "racha7.png"_spr, 60, 8 }, { "racha8.png"_spr, 70, 9 }, { "racha9.png"_spr, 80, 10 }

        };
        int i = 0;
        for (auto& [sprite, day, requiredPoints] : rachas) {
            auto spr = CCSprite::create(sprite.c_str());
            spr->setScale(0.22f);
            spr->setPosition({ startX + i * spacing, y });
            m_mainLayer->addChild(spr);
            auto label = CCLabelBMFont::create(
                CCString::createWithFormat(
                    "Day %d",
                     day
                     )->getCString(),
                    "goldFont.fnt");
            label->setScale(0.35f);
            label->setPosition({
                startX + i * spacing, y - 40
                }
            );

            m_mainLayer->addChild(label);
            auto pointIcon = CCSprite::create("streak_point.png"_spr);
            pointIcon->setScale(0.12f);
            pointIcon->setPosition({ 
                startX + i * spacing - 4, y - 60
                }
            );

            m_mainLayer->addChild(pointIcon);
            auto pointsLabel = CCLabelBMFont::create(std::to_string(requiredPoints).c_str(), "bigFont.fnt");
            pointsLabel->setScale(0.3f);
            pointsLabel->setPosition({
                startX + i * spacing + 6, y - 60 
                }
            );

            m_mainLayer->addChild(pointsLabel);
            i++;
        }
        return true;
    }
public:
    static AllRachasPopup* create() {
        auto ret = new AllRachasPopup();
        if (ret && ret->initAnchored(500.f, 155.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret); 
        return nullptr;
    }
};


class DayProgressPopup : public Popup<> {
protected:
    std::vector<StreakData::BadgeInfo> m_streakBadges;
    int m_currentGoalIndex = 0;
    CCLabelBMFont* m_titleLabel = nullptr;
    CCLabelBMFont* m_dayText = nullptr;
    CCLayerColor* m_barBg = nullptr; 
    CCLayerGradient* m_barFg = nullptr;
    CCLayerColor* m_border = nullptr;
    CCLayerColor* m_outer = nullptr;
    CCSprite* m_rewardSprite = nullptr;

    bool setup() override {
        auto winSize = m_mainLayer->getContentSize();
        g_streakData.load();
        for (const auto& badge : g_streakData.badges) { 
            if (!badge.isFromRoulette) m_streakBadges.push_back(badge);
        }

        auto leftArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png"); 
        leftArrow->setScale(0.8f);
        auto leftBtn = CCMenuItemSpriteExtra::create(
            leftArrow,
            this,
            menu_selector(DayProgressPopup::onPreviousGoal)
        ); 

        leftBtn->setPosition({ -winSize.width / 2 + 30, 0 });

        auto rightArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png"); 
        rightArrow->setScale(0.8f);
        rightArrow->setFlipX(true);
        auto rightBtn = CCMenuItemSpriteExtra::create(
            rightArrow,
            this,
            menu_selector(DayProgressPopup::onNextGoal)
        );
        rightBtn->setPosition({
            winSize.width / 2 - 30, 0 
            }
        );
        auto arrowMenu = CCMenu::create(); 
        arrowMenu->addChild(leftBtn); 
        arrowMenu->addChild(rightBtn);
        arrowMenu->setPosition({
            winSize.width / 2, winSize.height / 2 
            });
        m_mainLayer->addChild(arrowMenu);

        m_barBg = CCLayerColor::create(
            ccc4(45, 45, 45, 255), 180.0f, 16.0f);
        m_barFg = CCLayerGradient::create(
            ccc4(250, 225, 60, 255), ccc4(255, 165, 0, 255)
        );

        m_border = CCLayerColor::create(
            ccc4(255, 255, 255, 255), 182.0f, 18.0f
        );
        m_outer = CCLayerColor::create(
            ccc4(0, 0, 0, 255), 186.0f, 22.0f
        );

        m_barBg->setVisible(false); 
        m_barFg->setVisible(false);
        m_border->setVisible(false); 
        m_outer->setVisible(false);
        m_mainLayer->addChild(m_barBg, 1);
        m_mainLayer->addChild(m_barFg, 2);
        m_mainLayer->addChild(m_border, 4);
        m_mainLayer->addChild(m_outer, 0);

        m_titleLabel = CCLabelBMFont::create(
            "", "goldFont.fnt"
        );
        m_titleLabel->setScale(0.6f);
        m_titleLabel->setPosition({
            winSize.width / 2, winSize.height / 2 + 60
            });
        m_mainLayer->addChild(m_titleLabel, 5);
        m_dayText = CCLabelBMFont::create("", "goldFont.fnt");
        m_dayText->setScale(0.5f); 
        m_dayText->setPosition({
            winSize.width / 2, winSize.height / 2 - 35
            });
        m_mainLayer->addChild(m_dayText, 5);

        updateDisplay();
        return true;
    }

    void updateDisplay() {
        if (m_streakBadges.empty()) return;
        auto winSize = m_mainLayer->getContentSize();
        g_streakData.load();
        if (m_currentGoalIndex >= m_streakBadges.size()) m_currentGoalIndex = 0;
        auto& badge = m_streakBadges[m_currentGoalIndex];

        int currentDays = g_streakData.isBadgeUnlocked(badge.badgeID) ? badge.daysRequired : std::min(
            g_streakData.currentStreak,
            badge.daysRequired
        );
        float percent = badge.daysRequired > 0 ? currentDays / static_cast<float>(badge.daysRequired) : 0.f;

        m_titleLabel->setString(CCString::createWithFormat("Progress to %d Days", badge.daysRequired)->getCString());
        float barWidth = 180.0f; 
        float barHeight = 16.0f;
        m_barBg->setContentSize({ barWidth, barHeight });
        m_barBg->setPosition({
            winSize.width / 2 - barWidth / 2, winSize.height / 2 - 10 
            }
        );

        m_barBg->setVisible(true);
        m_barFg->setContentSize({ barWidth * percent, barHeight }); 
        m_barFg->setPosition({
            winSize.width / 2 - barWidth / 2, winSize.height / 2 - 10
            }
        );

        m_barFg->setVisible(true);
        m_border->setContentSize({ barWidth + 2, barHeight + 2 }); 
        m_border->setPosition({
            winSize.width / 2 - barWidth / 2 - 1, winSize.height / 2 - 11
            }
        ); 

        m_border->setVisible(true);
        m_border->setOpacity(120);
        m_outer->setContentSize({
            barWidth + 6, barHeight + 6
            }
        );

        m_outer->setPosition({ 
            winSize.width / 2 - barWidth / 2 - 3, winSize.height / 2 - 13
            }
        ); 

        m_outer->setVisible(true);
        m_outer->setOpacity(70);

        if (m_rewardSprite) {
            m_rewardSprite->removeFromParent(); 
            m_rewardSprite = nullptr; 
        }
        m_rewardSprite = CCSprite::create(badge.spriteName.c_str());
        if (m_rewardSprite) {
            m_rewardSprite->setScale(0.25f);
        m_rewardSprite->setPosition({ 
            winSize.width / 2, winSize.height / 2 + 30
            }
        ); 
        m_mainLayer->addChild(m_rewardSprite, 5);
        }
        m_dayText->setString(
            CCString::createWithFormat(
               "Day %d / %d", 
                currentDays, 
                badge.daysRequired)->getCString()
        );
    }

    void onNextGoal(CCObject*) {
        if (m_streakBadges.empty()) return;
        m_currentGoalIndex = (m_currentGoalIndex + 1) % m_streakBadges.size();
        updateDisplay();
    }


    void onPreviousGoal(CCObject*) {
        if (m_streakBadges.empty()) return;
        m_currentGoalIndex = (m_currentGoalIndex - 1 + m_streakBadges.size()) % m_streakBadges.size();
        updateDisplay(); 
    }


public:
    static DayProgressPopup* create() {
        auto ret = new DayProgressPopup();
        if (ret && ret->initAnchored(300.f, 180.f)) { ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};

class StreakProgressBar : public cocos2d::CCLayerColor {
protected:
    int m_pointsGained;
    int m_pointsBefore;
    int m_pointsRequired;
    cocos2d::CCLabelBMFont* m_pointLabel;
    cocos2d::CCNode* m_barContainer; 
    cocos2d::CCLayer* m_barFg;
    float m_barWidth; 
    float m_barHeight;
    float m_currentPercent; 
    float m_targetPercent;
    float m_currentPointsDisplay; 
    float m_targetPointsDisplay;



    bool init(int pointsGained, int pointsBefore, int pointsRequired) {
        if (!CCLayerColor::initWithColor({ 0, 0, 0, 0 })) return false;
        m_pointsGained = pointsGained;
        m_pointsBefore = pointsBefore;
        m_pointsRequired = pointsRequired;
        m_currentPercent = std::min(
            1.f, 
            static_cast<float>(m_pointsBefore) / m_pointsRequired
        );

        m_targetPercent = m_currentPercent;
        m_currentPointsDisplay = static_cast<float>(m_pointsBefore);
        m_targetPointsDisplay = m_currentPointsDisplay;
        auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
        m_barWidth = 160.0f; 
        m_barHeight = 15.0f;
        m_barContainer = CCNode::create();
        m_barContainer->setPosition(
            30,
            winSize.height - 280
        ); 

        this->addChild(m_barContainer);
        auto streakIcon = CCSprite::create(g_streakData.getRachaSprite().c_str());
        streakIcon->setScale(0.2f);
        streakIcon->setRotation(-15.f);
        streakIcon->setPosition({ 
            -5, (m_barHeight + 6) / 2
            }
        );

        m_barContainer->addChild(streakIcon, 10);
        auto barBg = cocos2d::extension::CCScale9Sprite::create("GJ_button_01.png");
        barBg->setContentSize({ m_barWidth + 6, m_barHeight + 6 });
        barBg->setColor({ 0, 0, 0 });
        barBg->setOpacity(120);
        barBg->setAnchorPoint({ 0, 0 });
        barBg->setPosition({ 0, 0 });
        m_barContainer->addChild(barBg);
        auto stencil = cocos2d::extension::CCScale9Sprite::create("GJ_button_01.png"); 
        stencil->setContentSize({ m_barWidth, m_barHeight }); 
        stencil->setAnchorPoint({ 0, 0 });
        stencil->setPosition({ 3, 3 });
        auto clipper = CCClippingNode::create();
        clipper->setStencil(stencil);
        barBg->addChild(clipper);
        m_barFg = CCLayerGradient::create(
            { 255, 225, 60, 255 },
            { 255, 165, 0, 255 }
        );

        m_barFg->setContentSize({ 
            m_barWidth * m_currentPercent, m_barHeight 
            }
        ); 

        m_barFg->setAnchorPoint({ 0, 0 }); 
        m_barFg->setPosition({ 0, 0 }); 
        clipper->addChild(m_barFg);
        m_pointLabel = CCLabelBMFont::create(
            CCString::createWithFormat(
                "%d/%d",
                m_pointsBefore, 
                m_pointsRequired)->getCString(),
               "bigFont.fnt"
        );

        m_pointLabel->setAnchorPoint({ 1, 0.5f });
        m_pointLabel->setScale(0.4f);
        m_pointLabel->setPosition({
            m_barFg->getContentSize().width - 5, m_barHeight / 2
            }
        );

        m_barFg->addChild(m_pointLabel, 5);
        this->runAnimations();
        this->scheduleUpdate();
        return true;
    }

    void update(float dt) override {
        float smoothingFactor = 8.0f;
        m_currentPercent = m_currentPercent + (m_targetPercent - m_currentPercent) * dt * smoothingFactor;
        m_currentPointsDisplay = m_currentPointsDisplay + (m_targetPointsDisplay - m_currentPointsDisplay) * dt * smoothingFactor;
        m_barFg->setContentSize({ 
            m_barWidth * m_currentPercent, m_barHeight 
            }
        );

        m_pointLabel->setString(
            CCString::createWithFormat(
                "%d/%d",
                static_cast<int>(
                round
               (m_currentPointsDisplay)),
                m_pointsRequired)->getCString()
        );

        m_pointLabel->setPosition({ m_barFg->getContentSize().width - 5, m_barHeight / 2 });
    }

    void runAnimations() {
        CCPoint onScreenPos = m_barContainer->getPosition();
        CCPoint offScreenPos = m_barContainer->getPosition() + CCPoint(-250, 0);
        m_barContainer->setPosition(offScreenPos);
        m_barContainer->runAction(CCSequence::create(CCEaseSineOut::create(
            CCMoveTo::create(0.4f, onScreenPos)),
            CCCallFunc::create(
                this,
                callfunc_selector(StreakProgressBar::spawnPointParticles)), 
                CCDelayTime::create(2.5f + m_pointsGained * 0.15f),
                CCEaseSineIn::create(CCMoveTo::create(0.4f, offScreenPos)), 
                CCCallFunc::create(this, callfunc_selector(StreakProgressBar::stopUpdateLoop)),
                CCRemoveSelf::create(), nullptr));
    }
    void stopUpdateLoop() {
        this->unscheduleUpdate();
    }

    void spawnPointParticles() {
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        CCPoint center = winSize / 2; 
        float delayPerPoint = 0.1f;
        for (int i = 0; i < m_pointsGained; ++i) {
            auto pointParticle = CCSprite::create("streak_point.png"_spr); 
            pointParticle->setScale(0.25f);
            pointParticle->setPosition(center);
            this->addChild(pointParticle, 10);

            CCPoint endPos = m_barContainer->getPosition() + CCPoint(3 + m_barWidth * (
                std::min(
                    1.f,
                    (float)(m_pointsBefore + i + 1) / m_pointsRequired)), 
                    3 + m_barHeight / 2);

            ccBezierConfig bezier; 
            bezier.endPosition = endPos; 
            float explosionRadius = 150.f; 
            float randomAngle = (float)(rand() % 360);
            bezier.controlPoint_1 = center + CCPoint((explosionRadius + (rand() % 50)) * cos(CC_DEGREES_TO_RADIANS(randomAngle)),
            (explosionRadius + (rand() % 50)) * sin(CC_DEGREES_TO_RADIANS(randomAngle)));
            bezier.controlPoint_2 = endPos + CCPoint(0, 100);


            auto bezierTo = CCBezierTo::create(1.0f, bezier); 
            auto rotateAction = CCRotateBy::create(1.0f, 360 + (rand() % 180));
            auto scaleAction = CCScaleTo::create(1.0f, 0.1f); 
            auto pointIndexObj = CCInteger::create(i + 1);
            pointParticle->runAction(CCSequence::create(
                CCDelayTime::create(i * delayPerPoint),
                CCSpawn::create(bezierTo, rotateAction, scaleAction, nullptr),
                CCCallFuncO::create(
                    this,
                    callfuncO_selector(StreakProgressBar::onPointHitBar),
                pointIndexObj
              ),
                CCRemoveSelf::create(),
                nullptr
            )

            );
        }
    }
    void onPointHitBar(CCObject* sender) {
        if (!sender) return;

        int pointsToAdd = 0;
        if (auto node = dynamic_cast<CCNode*>(sender)) {
            auto userObj = node->getUserObject();

            if (userObj) {
                if (auto strVal = dynamic_cast<CCString*>(userObj)) {
                    pointsToAdd = strVal->intValue();
                }
               
                else if (auto intVal = dynamic_cast<CCInteger*>(userObj)) {
                    pointsToAdd = intVal->getValue();
                }
            }
        }
      
        else if (auto intVal = dynamic_cast<CCInteger*>(sender)) {
            pointsToAdd = intVal->getValue();
        }
  
        if (pointsToAdd == 0) return;

        int currentTotalPoints = m_pointsBefore + pointsToAdd;

        m_targetPercent = std::min(1.f, static_cast<float>(currentTotalPoints) / m_pointsRequired);
        m_targetPointsDisplay = static_cast<float>(currentTotalPoints);      
        auto popUp = CCEaseSineOut::create(
            CCScaleTo::create(0.1f, 1.0f, 1.2f)
        );

        auto popDown = CCEaseSineIn::create(
            CCScaleTo::create(0.1f, 1.0f, 1.0f)
        );

        if (m_barFg) m_barFg->runAction(
            CCSequence::create(popUp, popDown, nullptr)
        );
        FMODAudioEngine::sharedEngine()->playEffect("coin.mp3"_spr);
    }

public:
    static StreakProgressBar* create(int pointsGained, int pointsBefore, int pointsRequired) {
        auto ret = new StreakProgressBar();
        if (ret && ret->init(pointsGained, pointsBefore, pointsRequired)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret); 
        return nullptr;
    }
};
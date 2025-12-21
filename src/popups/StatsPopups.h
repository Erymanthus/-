#pragma once
#include "StreakCommon.h"
#include "../StreakData.h"
#include <Geode/ui/Popup.hpp>
#include <Geode/binding/SimplePlayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/ui/ScrollLayer.hpp>


using namespace geode::prelude;

class DonationPopup : public Popup<> {
protected:
  
    void onConfirmKofi(bool btn2) {
        if (btn2) cocos2d::CCApplication::sharedApplication()->openURL("https://ko-fi.com/streakservers");
    }

    void onGoToKofi(CCObject*) {
        createQuickPopup("Wait!",
            "Please read the steps (<cb>click the info icon</c>) <cr>before</c> donating.",
            "Cancel",
            "Continue",
            [this](FLAlertLayer*, bool btn2) { 
                this->onConfirmKofi(btn2); 
            });
    }

    void onOpenDiscordInfo(CCObject*) {
        createQuickPopup("How to Claim",
            "Join Discord and <cy>open a ticket</c>.\n\n<cy>Discord Link:</c>\n<cg>https://discord.gg/vEPWBuFEn5</c>",
            "OK",
            "Join Discord",
            [](FLAlertLayer*, bool btn2) {
                if (btn2) cocos2d::CCApplication::sharedApplication()->openURL("https://discord.gg/vEPWBuFEn5");
            });
    }

     

    CCAction* createRainbowAction(float speed) {
        auto sequence = CCSequence::create(
            CCTintTo::create(speed, 255, 0, 0),    
            CCTintTo::create(speed, 255, 255, 0),  
            CCTintTo::create(speed, 0, 255, 0),    
            CCTintTo::create(speed, 0, 255, 255),  
            CCTintTo::create(speed, 0, 0, 255),    
            CCTintTo::create(speed, 255, 0, 255), 
            nullptr
        );
        return CCRepeatForever::create(sequence);
    }

    
    CCAction* createPulseAction(float duration, float scaleBase) {
        float scaleMin = scaleBase * 0.9f;
        float scaleMax = scaleBase * 1.1f;

        auto scaleUp = CCScaleTo::create(duration, scaleMax);
        auto scaleDown = CCScaleTo::create(duration, scaleMin);
        auto sequence = CCSequence::create(scaleUp, scaleDown, nullptr);

        return CCRepeatForever::create(sequence);
    }

    CCSprite* loadSpriteSafe(const char* name, const char* fallback) {
        auto sprite = CCSprite::create(name);
        if (!sprite) sprite = CCSprite::createWithSpriteFrameName(fallback);
        if (!sprite) sprite = CCSprite::create(fallback);
        if (!sprite) sprite = CCSprite::create("GJ_button_01.png");
        return sprite;
    }

   
    void runWaveAnimation(CCNode* labelNode) {
        auto label = static_cast<CCLabelBMFont*>(labelNode);
        if (!label) return;

        auto children = label->getChildren();
        if (!children) return;

        float delayPerChar = 0.05f;
        float jumpHeight = 10.0f;
        float jumpDuration = 0.2f;

        for (int i = 0; i < children->count(); ++i) {
            auto charNode = static_cast<CCNode*>(children->objectAtIndex(i));
            if (charNode) {
                charNode->stopAllActions();
                auto sequence = CCSequence::create(
                    CCDelayTime::create(i * delayPerChar),
                    CCMoveBy::create(jumpDuration, { 0, jumpHeight }),
                    CCMoveBy::create(jumpDuration, { 0, -jumpHeight }),
                    nullptr
                );
                charNode->runAction(sequence);
            }
        }
    }

   
    bool setup() override {
        auto winSize = m_mainLayer->getContentSize();
        this->setTitle("Support the Mod");
        auto gm = GameManager::sharedState();

         
        auto floor = CCSprite::createWithSpriteFrameName("floorLine_01_001.png");
        floor->setPosition({ winSize.width / 2, winSize.height / 2 + 25.f });
        floor->setScaleX(0.8f);
        floor->setOpacity(200);
        m_mainLayer->addChild(floor);

        
        auto player = SimplePlayer::create(gm->getPlayerFrame());
        player->updatePlayerFrame(gm->getPlayerFrame(), IconType::Cube);
        player->setColor(gm->colorForIdx(gm->getPlayerColor()));
        player->setSecondColor(gm->colorForIdx(gm->getPlayerColor2()));
        if (gm->getPlayerGlow()) player->setGlowOutline(gm->colorForIdx(gm->getPlayerGlowColor()));

        float groundY = floor->getPositionY() + 18.f;
        float startY = winSize.height + 50.f;

        player->setPosition({ winSize.width / 2, startY });
        player->setScale(1.1f);
        m_mainLayer->addChild(player, 5);

        auto fallAction = CCMoveTo::create(1.0f, { winSize.width / 2, groundY });
        auto bounceEffect = CCEaseBounceOut::create(fallAction);
        player->runAction(CCSequence::create(CCDelayTime::create(0.2f), bounceEffect, nullptr));
 
        auto nameContainer = CCNode::create();
        nameContainer->setPosition({ winSize.width / 2, groundY + 40.f });
        m_mainLayer->addChild(nameContainer, 10);

        auto nameLabel = CCLabelBMFont::create(gm->m_playerName.c_str(), "bigFont.fnt");
        nameLabel->setScale(0.6f);

        
        nameLabel->runAction(createRainbowAction(0.5f));

       
        auto waveSequence = CCSequence::create(
            CCCallFuncN::create(this, callfuncN_selector(DonationPopup::runWaveAnimation)),
            CCDelayTime::create(3.0f),
            nullptr
        );
        nameLabel->runAction(CCRepeatForever::create(waveSequence));

        nameContainer->addChild(nameLabel);

        
        float row1_Y = floor->getPositionY() - 35.f;
        float row2_Y = floor->getPositionY() - 85.f;

       
        auto bannerSprite = loadSpriteSafe("banner19.png"_spr, "GJ_button_02.png");
        float bannerScale = 0.1f;
        if (bannerSprite->getContentSize().width > 200.f) bannerScale = 200.f / bannerSprite->getContentSize().width;
        bannerSprite->setScale(bannerScale);
        bannerSprite->setPosition({ winSize.width / 2, row1_Y });
        m_mainLayer->addChild(bannerSprite);

        
        auto badgeLeft = loadSpriteSafe("magic_flower_badge.png"_spr, "starSmall_001.png");
        badgeLeft->setScale(0.23f);
        badgeLeft->setPosition({ winSize.width / 2 - 90.f, row1_Y });
        m_mainLayer->addChild(badgeLeft);

        
        auto badgeRight = loadSpriteSafe("moderator_badge.png"_spr, "starSmall_001.png");
        badgeRight->setScale(0.2f);
        badgeRight->setPosition({ winSize.width / 2 + 90.f, row1_Y });
        m_mainLayer->addChild(badgeRight);

        
        float textY = row1_Y - 22.f;

        
        auto mythicLabel = CCLabelBMFont::create("Mythic Badge & Mythic Banner", "goldFont.fnt");
        mythicLabel->setPosition({ winSize.width / 2 - 45.f, textY });
        mythicLabel->setScale(0.35f);
        mythicLabel->runAction(createRainbowAction(0.5f));
        mythicLabel->runAction(createPulseAction(0.8f, 0.35f));
        m_mainLayer->addChild(mythicLabel);

       
        auto moderatorLabel = CCLabelBMFont::create("Moderator", "goldFont.fnt");
        moderatorLabel->setPosition({ winSize.width / 2 + 90.f, textY });
        moderatorLabel->setScale(0.4f);
        moderatorLabel->runAction(createRainbowAction(0.5f));
        moderatorLabel->runAction(createPulseAction(0.8f, 0.4f));
        m_mainLayer->addChild(moderatorLabel);

       

        auto starIcon = loadSpriteSafe("super_star.png"_spr, "GJ_starsIcon_001.png");
        starIcon->setPosition({ winSize.width / 2 - 60.f, row2_Y });
        starIcon->setScale(0.2f);
        m_mainLayer->addChild(starIcon);

        auto starText = CCLabelBMFont::create("+150", "goldFont.fnt");
        starText->setColor({ 0, 255, 0 });
        starText->setScale(0.5f);
        starText->setAnchorPoint({ 0.f, 0.5f });
        starText->setPosition({ starIcon->getPositionX() + 15.f, row2_Y });
        m_mainLayer->addChild(starText);

        auto ticketIcon = loadSpriteSafe("star_tiket.png"_spr, "currencyOrbIcon_001.png");
        ticketIcon->setPosition({ winSize.width / 2 + 20.f, row2_Y });
        ticketIcon->setScale(0.2f);
        m_mainLayer->addChild(ticketIcon);

        auto ticketText = CCLabelBMFont::create("+20.000", "goldFont.fnt");
        ticketText->setColor({ 0, 255, 0 });
        ticketText->setScale(0.5f);
        ticketText->setAnchorPoint({ 0.f, 0.5f });
        ticketText->setPosition({ ticketIcon->getPositionX() + 15.f, row2_Y });
        m_mainLayer->addChild(ticketText);

        
        auto menu = CCMenu::create();
        menu->setPosition(0, 0);

        auto kofiBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Go to Ko-fi", 0, 0, "goldFont.fnt", "GJ_button_01.png", 0, 0.8f),
            this, menu_selector(DonationPopup::onGoToKofi));
        kofiBtn->setPosition({ winSize.width / 2, 30.f });
        menu->addChild(kofiBtn);

        auto infoBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"),
            this, menu_selector(DonationPopup::onOpenDiscordInfo));
        infoBtn->setPosition({ winSize.width - 25.f, winSize.height - 25.f });
        menu->addChild(infoBtn);

        m_mainLayer->addChild(menu);
        return true;
    }

public:
    static DonationPopup* create() {
        auto ret = new DonationPopup();
        if (ret && ret->initAnchored(360.f, 290.f, "geode.loader/GE_square03.png")) {
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

        
        std::vector<std::tuple<std::string, int, int, int>> rachas = {
            { "racha1.png"_spr, 1, 2, 25 },
            { "racha2.png"_spr, 10, 3, 40 }, 
            { "racha3.png"_spr, 20, 4, 55 },
            { "racha4.png"_spr, 30, 5, 70 }, 
            { "racha5.png"_spr, 40, 6, 85}, 
            { "racha6.png"_spr, 50, 7, 100 },
            { "racha7.png"_spr, 60, 8, 115 },
            { "racha8.png"_spr, 70, 9, 130 }, 
            { "racha9.png"_spr, 80, 10, 145 },
            { "racha10.png"_spr, 90, 11, 160 },
            { "racha11.png"_spr, 100, 12, 175 }
        };

        
        auto limitNodeSize = [](CCNode* node, float maxSize) {
            if (!node) return;
            CCSize size = node->getContentSize();
           
            if (size.width <= 0 || size.height <= 0) return;

           
            float maxSide = std::max(size.width, size.height);

          
            if (maxSide > maxSize) {
                float scale = maxSize / maxSide;
                node->setScale(scale);
            }
            else {
               
                node->setScale(1.0f);
            }
            };

        CCSize scrollSize = { winSize.width - 40.f, 150.f };

        auto scrollLayer = ScrollLayer::create(scrollSize);
        scrollLayer->setPosition((winSize.width - scrollSize.width) / 2, 35.f);

        auto bg = CCScale9Sprite::create("square02_001.png");
        bg->setContentSize(scrollSize);
        bg->setOpacity(50);
        bg->setPosition(winSize.width / 2, scrollLayer->getPositionY() + scrollSize.height / 2);
        m_mainLayer->addChild(bg);

        auto content = scrollLayer->m_contentLayer;
        content->setLayout(
            ColumnLayout::create()
            ->setAxisReverse(true)
            ->setGap(5.f)
            ->setAxisAlignment(AxisAlignment::End)
            ->setAutoGrowAxis(scrollSize.height)
        );

        for (const auto& [spriteName, day, firePoints, xpAmount] : rachas) {

            auto cell = CCNode::create();
            cell->setContentSize({ scrollSize.width, 40.f });

            auto cellBg = CCScale9Sprite::create("geode.loader/GE_square03.png");
            cellBg->setContentSize({ scrollSize.width, 40.f });
            cellBg->setOpacity(75);
            cellBg->setPosition(cell->getContentSize() / 2);
            cell->addChild(cellBg);

            
            auto spr = CCSprite::create(spriteName.c_str());
            if (spr) {
                spr->setScale(0.25f);
                spr->setPosition({ 30.f, 20.f });
                cell->addChild(spr);
            }

         
            auto label = CCLabelBMFont::create(
                CCString::createWithFormat("Day %d", day)->getCString(),
                "goldFont.fnt"
            );
            label->setScale(0.5f);
            label->setAnchorPoint({ 0.f, 0.5f });
            label->setPosition({ 60.f, 20.f });
            cell->addChild(label);

         
            auto pointsNode = CCNode::create();
            pointsNode->setContentSize({ 130.f, 40.f });
            pointsNode->setAnchorPoint({ 1.f, 0.5f });
            pointsNode->setPosition({ scrollSize.width - 10.f, 20.f });

            auto layout = RowLayout::create();
            layout->setGap(10.f);
            layout->setAxisAlignment(AxisAlignment::End);
         
            layout->setAutoScale(false);
            pointsNode->setLayout(layout);

          
            auto xpIcon = CCSprite::create("xp.png"_spr);
            if (xpIcon) {
               
                limitNodeSize(xpIcon, 18.0f);
                pointsNode->addChild(xpIcon);
            }

            auto xpLabel = CCLabelBMFont::create(std::to_string(xpAmount).c_str(), "bigFont.fnt");
            xpLabel->setScale(0.35f);
            pointsNode->addChild(xpLabel);

           
            auto pointIcon = CCSprite::create("streak_point.png"_spr);
            if (pointIcon) {
              
                limitNodeSize(pointIcon, 18.0f);
                pointsNode->addChild(pointIcon);
            }

            auto pointsLabel = CCLabelBMFont::create(std::to_string(firePoints).c_str(), "bigFont.fnt");
            pointsLabel->setScale(0.35f);
            pointsNode->addChild(pointsLabel);

            pointsNode->updateLayout();
            cell->addChild(pointsNode);

            content->addChild(cell);
        }

        content->updateLayout();
        m_mainLayer->addChild(scrollLayer);

        return true;
    }

public:
    static AllRachasPopup* create() {
        auto ret = new AllRachasPopup();
        if (ret && ret->initAnchored(270.f, 230.f, "geode.loader/GE_square03.png")) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};



#ifndef GOAL_ITEM_DEFINED
#define GOAL_ITEM_DEFINED
struct GoalItem {
    std::string id;
    std::string type; 
    int daysRequired;
    std::string spriteName;
    bool isUnlocked;
};
#endif


class GoalCell : public CCNode {
public:
    static GoalCell* create(const GoalItem& item, float width) {
        auto ret = new GoalCell();
        if (ret && ret->init(item, width)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init(const GoalItem& item, float width) {
        if (!CCNode::init()) return false;

        float height = 50.0f;
        this->setContentSize(CCSize{ width, height });

      
        auto bg = CCLayerColor::create({ 0, 0, 0, 80 });
        bg->setContentSize(CCSize{ width, height - 4 });
        bg->setPosition(CCPoint{ 0, 2 });
        this->addChild(bg);

       
        float iconCenterX = width - 35.0f;

      
        CCSprite* icon = CCSprite::create(item.spriteName.c_str());
        if (!icon) icon = CCSprite::createWithSpriteFrameName("GJ_questionMark_001.png");

        float maxIconSize = 40.0f;
        float scale = 1.0f;
 
        if (icon->getContentSize().width > icon->getContentSize().height) {
            scale = 55.0f / icon->getContentSize().width;
        }
        else {
            scale = maxIconSize / icon->getContentSize().height;
        }
        icon->setScale(scale);
        icon->setPosition(CCPoint{ iconCenterX, height / 2 + 4.0f });
        this->addChild(icon);

      
        int currentStreak = g_streakData.currentStreak;
        int target = item.daysRequired;
        if (item.isUnlocked) currentStreak = target;

        float percentage = 0.0f;
        if (target > 0) percentage = static_cast<float>(currentStreak) / static_cast<float>(target);
        if (percentage > 1.0f) percentage = 1.0f;

        float barWidth = width - 90.0f;
        float barHeight = 16.0f;
        float barX = 10.0f;
        float barY = 12.0f;
        float borderSize = 2.0f;

     
        auto border = CCLayerColor::create({ 0, 0, 0, 255 }, barWidth + (borderSize * 2), barHeight + (borderSize * 2));
        border->setPosition(CCPoint{ barX - borderSize, barY - borderSize });
        this->addChild(border);

       
        auto barTrack = CCLayerColor::create({ 40, 40, 40, 255 }, barWidth, barHeight);
        barTrack->setPosition(CCPoint{ barX, barY });
        this->addChild(barTrack);

      
        if (percentage > 0) {
            auto barFg = CCLayerGradient::create(
                { 0, 255, 255, 255 },   
                { 0, 100, 200, 255 }  
            );
            barFg->setContentSize(CCSize{ barWidth * percentage, barHeight });
            barFg->setPosition(CCPoint{ barX, barY });
            this->addChild(barFg);
        }

       
        auto title = CCLabelBMFont::create(fmt::format("{} Days", target).c_str(), "goldFont.fnt");
        title->setScale(0.45f);
        title->setAnchorPoint(CCPoint{ 0.0f, 0.5f });
        title->setPosition(CCPoint{ barX, height - 12.0f });
        this->addChild(title);

        auto progressTxt = CCLabelBMFont::create(fmt::format("{}/{}", std::min(currentStreak, target), target).c_str(), "bigFont.fnt");
        progressTxt->setScale(0.5f);
        progressTxt->setPosition(CCPoint{ barX + barWidth / 2, barY + barHeight / 2 + 1 });
        progressTxt->setColor({ 255, 255, 255 });

        auto shadow = CCLabelBMFont::create(progressTxt->getString(), "bigFont.fnt");
        shadow->setScale(0.5f);
        shadow->setColor({ 0,0,0 });
        shadow->setPosition(progressTxt->getPosition() + CCPoint{ -1, -1 });
        shadow->setZOrder(-1);
        this->addChild(shadow);
        this->addChild(progressTxt);

        
        auto typeLbl = CCLabelBMFont::create(item.type.c_str(), "bigFont.fnt");
        typeLbl->setScale(0.25f);
        typeLbl->setAnchorPoint(CCPoint{ 0.5f, 0.5f });
        typeLbl->setPosition(CCPoint{ iconCenterX, 8.0f });
        typeLbl->setOpacity(150);
        this->addChild(typeLbl);

        return true;
    }
};

 
class DayProgressPopup : public Popup<> {
protected:
    bool setup() override {
        this->setTitle("Streak Milestones");
        auto winSize = m_mainLayer->getContentSize();
        g_streakData.load();
 
        std::vector<GoalItem> allGoals;

        for (const auto& badge : g_streakData.badges) {
       
            if (!badge.isFromRoulette) {
                GoalItem item;
                item.id = badge.badgeID;
                item.type = "Badge";
                item.daysRequired = badge.daysRequired;
                item.spriteName = badge.spriteName;
                item.isUnlocked = g_streakData.isBadgeUnlocked(badge.badgeID);
                allGoals.push_back(item);
            }
        }

        
        std::sort(allGoals.begin(), allGoals.end(), [](const GoalItem& a, const GoalItem& b) {
            return a.daysRequired < b.daysRequired;
            });

       
        auto listSize = CCSize{ 340.f, 190.f };
        auto scroll = ScrollLayer::create(listSize);

        float cellHeight = 55.0f;
        float totalHeight = std::max(listSize.height, (float)allGoals.size() * cellHeight);

        scroll->m_contentLayer->setContentSize(CCSize{ listSize.width, totalHeight });

        int i = 0;
        for (const auto& item : allGoals) {
            auto cell = GoalCell::create(item, listSize.width);
            float yPos = totalHeight - (i * cellHeight) - cellHeight;
            cell->setPosition(CCPoint{ 0, yPos });
            scroll->m_contentLayer->addChild(cell);
            i++;
        }

        scroll->m_contentLayer->setPositionY(listSize.height - totalHeight);
        scroll->setPosition(CCPoint{
            (winSize.width - listSize.width) / 2,
            (winSize.height - listSize.height) / 2 - 10.f
            });

        auto listBg = cocos2d::extension::CCScale9Sprite::create("geode.loader/GE_square03.png");
        listBg->setContentSize(listSize + CCSize{ 4.f, 4.f });
        listBg->setColor({ 0,0,0 });
        listBg->setOpacity(100);
        listBg->setPosition(winSize / 2 - CCPoint{ 0.f, 10.f });
        m_mainLayer->addChild(listBg);

        m_mainLayer->addChild(scroll);

        return true;
    }

public:
    static DayProgressPopup* create() {
        auto ret = new DayProgressPopup();
        if (ret && ret->initAnchored(380.f, 260.f, "geode.loader/GE_square03.png")) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
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
#pragma once
#include <cocos2d.h>
#include "StreakData.h" 
#include <algorithm> 
#include <vector>

using namespace cocos2d;
using namespace geode::prelude;

class BadgeNotification : public CCNode {
    static std::vector<BadgeNotification*> s_activeBadges;
    bool m_isExiting = false;

public:
    static void show(const std::string& badgeID) {
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        if (!scene) return;

        auto badgeInfo = g_streakData.getBadgeInfo(badgeID);
        if (!badgeInfo) return;

        auto node = BadgeNotification::create(badgeInfo, badgeInfo->displayName);
        s_activeBadges.insert(s_activeBadges.begin(), node);
        node->setTag(1500);
        scene->addChild(node, 100000);
        BadgeNotification::updatePositions();
    }

    static void updatePositions() {
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        float startY = winSize.height - 30.f;
        float gap = 60.f;

        for (size_t i = 0; i < s_activeBadges.size(); ++i) {
            auto node = s_activeBadges[i];
            if (!node) continue;
            if (node->m_isExiting) continue;
            float targetY = startY - (i * gap);

            if (std::abs(node->getPositionY() - targetY) > 1.0f) {
                node->stopActionByTag(999);
                auto move = CCEaseInOut::create(CCMoveTo::create(0.3f, ccp(node->getPositionX(), targetY)), 2.0f);
                move->setTag(999);
                node->runAction(move);
            }
        }
    }

protected:
    static BadgeNotification* create(const StreakData::BadgeInfo* info, const std::string& nameText) {
        auto ret = new BadgeNotification();
        if (ret && ret->init(info, nameText)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init(const StreakData::BadgeInfo* info, const std::string& nameText) {
        if (!CCNode::init()) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        this->setAnchorPoint({ 0.f, 1.f });
        CCSize size = { 240.f, 50.f };
        this->setContentSize(size);

      
        auto bg = cocos2d::extension::CCScale9Sprite::create("square02_001.png");
        bg->setContentSize(size);
        bg->setPosition({ size.width / 2, size.height / 2 });
        bg->setOpacity(200);
        this->addChild(bg);

        CCSprite* badgeSpr = nullptr;
        badgeSpr = CCSprite::create(info->spriteName.c_str());
        if (!badgeSpr) badgeSpr = CCSprite::createWithSpriteFrameName(info->spriteName.c_str());
        if (!badgeSpr) badgeSpr = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");

        if (badgeSpr) {
            badgeSpr->setPosition({ 30.f, size.height / 2 });
            badgeSpr->setScale(0.4f);
            this->addChild(badgeSpr);

            auto glow = CCSprite::createWithSpriteFrameName("dragEffect_01.png");
            if (glow) {
                glow->setPosition(badgeSpr->getPosition());
                glow->setColor({ 255, 255, 0 });
                glow->setScale(0.4f);
                glow->setBlendFunc({ GL_SRC_ALPHA, GL_ONE });
                glow->runAction(CCRepeatForever::create(CCRotateBy::create(2.0f, 360.f)));
                this->addChild(glow, -1);
            }
        }

      
        auto title = CCLabelBMFont::create("NEW BADGE!", "goldFont.fnt");
        title->setScale(0.45f);
        title->setAnchorPoint({ 0, 0.5f });
        title->setPosition({ 60.f, size.height / 2 + 9.f });
        this->addChild(title);

        auto name = CCLabelBMFont::create(nameText.c_str(), "bigFont.fnt");
        name->setScale(0.35f);
        name->setAnchorPoint({ 0, 0.5f });
        name->setPosition({ 60.f, size.height / 2 - 9.f });
        name->limitLabelWidth(170.f, 0.35f, 0.1f);
        this->addChild(name);

      
        float topY = winSize.height - 30.f;
        float targetX = 10.f;
        this->setPosition({ -size.width, topY });

      
        auto moveIn = CCEaseElasticOut::create(CCMoveTo::create(0.8f, ccp(targetX, topY)), 0.6f);
        this->runAction(moveIn);
        this->scheduleOnce(schedule_selector(BadgeNotification::triggerExit), 3.5f);
        FMODAudioEngine::sharedEngine()->playEffect("achievement_01.ogg");
        return true;
    }

    void triggerExit(float dt) {
        m_isExiting = true;

        auto it = std::find(s_activeBadges.begin(), s_activeBadges.end(), this);
        if (it != s_activeBadges.end()) {
            s_activeBadges.erase(it);
        }

        BadgeNotification::updatePositions();
        float currentY = this->getPositionY();
        auto moveOut = CCEaseBackIn::create(CCMoveTo::create(0.5f, ccp(-this->getContentSize().width, currentY)));
        auto remove = CCCallFunc::create(this, callfunc_selector(BadgeNotification::removeFromParent));

        this->runAction(CCSequence::create(moveOut, remove, nullptr));
    }
};

std::vector<BadgeNotification*> BadgeNotification::s_activeBadges;
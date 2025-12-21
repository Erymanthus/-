#pragma once
#include <cocos2d.h>
#include <Geode/Geode.hpp>
#include <random>
#include <vector>
#include <algorithm>

using namespace cocos2d;
using namespace geode::prelude;

class RewardNotification : public CCNode {
    static std::vector<RewardNotification*> s_activeRewards;

protected:
    int m_addedAmount;
    int m_startAmount;
    int m_sessionAddedAmount; 
    float m_currentAccumulator;

    CCLabelBMFont* m_label;      
    CCLabelBMFont* m_addedLabel;  
    cocos2d::extension::CCScale9Sprite* m_bg;
    std::string m_spriteName;
    CCPoint m_finalBGPosition;
    int m_targetTotal;

public:
    static void show(const std::string& spriteName, int startAmount, int addedAmount) {
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        if (!scene) return;

      
        for (auto node : s_activeRewards) {
            if (node->m_spriteName == spriteName) {
                node->addBonus(addedAmount);
                return;
            }
        }

        auto node = RewardNotification::create(spriteName, startAmount, addedAmount);
        s_activeRewards.push_back(node);
        scene->addChild(node, 1501);
    }

    virtual ~RewardNotification() {
        auto it = std::find(s_activeRewards.begin(), s_activeRewards.end(), this);
        if (it != s_activeRewards.end()) {
            s_activeRewards.erase(it);
        }
    }

    static void updatePositions() {
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        float startY = winSize.height - 30.f;
        float gap = 55.f; 

        for (size_t i = 0; i < s_activeRewards.size(); ++i) {
            auto node = s_activeRewards[i];
            if (!node || !node->m_bg) continue;

            float absTargetY = startY - (i * gap) - (node->m_bg->getContentSize().height / 2);

            if (std::abs(node->m_bg->getPositionY() - absTargetY) > 1.0f) {
                node->m_bg->stopActionByTag(50);
                auto move = CCEaseInOut::create(CCMoveTo::create(0.3f, ccp(node->m_bg->getPositionX(), absTargetY)), 2.0f);
                move->setTag(50);
                node->m_bg->runAction(move);

                node->m_finalBGPosition = ccp(node->m_bg->getPositionX(), absTargetY);
            }
        }
    }

    void addBonus(int extraAmount) {
        m_targetTotal += extraAmount;
        m_sessionAddedAmount += extraAmount;

      
        if (m_addedLabel) {
            m_addedLabel->setString(fmt::format("+{}", m_sessionAddedAmount).c_str());
        }

        this->stopActionByTag(100);
        m_bg->stopActionByTag(200);
        auto resetMove = CCEaseOut::create(CCMoveTo::create(0.2f, m_finalBGPosition), 2.0f);
        m_bg->runAction(resetMove);

        this->runParticleAnimation(extraAmount);
        this->scheduleExit();
    }

protected:
    static RewardNotification* create(const std::string& spriteName, int startAmount, int addedAmount) {
        auto ret = new RewardNotification();
        if (ret && ret->init(spriteName, startAmount, addedAmount)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init(const std::string& spriteName, int startAmount, int addedAmount) {
        if (!CCNode::init()) return false;

     
        FMODAudioEngine::sharedEngine()->playEffect("claim_mission.mp3"_spr);

        m_spriteName = spriteName;
        m_startAmount = startAmount;
        m_addedAmount = addedAmount;
        m_sessionAddedAmount = addedAmount;
        m_targetTotal = startAmount + addedAmount;
        m_currentAccumulator = (float)startAmount;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        m_bg = cocos2d::extension::CCScale9Sprite::create("square02_001.png");
        m_bg->setColor({ 0, 0, 0 });
        m_bg->setOpacity(150);
        this->addChild(m_bg);

        auto icon = CCSprite::create(m_spriteName.c_str());
        if (!icon) icon = CCSprite::createWithSpriteFrameName(m_spriteName.c_str());
        if (icon) {
            icon->setTag(99);
            icon->setScale(0.35f);
            m_bg->addChild(icon);
        }

      
        m_label = CCLabelBMFont::create(std::to_string(m_startAmount).c_str(), "goldFont.fnt");
        m_label->setScale(0.45f);
        m_bg->addChild(m_label);

     
        m_addedLabel = CCLabelBMFont::create(fmt::format("+{}", m_sessionAddedAmount).c_str(), "goldFont.fnt");
        m_addedLabel->setScale(0.35f);
        m_addedLabel->setColor({ 0, 255, 0 }); 
        m_bg->addChild(m_addedLabel);

        this->updateBGSize();

        int myIndex = s_activeRewards.size();
        float gap = 55.f;
        float startY = winSize.height - 30.f;
        float targetY = startY - (myIndex * gap) - (m_bg->getContentSize().height / 2);

        m_finalBGPosition = ccp(winSize.width - (m_bg->getContentSize().width / 2) - 5.f, targetY);

        CCPoint startPos = ccp(winSize.width + m_bg->getContentSize().width, targetY);
        m_bg->setPosition(startPos);

        auto moveIn = CCEaseElasticOut::create(CCMoveTo::create(0.8f, m_finalBGPosition), 0.6f);
        m_bg->runAction(moveIn);

        this->scheduleOnce(schedule_selector(RewardNotification::initialParticles), 0.3f);
        this->scheduleExit();

        return true;
    }

    void updateBGSize() {
        auto icon = m_bg->getChildByTag(99);
        float iconWidth = icon ? icon->getScaledContentSize().width : 20.f;

     
        float labelWidth = m_label->getScaledContentSize().width;
        float addedWidth = m_addedLabel->getScaledContentSize().width;
        float maxTextWidth = std::max(labelWidth, addedWidth);

        float bgWidth = maxTextWidth + iconWidth + 30.f;
        float bgHeight = 45.f;

        m_bg->setContentSize({ bgWidth, bgHeight });

        if (icon) icon->setPosition({ 18.f, bgHeight / 2 });

      
        m_label->setPosition({ bgWidth - (maxTextWidth / 2) - 10.f, bgHeight / 2 + 7.f });

        m_addedLabel->setPosition({ bgWidth - (maxTextWidth / 2) - 10.f, bgHeight / 2 - 7.f });
    }

    void scheduleExit() {
        this->stopActionByTag(100);
        auto seq = CCSequence::create(
            CCDelayTime::create(4.5f),
            CCCallFunc::create(this, callfunc_selector(RewardNotification::onAnimationEnd)),
            nullptr
        );
        seq->setTag(100);
        this->runAction(seq);
    }

    void initialParticles(float dt) {
        this->runParticleAnimation(m_addedAmount);
    }

    void runParticleAnimation(int amountForThisWave) {
        auto winSize = CCDirector::sharedDirector()->getWinSize();

      
        int particleCount = amountForThisWave;
        if (particleCount > 30) particleCount = 30;
        if (particleCount < 1) particleCount = 1;

        float valPerParticle = (float)amountForThisWave / (float)particleCount;
        std::string valStr = fmt::format("{}", valPerParticle);

        CCPoint centerPos = this->convertToNodeSpace(winSize / 2);

        for (int i = 0; i < particleCount; ++i) {
            auto particle = CCSprite::create(m_spriteName.c_str());
            if (!particle) particle = CCSprite::createWithSpriteFrameName(m_spriteName.c_str());
            if (!particle) continue;

            auto dataObj = CCString::create(valStr);
            particle->setUserObject(dataObj);

            particle->setPosition(centerPos);
            particle->setScale(0.0f);
            this->addChild(particle, 100);

            float angle = (float)(rand() % 360) * (3.14159f / 180.f);
            float distance = 40.f + (rand() % 30);
            CCPoint explosionOffset = ccp(cosf(angle) * distance, sinf(angle) * distance);
            CCPoint popPosition = centerPos + explosionOffset;

          
            float sequenceDelay = i * 0.05f;

            float popTime = 0.4f;
            auto popMove = CCEaseOut::create(CCMoveTo::create(popTime, popPosition), 2.0f);
            auto popScale = CCScaleTo::create(popTime, 0.5f);
            auto popFade = CCFadeIn::create(0.1f);

            ccBezierConfig bezier;
            bezier.endPosition = m_bg->getPosition();
            bezier.controlPoint_1 = popPosition + (explosionOffset * 0.5f);
            bezier.controlPoint_2 = m_bg->getPosition() + ccp((rand() % 100) - 50, -50);

            auto bezierTo = CCBezierTo::create(0.7f, bezier);
            auto scaleDown = CCScaleTo::create(0.7f, 0.2f);

            particle->runAction(CCSequence::create(
                CCDelayTime::create(sequenceDelay),
                CCSpawn::create(popMove, popScale, popFade, nullptr),
                CCSpawn::create(
                    CCEaseSineIn::create(bezierTo), scaleDown, CCRotateBy::create(0.7f, 360), nullptr
                ),
                CCCallFuncN::create(this, callfuncN_selector(RewardNotification::onParticleHit)),
                CCRemoveSelf::create(),
                nullptr
            ));
        }
    }

    void onParticleHit(CCNode* sender) {
        FMODAudioEngine::sharedEngine()->playEffect("coin.mp3"_spr);

        float valToAdd = 0.f;
        if (auto dataObj = static_cast<CCString*>(sender->getUserObject())) {
            valToAdd = dataObj->floatValue();
        }

        m_currentAccumulator += valToAdd;
        int displayValue = static_cast<int>(std::round(m_currentAccumulator));

        if (displayValue > m_targetTotal) displayValue = m_targetTotal;

        m_label->setString(std::to_string(displayValue).c_str());
    

        m_label->stopAllActions();
        m_label->setScale(0.55f);
        m_label->runAction(CCScaleTo::create(0.1f, 0.45f));
    }

    void onAnimationEnd() {
        m_label->setString(std::to_string(m_targetTotal).c_str());

        auto slideOff = CCEaseBackIn::create(
            CCMoveBy::create(0.5f, ccp(m_bg->getContentSize().width + 100.f, 0))
        );
        slideOff->setTag(200);

        auto seq = CCSequence::create(
            slideOff,
            CCCallFunc::create(this, callfunc_selector(RewardNotification::cleanupAndRemove)),
            nullptr
        );
        seq->setTag(200);
        m_bg->runAction(seq);
    }

    void cleanupAndRemove() {
        auto it = std::find(s_activeRewards.begin(), s_activeRewards.end(), this);
        if (it == s_activeRewards.end()) {
            return;
        }

        s_activeRewards.erase(it);
        RewardNotification::updatePositions();
        this->removeFromParentAndCleanup(true);
    }
};


inline std::vector<RewardNotification*> RewardNotification::s_activeRewards;
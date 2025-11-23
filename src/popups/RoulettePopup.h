#pragma once
#include "StreakCommon.h"
#include "SharedVisuals.h" 
#include "ShopPopup.h"
#include "../FirebaseManager.h"
#include <Geode/utils/cocos.hpp>

#include "../BadgeNotification.h"
#include "../RewardNotification.h"


class RoulettePopup : public Popup<> {
protected:
    CCNode* m_rouletteNode;
    CCSprite* m_selectorSprite;
    CCClippingNode* m_adsClipper = nullptr;
    CCNode* m_adsContainer = nullptr;
    std::vector<CCSprite*> m_adSprites;
    int m_currentAdIndex = 0;
    float m_timeSinceLastAdSwitch = 0.0f;
    bool m_isAdAnimating = false;
    const float AD_DISPLAY_TIME = 4.0f;
    const float AD_SLIDE_DURATION = 0.5f; 
    const float AD_SIZE = 98.0f;
//-- 
    CCMenuItemSpriteExtra* m_spinBtn;
    CCMenuItemSpriteExtra* m_spin10Btn;
    CCMenu* m_shopMenu = nullptr;
    CCMenu* m_infoMenu = nullptr;
    CCMenuItemToggler* m_skipToggle = nullptr;
    bool m_isSpinning = false;
  //--
    std::vector<CCNode*> m_orderedSlots;
    std::vector<RoulettePrize> m_roulettePrizes;
    int m_currentSelectorIndex = 0;
    int m_totalSteps = 0;
  //---
    std::vector<CCSprite*> m_mythicSprites;
    std::vector<ccColor3B> m_mythicColors;
    int m_colorIndex = 0;
    float m_colorTransitionTime = 0.0f;
    ccColor3B m_currentColor;
    ccColor3B m_targetColor;
    float m_slotSize = 0.f;
  //--
    std::vector<GenericPrizeResult> m_multiSpinResults;
    std::vector<StreakData::BadgeInfo> m_pendingMythics;
    std::vector<std::string> m_newBadgesWon;
    int m_pendingTotalTickets = 0;

    void runPopAnimation(CCNode* node) {
        if (node) {
            node->stopAllActions();
            node->setScale(1.0f);
            node->runAction(CCSequence::create(
                CCScaleTo::create(0.05f, 1.15f),
                CCScaleTo::create(0.05f, 1.0f),
                nullptr)
            );
        }
    }

    void runSlotAnimation(CCNode* node) {
        if (node) {
            node->runAction(CCSequence::create(
                CCScaleTo::create(0.02f, 1.1f),
                CCScaleTo::create(0.02f, 1.0f),
                nullptr)
            );
        }
    }

    
    void flashSlot(CCNode* slotNode) {
        if (!slotNode) return;

        auto flashSprite = CCSprite::create("cuadro.png"_spr);
        if (flashSprite) {
            flashSprite->setScale(m_slotSize / flashSprite->getContentSize().width);
            flashSprite->setPosition(slotNode->getPosition());

            flashSprite->setBlendFunc({ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA });
            flashSprite->setOpacity(80);

            m_rouletteNode->addChild(flashSprite, 4);

            flashSprite->runAction(CCSequence::create(
                CCFadeOut::create(0.6f),
                CCRemoveSelf::create(),
                nullptr
            ));
        }
    }

    void toggleUI(bool enabled) {
        if (m_spinBtn) m_spinBtn->setEnabled(enabled);
        if (m_spin10Btn) m_spin10Btn->setEnabled(enabled);
        if (m_shopMenu) m_shopMenu->setEnabled(enabled);
        if (m_infoMenu) m_infoMenu->setEnabled(enabled);
        if (m_skipToggle) m_skipToggle->setEnabled(enabled);
    }

    void setupAdsCarousel() {
        auto stencil = CCSprite::create("square02_001.png");
        stencil->setScale(AD_SIZE / stencil->getContentSize().width);

        m_adsClipper = CCClippingNode::create(stencil);
        m_adsClipper->setPosition({ 0, 0 });
        m_adsClipper->setAlphaThreshold(0.5f);
        m_rouletteNode->addChild(m_adsClipper, 0);
        m_adsContainer = CCNode::create();
        m_adsClipper->addChild(m_adsContainer);

     
        std::vector<std::string> images = { "publi_1.png"_spr, "publi_2.png"_spr, "publi_3.png"_spr};

        for (int i = 0; i < images.size(); i++) {
            auto spr = CCSprite::create(images[i].c_str());

            if (!spr) {
                spr = CCSprite::create("square02_001.png");
                spr->setColor({ 255, 0, 0 });
            }

            if (spr) {
                float scaleX = AD_SIZE / spr->getContentSize().width;
                float scaleY = AD_SIZE / spr->getContentSize().height;
                float finalScale = std::max(scaleX, scaleY);
                spr->setScale(finalScale);
                if (i == 0) spr->setPosition({ 0, 0 });
                else spr->setPosition({ AD_SIZE + 10.f, 0 });

                spr->setVisible(i == 0);
                m_adsContainer->addChild(spr);
                m_adSprites.push_back(spr);
            }
        }
    }


    void switchAd() {
        if (m_adSprites.size() < 2) return;
        m_isAdAnimating = true;

        int nextIndex = (m_currentAdIndex + 1) % m_adSprites.size();

        auto currentSprite = m_adSprites[m_currentAdIndex];
        auto nextSprite = m_adSprites[nextIndex];

       
        nextSprite->setVisible(true);
        nextSprite->setPosition({ AD_SIZE + 5.f, 0 });
        auto moveOut = CCEaseSineInOut::create(CCMoveTo::create(AD_SLIDE_DURATION, ccp(-(AD_SIZE + 5.f), 0)));
        auto hideAction = CallFuncExt::create([currentSprite]() {
            currentSprite->setVisible(false); 
            });

        auto hideAfter = CCCallFunc::create(this, callfunc_selector(RoulettePopup::onAdAnimationFinished)); 
        auto moveIn = CCEaseSineInOut::create(CCMoveTo::create(AD_SLIDE_DURATION, ccp(0, 0)));
        currentSprite->runAction(CCSequence::create(moveOut, hideAction, nullptr));
        nextSprite->runAction(CCSequence::create(moveIn, hideAfter, nullptr));

       
        m_currentAdIndex = nextIndex;
    }

    void onAdAnimationFinished() {
        m_isAdAnimating = false;
        m_timeSinceLastAdSwitch = 0.0f;
    }

    bool setup() override {
        this->setTitle("Roulette");
        auto winSize = m_mainLayer->getContentSize();
        g_streakData.load();
        m_currentSelectorIndex = g_streakData.lastRouletteIndex;

        m_mythicColors = {
            ccc3(255, 0, 0),
            ccc3(255, 165, 0),
            ccc3(255, 255, 0),
            ccc3(0, 255, 0),
            ccc3(0, 0, 255), 
            ccc3(75, 0, 130), 
            ccc3(238, 130, 238) 
        };

        m_currentColor = m_mythicColors[0]; m_targetColor = m_mythicColors[1];

        m_rouletteNode = CCNode::create();
        m_rouletteNode->setPosition({ winSize.width / 2, winSize.height / 2 + 10.f });
        m_mainLayer->addChild(m_rouletteNode);
        this->setupAdsCarousel();
       

        m_roulettePrizes = {
            { RewardType::Badge, "bh_badge_7", 1, "", "Black Hole Galaxy", 1, StreakData::BadgeCategory::MYTHIC },
            { RewardType::Badge, "gold_streak_badge", 1, "", "Gold Legend's", 3, StreakData::BadgeCategory::LEGENDARY },
            { RewardType::Badge, "mc_badge_2", 1, "", "full farming", 5, StreakData::BadgeCategory::EPIC },
            { RewardType::Badge, "bh_badge_3", 1, "", "Black hole Green", 10, StreakData::BadgeCategory::SPECIAL },
            { RewardType::Badge, "mc_badge_3", 1, "", "break shields", 70, StreakData::BadgeCategory::COMMON },
            { RewardType::Badge, "bh_badge_4", 1, "", "Black hole blue", 70, StreakData::BadgeCategory::COMMON },
            { RewardType::StarTicket, "star_tiket_1", 1, "star_tiket.png"_spr, "1 star tiket", 70, StreakData::BadgeCategory::COMMON },
            { RewardType::StarTicket, "star_tiket_3", 3, "star_tiket.png"_spr, "3 star tiket", 70, StreakData::BadgeCategory::COMMON },
            { RewardType::StarTicket, "star_ticket_15", 15, "star_tiket.png"_spr, "15 Tickets", 70, StreakData::BadgeCategory::COMMON },
            { RewardType::StarTicket, "star_ticket_30", 30, "star_tiket.png"_spr, "30 Tickets", 10, StreakData::BadgeCategory::SPECIAL },
            { RewardType::StarTicket, "star_ticket_60", 60, "star_tiket.png"_spr, "60 Tickets", 5, StreakData::BadgeCategory::EPIC },
            { RewardType::Badge, "mc_badge_1", 1, "mc_badge_1.png"_spr, "PVP Master", 3, StreakData::BadgeCategory::LEGENDARY }
        };

        const int gridSize = 4;
        m_slotSize = 38.f;
        const float spacing = 5.f;
        const float totalSize = (gridSize * m_slotSize) + ((gridSize - 1) * spacing);
        const float startPos = -totalSize / 2.f + m_slotSize / 2.f;
        std::vector<CCPoint> slotPositions;

        for (int i = 0; i < gridSize; ++i) slotPositions.push_back({
            startPos + i * (m_slotSize + spacing), 
            startPos + (gridSize - 1) * (m_slotSize + spacing)
            });

        for (int i = gridSize - 2; i > 0; --i) slotPositions.push_back({ 
            startPos + (gridSize - 1) * (m_slotSize + spacing),
            startPos + i * (m_slotSize + spacing) 
            });

        for (int i = gridSize - 1; i >= 0; --i) slotPositions.push_back({
            startPos + i * (m_slotSize + spacing),
            startPos 
            });

        for (int i = 1; i < gridSize - 1; ++i) slotPositions.push_back({ 
            startPos,
            startPos + i * (m_slotSize + spacing)
            });


        for (size_t i = 0; i < slotPositions.size(); ++i) {
            if (i >= m_roulettePrizes.size()) continue;
            auto& currentPrize = m_roulettePrizes[i];
            auto slotContainer = CCNode::create();
            slotContainer->setPosition(slotPositions[i]);
            m_rouletteNode->addChild(slotContainer);
            m_orderedSlots.push_back(slotContainer);

            auto qualitySprite = CCSprite::create(getQualitySpriteName(currentPrize.category).c_str());
            qualitySprite->setScale((m_slotSize - 4.f) / qualitySprite->getContentSize().width);
            slotContainer->addChild(qualitySprite, 1);

            if (currentPrize.category == StreakData::BadgeCategory::MYTHIC) m_mythicSprites.push_back(qualitySprite);

            if (currentPrize.type == RewardType::Badge) {
                auto* badgeInfo = g_streakData.getBadgeInfo(currentPrize.id);
                if (badgeInfo) {
                    auto rewardIcon = CCSprite::create(badgeInfo->spriteName.c_str());
                    rewardIcon->setScale(0.15f);
                    slotContainer->addChild(rewardIcon, 2);
                    if (g_streakData.isBadgeUnlocked(badgeInfo->badgeID)) {
                        auto claimedIcon = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
                        claimedIcon->setScale(0.5f);
                        claimedIcon->setPosition({
                            m_slotSize / 2 - 5, 
                            -m_slotSize / 2 + 5 
                            });

                        claimedIcon->setTag(199);
                        slotContainer->addChild(claimedIcon, 3);
                    }
                }
            }
            else {
                auto rewardIcon = CCSprite::create(currentPrize.spriteName.c_str());
                rewardIcon->setScale(0.2f);
                slotContainer->addChild(rewardIcon, 2);
                auto quantityLabel = CCLabelBMFont::create(
                    CCString::createWithFormat(
                        "x%d",
                        currentPrize.quantity)->getCString(),
                    "goldFont.fnt");
                quantityLabel->setScale(0.3f);
                quantityLabel->setPosition(0, -m_slotSize / 2 + 5);
                slotContainer->addChild(quantityLabel, 3);
            }
        }

        m_selectorSprite = CCSprite::create("casilla_selector.png"_spr);
        m_selectorSprite->setScale(m_slotSize / m_selectorSprite->getContentSize().width);
        if (!m_orderedSlots.empty()) m_selectorSprite->setPosition(
            m_orderedSlots[m_currentSelectorIndex]->getPosition()
        );

        m_rouletteNode->addChild(m_selectorSprite, 5);

        m_spinBtn = CCMenuItemSpriteExtra::create(ButtonSprite::create("Spin"), this, menu_selector(RoulettePopup::onSpin));
        m_spin10Btn = CCMenuItemSpriteExtra::create(ButtonSprite::create("x10"), this, menu_selector(RoulettePopup::onSpinMultiple));


        auto spinMenu = CCMenu::create();
        spinMenu->addChild(m_spinBtn); spinMenu->addChild(m_spin10Btn);
        spinMenu->alignItemsHorizontallyWithPadding(10.f);
        spinMenu->setPosition({ winSize.width / 2, 30.f });
        m_mainLayer->addChild(spinMenu);

        auto shopSprite = CCSprite::create("shop_btn.png"_spr);
        shopSprite->setScale(0.7f);
        auto shopBtn = CCMenuItemSpriteExtra::create(
            shopSprite, 
            this,
            menu_selector(RoulettePopup::onOpenShop)
        );

        m_shopMenu = CCMenu::createWithItem(shopBtn);
        m_shopMenu->setPosition({ winSize.width - 35.f, 30.f });
        m_mainLayer->addChild(m_shopMenu);

        
        auto infoIcon = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"); infoIcon->setScale(0.8f);
        auto infoBtn = CCMenuItemSpriteExtra::create(
            infoIcon, 
            this,
            menu_selector(RoulettePopup::onShowProbabilities)
        );

        m_infoMenu = CCMenu::createWithItem(infoBtn);
        m_infoMenu->setPosition({ 25.f, winSize.height - 25.f });
        m_mainLayer->addChild(m_infoMenu);

      
        auto skipMenu = CCMenu::create();
        skipMenu->setPosition({ 35.f, 35.f });
        m_mainLayer->addChild(skipMenu);

        m_skipToggle = CCMenuItemToggler::createWithStandardSprites(
            this,
            menu_selector(RoulettePopup::onToggleSkip),
            0.6f
        );

        m_skipToggle->setPosition({ 0, 0 });
        skipMenu->addChild(m_skipToggle);

        auto skipLabel = CCLabelBMFont::create("Skip", "bigFont.fnt");
        skipLabel->setScale(0.35f);
        skipLabel->setPosition({ 0.f, 20.f });
        skipLabel->setAlignment(kCCTextAlignmentCenter);
        skipMenu->addChild(skipLabel);

     
        auto superStarCounterNode = CCNode::create();
        auto haveAmountLabel = CCLabelBMFont::create(std::to_string(g_streakData.superStars).c_str(), "goldFont.fnt");
        haveAmountLabel->setScale(0.4f); haveAmountLabel->setAnchorPoint({ 1.f, 0.5f });
        haveAmountLabel->setID("roulette-super-star-label");
        superStarCounterNode->addChild(haveAmountLabel);
        auto haveStar = CCSprite::create("super_star.png"_spr);
        haveStar->setScale(0.12f); superStarCounterNode->addChild(haveStar);
        haveAmountLabel->setPosition({ -haveStar->getScaledContentSize().width / 2 - 2, 0 });
        haveStar->setPosition({ haveAmountLabel->getScaledContentSize().width / 2, 0 });
        superStarCounterNode->setPosition({ winSize.width - 35.f, winSize.height - 25.f });
        m_mainLayer->addChild(superStarCounterNode);

        this->scheduleUpdate();
        return true;
    }

    void onToggleSkip(CCObject* sender) {}

    void update(float dt) override {
       
        if (!m_mythicSprites.empty()) {
            m_colorTransitionTime += dt;
            if (m_colorTransitionTime >= 1.0f) {
                m_colorTransitionTime = 0.0f;
                m_colorIndex = (m_colorIndex + 1) % m_mythicColors.size();
                m_currentColor = m_targetColor;
                m_targetColor = m_mythicColors[(m_colorIndex + 1) % m_mythicColors.size()];
            }
            float progress = m_colorTransitionTime / 1.0f;
            ccColor3B interpolatedColor = {
                static_cast<GLubyte>(m_currentColor.r + (m_targetColor.r - m_currentColor.r) * progress),
                static_cast<GLubyte>(m_currentColor.g + (m_targetColor.g - m_currentColor.g) * progress),
                static_cast<GLubyte>(m_currentColor.b + (m_targetColor.b - m_currentColor.b) * progress)
            };
            for (auto* sprite : m_mythicSprites) sprite->setColor(interpolatedColor);
        }

      
        if (!m_isAdAnimating && !m_adSprites.empty()) {
            m_timeSinceLastAdSwitch += dt;
            if (m_timeSinceLastAdSwitch >= AD_DISPLAY_TIME) {
                switchAd();
            }
        }
    }

    void onSpin(CCObject*) {
        g_streakData.load();
        if (g_streakData.superStars < 1) {
            FLAlertLayer::create(
                "Not Enough Super Stars",
                "You need 1 Super Star.",
                "OK")->show(); 
            return;
        }

        if (m_isSpinning) return;

        m_isSpinning = true;
        toggleUI(false);

        g_streakData.superStars -= 1;
        g_streakData.totalSpins += 1;
        g_streakData.save();
        if (auto lbl = static_cast<CCLabelBMFont*>(
            m_mainLayer->getChildByIDRecursive("roulette-super-star-label"))
            )
            lbl->setString(
                std::to_string(g_streakData.superStars).c_str()
            );


        int totalWeight = 0; for (const auto& prize : m_roulettePrizes) totalWeight += prize.probabilityWeight;
        int randomValue = rand() % totalWeight;
        int winningIndex = 0;
        for (size_t i = 0; i < m_roulettePrizes.size(); ++i) {
            if (randomValue < m_roulettePrizes[i].probabilityWeight) {
                winningIndex = i;
                break;
            }

            randomValue -= m_roulettePrizes[i].probabilityWeight;
        }

        if (m_skipToggle->isToggled()) {
            m_currentSelectorIndex = winningIndex;
            m_selectorSprite->setPosition(m_orderedSlots[winningIndex]->getPosition());
            this->onSpinEnd();
            return;
        }

        int totalSlots = m_orderedSlots.size();
        m_totalSteps = (5 * totalSlots) + (winningIndex - m_currentSelectorIndex + totalSlots) % totalSlots;

        auto actions = CCArray::create();
        for (int i = 1; i <= m_totalSteps; ++i) {
            int stepIndex = (m_currentSelectorIndex + i) % totalSlots;
            float duration = (i < 5) ? 0.2f - (i * 0.03f) : (i > m_totalSteps - 10) ? 0.05f + ((i - (m_totalSteps - 10)) * 0.04f) : 0.05f;

            auto moveAction = CCEaseSineInOut::create(CCMoveTo::create(
                duration,
                m_orderedSlots[stepIndex]->getPosition())
            );

            auto arriveEffects = CallFuncExt::create([this, stepIndex]() {
                this->playTickSound();
                this->runPopAnimation(m_orderedSlots[stepIndex]);
                this->flashSlot(m_orderedSlots[stepIndex]);
                });

            actions->addObject(CCSequence::create(moveAction, arriveEffects, nullptr));
        }
        actions->addObject(CCCallFunc::create(
            this, 
            callfunc_selector(RoulettePopup::onSpinEnd))
        );
        m_selectorSprite->runAction(CCSequence::create(actions));
        updatePlayerDataInFirebase();
    }

    void onSpinMultiple(CCObject*) {
        g_streakData.load();
        if (g_streakData.superStars < 10) { 
            FLAlertLayer::create("Error",
                "Need 10 Super Stars.",
                "OK")->show();
            return; 
        }

        if (m_isSpinning) return;

        m_isSpinning = true;
        toggleUI(false);

        g_streakData.superStars -= 10; g_streakData.totalSpins += 10;

        m_multiSpinResults.clear();
        m_pendingMythics.clear();
        m_newBadgesWon.clear(); 

        std::vector<int> winningIndices;
        int totalWeight = 0; for (const auto& prize : m_roulettePrizes) totalWeight += prize.probabilityWeight;

        for (int i = 0; i < 10; ++i) {
            int rVal = rand() % totalWeight; int wIndex = 0;
            for (size_t j = 0; j < m_roulettePrizes.size(); ++j) {
                if (rVal < m_roulettePrizes[j].probabilityWeight) {
                    wIndex = j; break;
                } 
                rVal -= m_roulettePrizes[j].probabilityWeight;
            }

            winningIndices.push_back(wIndex);

            auto& prize = m_roulettePrizes[wIndex];
            GenericPrizeResult res; res.type = prize.type;
            res.id = prize.id; res.quantity = prize.quantity;
            res.displayName = prize.displayName;
            res.category = prize.category;

            if (prize.type == RewardType::Badge) {
                auto* bInfo = g_streakData.getBadgeInfo(prize.id);
                if (bInfo) {
                    res.spriteName = bInfo->spriteName;
                    res.isNew = !g_streakData.isBadgeUnlocked(prize.id);

                    if (res.isNew) {
                        g_streakData.unlockBadge(prize.id);
                        m_newBadgesWon.push_back(prize.id);
                    

                        if (bInfo->category == StreakData::BadgeCategory::MYTHIC) m_pendingMythics.push_back(*bInfo);
                    }
                    else {
                        int t = g_streakData.getTicketValueForRarity(bInfo->category);
                        g_streakData.starTickets += t;
                        res.ticketsFromDuplicate = t;
                    }
                }
            }
            else {
                if (prize.type == RewardType::SuperStar) g_streakData.superStars += prize.quantity;
                else if (prize.type == RewardType::StarTicket) g_streakData.starTickets += prize.quantity;
                res.spriteName = prize.spriteName;
            }
            m_multiSpinResults.push_back(res);
        }

        g_streakData.save();
        if (auto lbl = static_cast<CCLabelBMFont*>(
            m_mainLayer->getChildByIDRecursive(
                "roulette-super-star-label"
            )))
            lbl->setString(std::to_string(g_streakData.superStars).c_str()
            );

        if (m_skipToggle->isToggled()) {
            m_currentSelectorIndex = winningIndices.back();
            m_selectorSprite->setPosition(
                m_orderedSlots[m_currentSelectorIndex]->getPosition()
            );
            this->onMultiSpinEnd();
            return;
        }

        auto actions = CCArray::create();
        int totalSlots = m_orderedSlots.size(); int lastIdx = m_currentSelectorIndex;
        for (int pIdx : winningIndices) {
            int dist = (pIdx - lastIdx + totalSlots) % totalSlots; if (dist == 0) dist = totalSlots;
            for (int i = 1; i <= dist; ++i) {
                int currentSlotIdx = (lastIdx + i) % totalSlots;

                auto moveAction = CCMoveTo::create(0.04f, m_orderedSlots[currentSlotIdx]->getPosition());
                auto arriveEffects = CallFuncExt::create([this, currentSlotIdx]() {
                    this->playTickSound();
                    this->runSlotAnimation(m_orderedSlots[currentSlotIdx]);
                    this->flashSlot(m_orderedSlots[currentSlotIdx]);
                    });

                actions->addObject(CCSequence::create(moveAction, arriveEffects, nullptr));
            }
            actions->addObject(CCDelayTime::create(0.25f));
            lastIdx = pIdx;
        }
        m_currentSelectorIndex = lastIdx;
        actions->addObject(CCCallFunc::create(this, callfunc_selector(RoulettePopup::onMultiSpinEnd)));
        m_selectorSprite->runAction(CCSequence::create(actions));
        updatePlayerDataInFirebase();
    }

    void processMythicQueue() {
        if (!m_pendingMythics.empty()) {
            auto mb = m_pendingMythics.front();
            m_pendingMythics.erase(m_pendingMythics.begin());

         
            auto al = MythicAnimationLayer::create(mb, [this]() { 
                this->processMythicQueue();
                });
            CCDirector::sharedDirector()->getRunningScene()->addChild(al, 400);
        }
        else {      
            showMultiSpinSummary();
        }
    }

    void onMultiSpinEnd() {
        m_isSpinning = false;
        g_streakData.lastRouletteIndex = m_currentSelectorIndex;
        g_streakData.save();
        updateAllCheckmarks();
        m_pendingTotalTickets = 0;
        for (const auto& r : m_multiSpinResults) {
            if (r.type == RewardType::StarTicket) m_pendingTotalTickets += r.quantity;
            if (r.type == RewardType::Badge && !r.isNew) m_pendingTotalTickets += r.ticketsFromDuplicate;
        }

        if (!m_pendingMythics.empty()) processMythicQueue();
        else showMultiSpinSummary();
    }

  
    void showMultiSpinSummary() {
        MultiPrizePopup::create(m_multiSpinResults, [this]() {
            this->toggleUI(true);
            for (const auto& badgeID : m_newBadgesWon) {
                BadgeNotification::show(badgeID);
            }

         
            if (m_pendingTotalTickets > 0) {
                int startAmount = g_streakData.starTickets - m_pendingTotalTickets;
                RewardNotification::show(
                    "star_tiket.png"_spr,
                    startAmount,
                    m_pendingTotalTickets
                );
            }

            // Actualizar UI
            if (auto lbl = static_cast<CCLabelBMFont*>(m_mainLayer->getChildByIDRecursive("roulette-super-star-label"))) {
                lbl->setString(std::to_string(g_streakData.superStars).c_str());
            }

            })->show();
    }

    void onSpinEnd() {
        m_isSpinning = false;
        toggleUI(true);

      
        m_currentSelectorIndex = (m_currentSelectorIndex + m_totalSteps) % m_orderedSlots.size();
        g_streakData.lastRouletteIndex = m_currentSelectorIndex;
        auto& prize = m_roulettePrizes[m_currentSelectorIndex];
        int pendingTickets = 0;
        int pendingStars = 0;
        bool showBadgeNotify = false;
        bool isMythicAnimation = false;

        if (prize.type == RewardType::Badge) {
            auto* bi = g_streakData.getBadgeInfo(prize.id);
            if (bi) {
                bool isNew = !g_streakData.isBadgeUnlocked(prize.id);

                if (isNew) {               
                    g_streakData.unlockBadge(prize.id);
                    if (bi->category == StreakData::BadgeCategory::MYTHIC) {
                        isMythicAnimation = true;
                        auto animLayer = MythicAnimationLayer::create(*bi, [this, prize]() {
                            BadgeNotification::show(prize.id);
                            });

                     
                        CCDirector::sharedDirector()->getRunningScene()->addChild(animLayer, 400);
                    }
                    else {
                        showBadgeNotify = true;
                    }
                }
                else {
               
                    int val = g_streakData.getTicketValueForRarity(bi->category);
                    pendingTickets = val;
                    g_streakData.starTickets += val;
                }
            }
        }
        else if (prize.type == RewardType::SuperStar) {
            pendingStars = prize.quantity;
            g_streakData.superStars += prize.quantity;
        }
        else if (prize.type == RewardType::StarTicket) {
            pendingTickets = prize.quantity;
            g_streakData.starTickets += prize.quantity;
        }

       
        g_streakData.save();
        updateAllCheckmarks();

       
        if (auto lbl = static_cast<CCLabelBMFont*>(m_mainLayer->getChildByIDRecursive("roulette-super-star-label"))) {
            lbl->setString(std::to_string(g_streakData.superStars).c_str());
        }

      
        if (!isMythicAnimation) {
            if (showBadgeNotify) {
                BadgeNotification::show(prize.id);
            }

            if (pendingTickets > 0) {
                int current = g_streakData.starTickets - pendingTickets; 
                RewardNotification::show("star_tiket.png"_spr, current, pendingTickets);
            }

            if (pendingStars > 0) {
                int current = g_streakData.superStars - pendingStars;
                RewardNotification::show("super_star.png"_spr, current, pendingStars);
            }
        }
    }

    void updateAllCheckmarks() {
        for (size_t i = 0; i < m_orderedSlots.size(); ++i) {
            if (i >= m_roulettePrizes.size()) continue;
            if (m_roulettePrizes[i].type != RewardType::Badge) continue;
            if (g_streakData.isBadgeUnlocked(m_roulettePrizes[i].id) && !m_orderedSlots[i]->getChildByTag(199)) {
                auto cm = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
                cm->setScale(0.5f); cm->setPosition({ m_slotSize / 2 - 5, -m_slotSize / 2 + 5 }); cm->setTag(199);
                m_orderedSlots[i]->addChild(cm, 3);
            }
        }
    }
    void playTickSound() { FMODAudioEngine::sharedEngine()->playEffect("ruleta_sfx.mp3"_spr); }
    void onOpenShop(CCObject*) {
        auto shop = ShopPopup::create([this]() {
            this->updateAllCheckmarks();          
            if (auto lbl = static_cast<CCLabelBMFont*>(
                m_mainLayer->getChildByIDRecursive("roulette-super-star-label")))
            {
                lbl->setString(std::to_string(g_streakData.superStars).c_str());
            }
            });

        shop->show();
    }

    void onShowProbabilities(CCObject*) {
        g_streakData.load();

        int totalWeight = 0;
        for (const auto& prize : m_roulettePrizes) totalWeight += prize.probabilityWeight;

        std::map<StreakData::BadgeCategory, int> weightsByCategory;
        for (const auto& prize : m_roulettePrizes) {
            weightsByCategory[prize.category] += prize.probabilityWeight;
        }

        std::string infoText = fmt::format("Total Spins: {}\n\n", g_streakData.totalSpins);

        struct CategoryDisplay {
            StreakData::BadgeCategory cat;
            std::string name;
            std::string colorTag;
        };

        std::vector<CategoryDisplay> displayOrder = {
            { StreakData::BadgeCategory::COMMON, "Common", "<cl>" },
            { StreakData::BadgeCategory::SPECIAL, "Special", "<cy>" },
            { StreakData::BadgeCategory::EPIC, "Epic", "<co>" },
            { StreakData::BadgeCategory::LEGENDARY, "Legendary", "<cr>" },
            { StreakData::BadgeCategory::MYTHIC, "Mythic", "<cp>" }
        };

        for (const auto& item : displayOrder) {
            int weight = weightsByCategory[item.cat];
            if (weight > 0) {
                float percent = (static_cast<float>(weight) / totalWeight) * 100.0f;
                char buffer[50];
                snprintf(buffer, sizeof(buffer), "%.1f%%", percent);
                infoText += fmt::format("{}{}</c>: {}\n", item.colorTag, item.name, buffer);
            }
        }

        FLAlertLayer::create("Probabilities", infoText, "OK")->show();
    }

    std::string getQualitySpriteName(StreakData::BadgeCategory c) {
        switch (c) { case StreakData::BadgeCategory::SPECIAL: return "casilla_especial.png"_spr;
        case StreakData::BadgeCategory::EPIC: return "casilla_epica.png"_spr;
        case StreakData::BadgeCategory::LEGENDARY: return "casilla_legendaria.png"_spr;
        case StreakData::BadgeCategory::MYTHIC: return "casilla_mitica.png"_spr;
        default: return "casilla_comun.png"_spr;
        }
    }

public:
    static RoulettePopup* create() {
        auto ret = new RoulettePopup();
        if (ret && ret->initAnchored(260.f, 260.f)) {
            ret->autorelease(); 
            return ret; 
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
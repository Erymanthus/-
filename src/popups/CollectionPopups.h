#pragma once
#include "StreakCommon.h"
#include "../StreakData.h"
#include "../FirebaseManager.h"
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>


class EquipBadgePopup : public Popup<std::string> {
protected:
    std::string m_badgeID;
    bool m_isCurrentlyEquipped;

    bool setup(std::string badgeID) override {
        m_badgeID = badgeID;
        auto winSize = m_mainLayer->getContentSize();
        auto badgeInfo = g_streakData.getBadgeInfo(badgeID);
        if (!badgeInfo) return false;

        auto equippedBadge = g_streakData.getEquippedBadge();
        m_isCurrentlyEquipped = (equippedBadge && equippedBadge->badgeID == badgeID);

        this->setTitle(m_isCurrentlyEquipped ? "Badge Equipped" : "Equip Badge");

        auto badgeSprite = CCSprite::create(badgeInfo->spriteName.c_str());
        if (badgeSprite) {
            badgeSprite->setScale(0.3f);
            badgeSprite->setPosition({
                winSize.width / 2,
                winSize.height / 2 + 20 
                });
            m_mainLayer->addChild(badgeSprite);
        }

        auto nameLabel = CCLabelBMFont::create(
            badgeInfo->displayName.c_str(),
            "goldFont.fnt"
        );
        nameLabel->setScale(0.6f);
        nameLabel->setPosition({ 
            winSize.width / 2, 
            winSize.height / 2 - 20
            });
        m_mainLayer->addChild(nameLabel);

        auto categoryLabel = CCLabelBMFont::create(g_streakData.getCategoryName(badgeInfo->category).c_str(), "bigFont.fnt");
        categoryLabel->setScale(0.4f);
        categoryLabel->setPosition({ winSize.width / 2, winSize.height / 2 - 40 });
        categoryLabel->setColor(g_streakData.getCategoryColor(badgeInfo->category));
        m_mainLayer->addChild(categoryLabel);

        auto mainBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create(m_isCurrentlyEquipped ? "Unequip" : "Equip"),
            this, menu_selector(EquipBadgePopup::onToggleEquip)
        );
        auto menu = CCMenu::create(); menu->addChild(mainBtn);
        menu->setPosition(winSize.width / 2, winSize.height / 2 - 70);
        m_mainLayer->addChild(menu);
        return true;
    }

    void onToggleEquip(CCObject*) {
        if (m_isCurrentlyEquipped) {
            g_streakData.unequipBadge();
            FLAlertLayer::create("Success", "Badge unequipped!", "OK")->show();
        }
        else {
            g_streakData.equipBadge(m_badgeID);
            FLAlertLayer::create("Success", "Badge equipped!", "OK")->show();
        }
        g_streakData.save();
        updatePlayerDataInFirebase();
        this->onClose(nullptr);
    }

public:
    static EquipBadgePopup* create(std::string badgeID) {
        auto ret = new EquipBadgePopup();
        if (ret && ret->initAnchored(250.f, 200.f, badgeID)) {
            ret->autorelease(); 
            return ret;
        }
        CC_SAFE_DELETE(ret); 
        return nullptr;
    }
};


class RewardsPopup : public Popup<> {
protected:
    int m_currentCategory = 0;
    int m_currentPage = 0;
    int m_totalPages = 0;
    CCLabelBMFont* m_categoryLabel = nullptr;
    CCNode* m_badgeContainer = nullptr;
    CCMenuItemSpriteExtra* m_pageLeftArrow = nullptr;
    CCMenuItemSpriteExtra* m_pageRightArrow = nullptr;
    CCLabelBMFont* m_counterText = nullptr;
    int m_colorIndex = 0;
    float m_colorTransitionTime = 0.0f;
    std::vector<ccColor3B> m_mythicColors;
    ccColor3B m_currentColor, m_targetColor;

    bool setup() override {
        this->setTitle("Badges Collection");
        auto winSize = m_mainLayer->getContentSize();
        g_streakData.load();
        m_mythicColors = {
            ccc3(255, 0, 0),
            ccc3(255, 165, 0), 
            ccc3(255, 255, 0),
            ccc3(0, 255, 0),
            ccc3(0, 0, 255), 
            ccc3(75, 0, 130),
            ccc3(238, 130, 238),
            ccc3(255, 105, 180),
            ccc3(255, 215, 0), 
            ccc3(192, 192, 192) 
        };

        m_currentColor = m_mythicColors[0];
        m_targetColor = m_mythicColors[1];

        auto background = cocos2d::extension::CCScale9Sprite::create("square02_001.png");
        background->setColor({ 0, 0, 0 }); 
        background->setOpacity(120);
        background->setContentSize({ 320.f, 150.f });
        background->setPosition({
            winSize.width / 2,
            winSize.height / 2 - 15.f 
            }); 
        m_mainLayer->addChild(background);

        auto catLeftArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        auto catLeftBtn = CCMenuItemSpriteExtra::create(
            catLeftArrow, 
            this,
            menu_selector(RewardsPopup::onPreviousCategory
            )); 

        catLeftBtn->setPosition(-110.f, 0);
        auto catRightArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png"); 
        catRightArrow->setFlipX(true);
        auto catRightBtn = CCMenuItemSpriteExtra::create(
            catRightArrow, 
            this,
            menu_selector(RewardsPopup::onNextCategory
            )); 

        catRightBtn->setPosition(110.f, 0);
        auto catArrowMenu = CCMenu::create();
        catArrowMenu->addChild(catLeftBtn);
        catArrowMenu->addChild(catRightBtn);
        catArrowMenu->setPosition({
            winSize.width / 2,
            winSize.height - 40.f 
            });

        m_mainLayer->addChild(catArrowMenu);
        m_categoryLabel = CCLabelBMFont::create("", "goldFont.fnt"); 
        m_categoryLabel->setScale(0.7f);
        m_categoryLabel->setPosition({
            winSize.width / 2, winSize.height - 40.f 
            });

        m_mainLayer->addChild(m_categoryLabel);
        auto pageLeftArrowSprite = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        m_pageLeftArrow = CCMenuItemSpriteExtra::create(
            pageLeftArrowSprite, 
            this, menu_selector(RewardsPopup::onPreviousBadgePage
            )); 

        m_pageLeftArrow->setPosition(-background->getContentSize().width / 2 - 5.f, 0);
        auto pageRightArrowSprite = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png"); 
        pageRightArrowSprite->setFlipX(true);
        m_pageRightArrow = CCMenuItemSpriteExtra::create(
            pageRightArrowSprite,
            this,
            menu_selector(RewardsPopup::onNextBadgePage
            )); 

        m_pageRightArrow->setPosition(background->getContentSize().width / 2 + 5.f, 0);
        auto pageArrowMenu = CCMenu::create();
        pageArrowMenu->addChild(m_pageLeftArrow);
        pageArrowMenu->addChild(m_pageRightArrow); 
        pageArrowMenu->setPosition(background->getPosition());
        m_mainLayer->addChild(pageArrowMenu);

        m_badgeContainer = CCNode::create(); 
        m_mainLayer->addChild(m_badgeContainer);
        m_counterText = CCLabelBMFont::create("", "bigFont.fnt");
        m_counterText->setScale(0.5f);
        m_counterText->setPosition({ winSize.width / 2, 32.f });
        m_mainLayer->addChild(m_counterText);

        this->scheduleUpdate();
        updateCategoryDisplay();
        return true;
    }

    void update(float dt) override {
        if (static_cast<StreakData::BadgeCategory>(m_currentCategory) != StreakData::BadgeCategory::MYTHIC || !m_categoryLabel) {
            m_categoryLabel->setColor(
                g_streakData.getCategoryColor(
                    static_cast<StreakData::BadgeCategory>(
                        m_currentCategory
                        ))); 
            return;
        }

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

        m_categoryLabel->setColor(interpolatedColor);
    }

    void updateCategoryDisplay() {
        m_badgeContainer->removeAllChildren();
        StreakData::BadgeCategory currentCat = static_cast<StreakData::BadgeCategory>(m_currentCategory);
        std::vector<StreakData::BadgeInfo> categoryBadges;
        for (auto& badge : g_streakData.badges) {
            if (badge.category == currentCat) categoryBadges.push_back(badge);
        }

        m_categoryLabel->setString(g_streakData.getCategoryName(currentCat).c_str());
        const int badgesPerPage = 6;
        m_totalPages = static_cast<int>(ceil(static_cast<float>(categoryBadges.size()) / badgesPerPage));
        if (m_currentPage >= m_totalPages && m_totalPages > 0) m_currentPage = m_totalPages - 1;

        int startIndex = m_currentPage * badgesPerPage;
        int endIndex = std::min(
            static_cast<int>(categoryBadges.size()), 
            startIndex + badgesPerPage
        );

        const int badgesPerRow = 3;
        const float horizontalSpacing = 65.f; 
        const float verticalSpacing = 65.f;
        auto winSize = m_mainLayer->getContentSize();
        const CCPoint startPosition = {
            winSize.width / 2 - horizontalSpacing, 
            winSize.height / 2 + 10.f
        };


        for (int i = startIndex; i < endIndex; ++i) {
            int indexOnPage = i - startIndex;
            int row = indexOnPage / badgesPerRow;
            int col = indexOnPage % badgesPerRow;
            auto& badge = categoryBadges[i];
            auto badgeSprite = CCSprite::create(badge.spriteName.c_str());
            badgeSprite->setScale(0.3f);
            bool unlocked = g_streakData.isBadgeUnlocked(badge.badgeID);
            if (!unlocked) badgeSprite->setColor({ 100, 100, 100 });

            CCMenuItemSpriteExtra* badgeBtn;
            if (unlocked) {
                badgeBtn = CCMenuItemSpriteExtra::create(
                    badgeSprite, 
                    this, 
                    menu_selector(RewardsPopup::onBadgeClick
                    ));

                badgeBtn->setUserObject(
                    "badge"_spr,
                    CCString::create(badge.badgeID
                    ));
            }
            else {
                badgeBtn = CCMenuItemSpriteExtra::create(
                    badgeSprite, this, nullptr);
                badgeBtn->setEnabled(false);
            }
            CCPoint position = {
                startPosition.x + col * horizontalSpacing, 
                startPosition.y - row * verticalSpacing 
            };

            auto badgeMenu = CCMenu::createWithItem(badgeBtn);
            badgeMenu->setPosition(position);
            m_badgeContainer->addChild(badgeMenu);

            if (!badge.isFromRoulette) {
                auto daysLabel = CCLabelBMFont::create(CCString::createWithFormat(
                    "%d days", 
                    badge.daysRequired)->getCString(),
                    "goldFont.fnt"
                );

                daysLabel->setScale(0.35f);
                daysLabel->setPosition(
                position - CCPoint(0, 30.f)

                ); 

                m_badgeContainer->addChild(daysLabel);
            }
            if (!unlocked) {
                auto lockIcon = CCSprite::createWithSpriteFrameName("GJ_lock_001.png");
                lockIcon->setPosition(position);
                m_badgeContainer->addChild(lockIcon, 5);
            }
        }
        m_pageLeftArrow->setVisible(m_currentPage > 0);
        m_pageRightArrow->setVisible(m_currentPage < m_totalPages - 1);
        int unlockedCount = 0;
        for (auto& badge : categoryBadges) {
            if (g_streakData.isBadgeUnlocked(badge.badgeID)) unlockedCount++;
        }
        m_counterText->setString(
            CCString::createWithFormat(
                "Unlocked: %d/%d",
                unlockedCount, 
                static_cast<int>(
                    categoryBadges.size()))->getCString()
        );
    }

    void onBadgeClick(CCObject* sender) {
        auto badgeID = static_cast<CCString*>(static_cast<CCNode*>(sender)->getUserObject("badge"_spr))->getCString();
        EquipBadgePopup::create(badgeID)->show();
    }
    void onNextBadgePage(CCObject*) {
        if (m_currentPage < m_totalPages - 1) {
            m_currentPage++; 
            updateCategoryDisplay();
        } 
    }

    void onPreviousBadgePage(CCObject*) {
        if (m_currentPage > 0) {
            m_currentPage--; 
            updateCategoryDisplay();
        } 
    }

    void onNextCategory(CCObject*) { 
        m_currentCategory = (m_currentCategory + 1) % 5;
        m_currentPage = 0;
        updateCategoryDisplay();
    }

    void onPreviousCategory(CCObject*) {
        m_currentCategory = (m_currentCategory - 1 + 5) % 5;
        m_currentPage = 0; 
        updateCategoryDisplay(); 
    }

    void onClose(CCObject* sender) override { 
        this->unscheduleUpdate(); 
        Popup::onClose(sender); 
    }

public:
    static RewardsPopup* create() {
        auto ret = new RewardsPopup();
        if (ret && ret->initAnchored(360.f, 250.f)) {
            ret->autorelease();
            return ret; 
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
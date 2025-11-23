#pragma once
#include "../StreakData.h"
#include <Geode/utils/cocos.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

class HistoryCell : public cocos2d::CCLayer {
protected:
    bool init(const std::string& date, int points) {
        if (!cocos2d::CCLayer::init()) return false;
        this->setContentSize({ 280.f, 25.f });
        float cellHeight = this->getContentSize().height;

        auto dateLabel = cocos2d::CCLabelBMFont::create(date.c_str(), "goldFont.fnt");
        dateLabel->setScale(0.5f);
        dateLabel->setAnchorPoint({ 0.0f, 0.5f });
        dateLabel->setPosition({ 10.f, cellHeight / 2 });
        this->addChild(dateLabel);

        auto pointsLabel = cocos2d::CCLabelBMFont::create(std::to_string(points).c_str(), "bigFont.fnt");
        pointsLabel->setScale(0.4f);
        pointsLabel->setAnchorPoint({ 1.0f, 0.5f });
        pointsLabel->setPosition({
            this->getContentSize().width - 10.f, cellHeight / 2 
            }
         );

        this->addChild(pointsLabel);
        auto pointIcon = cocos2d::CCSprite::create("streak_point.png"_spr);
        pointIcon->setScale(0.15f);
        pointIcon->setPosition({
            pointsLabel->getPositionX() - pointsLabel->getScaledContentSize().width - 5.f, cellHeight / 2 
            }
         );

        this->addChild(pointIcon);
        return true;
    }


public:
    static HistoryCell* create(const std::string& date, int points) {
        auto ret = new HistoryCell();
        if (ret && ret->init(date, points)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};


class HistoryPopup : public Popup<> {
protected:
    std::vector<std::pair<std::string, int>> m_historyEntries;
    int m_currentPage = 0;
    int m_totalPages = 0;
    const int m_itemsPerPage = 8;
    CCLayer* m_pageContainer;
    CCMenuItemSpriteExtra* m_leftArrow;
    CCMenuItemSpriteExtra* m_rightArrow;

    bool setup() override {
        this->setTitle("Streak Point History");
        g_streakData.load();
        auto listSize = CCSize{ 280.f, 200.f };
        auto popupCenter = m_mainLayer->getContentSize() / 2;
        auto listBg = cocos2d::extension::CCScale9Sprite::create("square02_001.png");
        listBg->setContentSize(listSize);
        listBg->setColor({ 0, 0, 0 });
        listBg->setOpacity(100);
        listBg->setPosition(popupCenter);
        m_mainLayer->addChild(listBg);
        m_historyEntries = std::vector<std::pair<std::string, int>>(
            g_streakData.streakPointsHistory.begin(),
            g_streakData.streakPointsHistory.end());

        std::sort(m_historyEntries.begin(),
            m_historyEntries.end(),
            [](const auto& a, const auto& b) {
            return a.first < b.first;
            }
        );

        m_totalPages = static_cast<int>(ceil(static_cast<float>(m_historyEntries.size()) / m_itemsPerPage));
        m_currentPage = (m_totalPages > 0) ? (m_totalPages - 1) : 0;
        m_pageContainer = CCLayer::create();
        m_pageContainer->setPosition(popupCenter - listSize / 2);
        m_mainLayer->addChild(m_pageContainer);

        auto leftSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        m_leftArrow = CCMenuItemSpriteExtra::create(
            leftSpr,
            this, 
            menu_selector(
                HistoryPopup::onPrevPage
            ));

        auto rightSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        rightSpr->setFlipX(true);
        m_rightArrow = CCMenuItemSpriteExtra::create(
            rightSpr, 
            this, 
            menu_selector(
                HistoryPopup::onNextPage
            ));

        CCArray* navItems = CCArray::create();
        navItems->addObject(m_leftArrow);
        navItems->addObject(m_rightArrow);
        auto navMenu = CCMenu::createWithArray(navItems);
        navMenu->alignItemsHorizontallyWithPadding(listSize.width + 25.f);
        navMenu->setPosition(popupCenter);
        m_mainLayer->addChild(navMenu);
        this->updatePage();
        return true;
    }

    void updatePage() {
        this->setTitle(
            m_totalPages > 0 ? fmt::format(
                "Point History - Week {}",
                m_currentPage + 1).c_str() : "Point History"
        );
        m_pageContainer->removeAllChildren();
        int startIndex = m_currentPage * m_itemsPerPage;
        for (int i = 0; i < m_itemsPerPage; ++i) {
            int entryIndex = startIndex + i;

            if (entryIndex < m_historyEntries.size()) {
                const auto& [date, points] = m_historyEntries[entryIndex];
                auto cell = HistoryCell::create(date, points);
                cell->setPosition({ 0, 200.f - (i + 1) * 25.f });
                m_pageContainer->addChild(cell);
            }
        }
        m_leftArrow->setVisible(m_currentPage > 0);
        m_rightArrow->setVisible(m_currentPage < m_totalPages - 1);
    }

    void onPrevPage(CCObject*) {
        if (m_currentPage > 0) {
            m_currentPage--;
            this->updatePage();
        }
    }
    void onNextPage(CCObject*) {
        if (m_currentPage < m_totalPages - 1) {
            m_currentPage++;
            this->updatePage();
        }
    }


public:
    static HistoryPopup* create() {
        auto ret = new HistoryPopup();
        if (ret && ret->initAnchored(340.f, 250.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
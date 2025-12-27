#pragma once
#include "../StreakData.h"
#include <Geode/utils/cocos.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>

using namespace geode::prelude;

class HistoryCell : public cocos2d::CCLayer {
protected:
    bool init(const std::string& date, int points, float width) {
        if (!cocos2d::CCLayer::init()) return false;

        float height = 30.f;
        this->setContentSize({ width, height });

        auto dateLabel = cocos2d::CCLabelBMFont::create(date.c_str(), "goldFont.fnt");
        dateLabel->setScale(0.5f);
        dateLabel->setAnchorPoint({ 0.0f, 0.5f });
        dateLabel->setPosition({ 10.f, height / 2 });
        this->addChild(dateLabel);

        auto pointsLabel = cocos2d::CCLabelBMFont::create(std::to_string(points).c_str(), "bigFont.fnt");
        pointsLabel->setScale(0.4f);
        pointsLabel->setAnchorPoint({ 1.0f, 0.5f });
        pointsLabel->setPosition({ width - 10.f, height / 2 });
        this->addChild(pointsLabel);

        auto pointIcon = cocos2d::CCSprite::create("streak_point.png"_spr);
        pointIcon->setScale(0.15f);
        pointIcon->setPosition({
            pointsLabel->getPositionX() - pointsLabel->getScaledContentSize().width - 10.f,
            height / 2
            });
        this->addChild(pointIcon);

        return true;
    }

public:
    static HistoryCell* create(const std::string& date, int points, float width) {
        auto ret = new HistoryCell();
        if (ret && ret->init(date, points, width)) {
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

    bool setup() override {
        this->setTitle("Streak History");
        g_streakData.load();

        // REEMPLAZO DEL FONDO PRINCIPAL DEL POPUP
        if (m_bgSprite) {
            m_bgSprite->removeFromParent();
        }
        m_bgSprite = CCScale9Sprite::create("geode.loader/GE_square01.png");
        m_bgSprite->setContentSize(m_size);
        m_bgSprite->setPosition(CCDirector::get()->getWinSize() / 2);
        this->addChild(m_bgSprite, -1);

        auto listSize = CCSize{ 220.f, 130.f };
        auto popupCenter = m_mainLayer->getContentSize() / 2;

        // FONDO DE LA LISTA (OSCURO SEMI TRANSPARENTE)
        auto listBg = cocos2d::extension::CCScale9Sprite::create("square02_001.png");
        listBg->setContentSize(listSize);
        listBg->setColor({ 0, 0, 0 });
        listBg->setOpacity(100);
        listBg->setPosition(popupCenter);
        m_mainLayer->addChild(listBg);

        m_historyEntries = std::vector<std::pair<std::string, int>>(
            g_streakData.streakPointsHistory.begin(),
            g_streakData.streakPointsHistory.end());

        std::sort(m_historyEntries.begin(), m_historyEntries.end(),
            [](const auto& a, const auto& b) {
                return a.first > b.first;
            }
        );

        auto scroll = ScrollLayer::create(listSize);
        scroll->setPosition(popupCenter - listSize / 2);

        for (const auto& [date, points] : m_historyEntries) {
            auto cell = HistoryCell::create(date, points, listSize.width);
            scroll->m_contentLayer->addChild(cell);
        }

        scroll->m_contentLayer->setLayout(
            ColumnLayout::create()
            ->setGap(2.f)
            ->setAxisAlignment(AxisAlignment::End)
            ->setAxisReverse(true)
        );

        scroll->m_contentLayer->updateLayout();
        scroll->scrollToTop();

        m_mainLayer->addChild(scroll);

        return true;
    }

public:
    static HistoryPopup* create() {
        auto ret = new HistoryPopup();
        if (ret && ret->initAnchored(260.f, 210.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
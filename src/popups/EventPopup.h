#pragma once
#include "StreakCommon.h"
#include "../StreakData.h"
#include <Geode/ui/Popup.hpp>
#include <Geode/utils/web.hpp>
#include "../BadgeNotification.h"
#include "../RewardNotification.h"
#include "SharedVisuals.h" 


class EventPopup : public Popup<> {
protected:
    CCLabelBMFont* m_statusLabel = nullptr;
    CCLayer* m_contentLayer = nullptr;
    CCMenu* m_rewardMenu = nullptr;
    CCLayer* m_barFill = nullptr;
    CCDrawNode* m_indicatorNode = nullptr;
    std::string m_eventID, m_eventName, m_claimingID;
    matjson::Value m_eventRewards, m_playerProgress;
    EventListener<web::WebTask> m_loadListener, m_claimListener;
    bool m_isClaiming = false;
    std::vector<CCLabelBMFont*> m_mythicLabels;
    std::vector<ccColor3B> m_mythicColors;
    int m_colorIndex = 0; 
    float m_colorTransitionTime = 0.0f;
    ccColor3B m_currentColor, m_targetColor;

    bool setup() override {
        this->setTitle("Event");
        auto winSize = m_mainLayer->getContentSize();
        auto listSize = CCSize{ 340.f, 160.f };
        auto listBg = cocos2d::extension::CCScale9Sprite::create("square02_001.png");

        listBg->setContentSize(listSize);
        listBg->setColor({ 0, 0, 0 }); 
        listBg->setOpacity(120); 
        listBg->setPosition(
            winSize / 2 + CCPoint{ 0, 10.f 
            });

        m_mainLayer->addChild(listBg);
        m_contentLayer = CCLayer::create();
        m_contentLayer->setContentSize(listSize);
        m_contentLayer->setPosition(
            listBg->getPosition() - listBg->getContentSize() / 2
        ); 

        m_mainLayer->addChild(m_contentLayer);

        auto streakIcon = CCSprite::create(g_streakData.getRachaSprite().c_str());
        if (!streakIcon) streakIcon = CCSprite::create("racha0.png"_spr);
        streakIcon->setScale(0.18f); 
        streakIcon->setAnchorPoint({ 0.f, 0.5f });
        auto streakLabel = CCLabelBMFont::create(
            std::to_string(g_streakData.currentStreak).c_str(),
            "goldFont.fnt"
        );

        streakLabel->setScale(0.6f);
        streakLabel->setAnchorPoint({ 0.f, 0.5f });
        float padding = 8.f, gap = 2.f;
        streakIcon->setPosition({
            padding,
            padding + (streakIcon->getContentSize().height * streakIcon->getScaleY()) / 2
            }
        );

        m_mainLayer->addChild(streakIcon);
        streakLabel->setPosition({
            streakIcon->getPositionX() + (
                streakIcon->getContentSize().width * streakIcon->getScaleX()) + gap,
                streakIcon->getPositionY() 
            }
        );

        m_mainLayer->addChild(streakLabel);

        m_statusLabel = CCLabelBMFont::create("Loading Event...", "bigFont.fnt");
        m_statusLabel->setPosition(
            winSize / 2 + CCPoint{ 0, 10.f }
        );

        m_statusLabel->setScale(0.7f);
        m_statusLabel->setVisible(false);
        m_mainLayer->addChild(m_statusLabel, 10);

        m_loadListener.bind(this, &EventPopup::onEventResponse);
        m_claimListener.bind(this, &EventPopup::onClaimResponse);
        m_mythicColors = {
            ccc3(255, 0, 0), 
            ccc3(255, 165, 0), 
            ccc3(255, 255, 0), 
            ccc3(0, 255, 0),
            ccc3(0, 0, 255), 
            ccc3(75, 0, 130), 
            ccc3(238, 130, 238)
        };

        m_currentColor = m_mythicColors[0];
        m_targetColor = m_mythicColors[1];
        this->scheduleUpdate();
        this->loadEvent();
        return true;
    }

    void update(float dt) override {
        if (m_mythicLabels.empty()) return;
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

        for (auto* label : m_mythicLabels) label->setColor(interpolatedColor);
    }
    void onClose(CCObject* sender) override {
        this->unscheduleUpdate();
        Popup::onClose(sender);
    }


    void loadEvent() {
        m_statusLabel->setString("Loading Event...");
        m_statusLabel->setVisible(true);
        m_contentLayer->setVisible(false);
        auto req = web::WebRequest();
        m_loadListener.setFilter(
            req.get(
                fmt::format("https://streak-servidor.onrender.com/event/current/{}",
                GJAccountManager::sharedState()->m_accountID))
        );
    }

    void onEventResponse(web::WebTask::Event* e) {
        if (!e->getValue() && !e->isCancelled()) return;
        if (web::WebResponse* res = e->getValue()) {
            if (res->code() == 404) { m_statusLabel->setString("No Active Event");
            return;
            }

            if (res->ok() && res->json().isOk()) {
                auto data = res->json().unwrap();
                m_eventID = data["event"]["eventID"].as<std::string>().unwrapOr("error_id");
                m_eventName = data["event"]["eventName"].as<std::string>().unwrapOr("Event");
                m_eventRewards = data["event"]["rewards"];
                m_playerProgress = data["progress"];
                this->setTitle(m_eventName.c_str());
                m_statusLabel->setVisible(false);
                m_contentLayer->setVisible(true);
                this->buildList();
            }
            else m_statusLabel->setString("Failed to load event");
        }
    }

    void buildList() {
        m_contentLayer->removeAllChildren();
        m_mythicLabels.clear();
        if (!m_eventRewards.isObject()) return;

        std::vector<std::pair<std::string, matjson::Value>> sortedRewards;
        auto rewardsMap = m_eventRewards.as<std::map<std::string, matjson::Value>>().unwrap();
        for (const auto& pair : rewardsMap) sortedRewards.push_back(pair);
        std::sort(sortedRewards.begin(), sortedRewards.end(), [](const auto& a, const auto& b) {
            return a.second["day"].template as<int>().unwrapOr(999) < b.second["day"].template as<int>().unwrapOr(999);
            }
        );

        if (sortedRewards.empty()) {
            m_statusLabel->setString("Event is empty");
            m_statusLabel->setVisible(true);
            return;
        }

        float barWidth = 300.f, barHeight = 10.f;
        float barCenterY = m_contentLayer->getContentSize().height / 2 - 20.f;
        float barStartX = (m_contentLayer->getContentSize().width - barWidth) / 2;

        m_rewardMenu = CCMenu::create();
        m_rewardMenu->setPosition({ 0, 0 });
        m_contentLayer->addChild(m_rewardMenu, 10);
        auto labelNode = CCNode::create();
        m_contentLayer->addChild(labelNode, 15);

        int currentStreak = g_streakData.currentStreak;
        int maxDay = sortedRewards.back().second["day"].as<int>().unwrapOr(1);
        if (maxDay == 0) maxDay = 1;
        float fillPercent = std::min(1.f, static_cast<float>(currentStreak) / maxDay);

        auto outer = CCLayerColor::create(
            { 0, 0, 0, 70 },
            barWidth + 6, barHeight + 6);
        outer->setPosition({ barStartX - 3, barCenterY - (barHeight / 2) - 3 });
        m_contentLayer->addChild(outer, 0);
        auto border = CCLayerColor::create(
            { 255, 255, 255, 120 },
            barWidth + 2, barHeight + 2);
        border->setPosition({
            barStartX - 1,
            barCenterY - (barHeight / 2) - 1 
            }
        );

        m_contentLayer->addChild(border, 4);
        auto barBg = CCLayerColor::create(
            { 45, 45, 45, 255 },
            barWidth, barHeight);
            barBg->setPosition({ barStartX, barCenterY - (barHeight / 2) });
            m_contentLayer->addChild(barBg, 1);

        auto clipper = CCClippingNode::create();
        clipper->setPosition({ 
            barStartX, barCenterY - (barHeight / 2) 
            }
        );

        auto stencil = CCLayerColor::create(
            { 255, 255, 255, 255 }, 
            barWidth,
            barHeight);
            stencil->setPosition({ 0, 0 });
            clipper->setStencil(stencil);
            m_barFill = CCLayerGradient::create(
               { 100, 255, 100, 255 },
               { 0, 200, 0, 255 }
            );

            m_barFill->setContentSize({
                barWidth * fillPercent, barHeight
                }
            ); 

            m_barFill->setPosition({ 0, 0 }); 
            clipper->addChild(m_barFill);
            m_contentLayer->addChild(clipper, 2);

        for (size_t i = 0; i < sortedRewards.size(); ++i) {
            const auto& [claimID, rewardData] = sortedRewards[i];
            int dayRequired = rewardData["day"].as<int>().unwrapOr(0);
            float rewardPercent = (maxDay == 0) ? 0 : static_cast<float>(dayRequired) / maxDay;
            float rewardX = barStartX + (barWidth * rewardPercent);
            float rewardY = barCenterY + 40.f;

            bool isClaimed = m_playerProgress[claimID].as<bool>().unwrapOr(false);
            bool isAvailable = !isClaimed && currentStreak >= dayRequired;

            CCSprite* rewardSprite = nullptr;
            std::string badgeID = "";
            int rewardAmount = 0;

            if (rewardData.contains("badge")) {
                badgeID = rewardData["badge"].as<std::string>().unwrapOr("");
                auto binfo = g_streakData.getBadgeInfo(badgeID);
                rewardSprite = CCSprite::create(
                    binfo ? binfo->spriteName.c_str() : "GJ_secretChest_001.png"
                );
            }
            else if (rewardData.contains("super_stars")) {
                rewardAmount = rewardData["super_stars"].as<int>().unwrapOr(0);
                rewardSprite = CCSprite::create("super_star.png"_spr);
            }

            else if (rewardData.contains("star_tickets")) {
                rewardAmount = rewardData["star_tickets"].as<int>().unwrapOr(0); 
                rewardSprite = CCSprite::create("star_tiket.png"_spr);
            }

            if (!rewardSprite) rewardSprite = CCSprite::createWithSpriteFrameName("GJ_questionMark_001.png");
            rewardSprite->setScale(0.22f);


            if (i == sortedRewards.size() - 1) {
                auto shine = CCSprite::createWithSpriteFrameName("shineBurst_001.png"); 
                shine->setBlendFunc({ GL_SRC_ALPHA, GL_ONE }); 
                shine->setScale(3.0f); 
                shine->runAction(CCRepeatForever::create(CCRotateBy::create(4.0f, 360.f)));
                shine->setPosition(rewardSprite->getContentSize() / 2);
                rewardSprite->addChild(shine, -1);
            }

            auto btn = CCMenuItemSpriteExtra::create(rewardSprite, this, menu_selector(EventPopup::onClaim));
            btn->setUserObject("claim-id"_spr, CCString::create(claimID));
            btn->setID(claimID);
            btn->setPosition({ rewardX, rewardY });
            m_rewardMenu->addChild(btn);

            if (rewardAmount > 0) {
                auto amountLabel = CCLabelBMFont::create(fmt::format("x{}", rewardAmount).c_str(), "goldFont.fnt");
                amountLabel->setScale(0.5f);
                amountLabel->setAnchorPoint({ 0.5f, 1.0f });
                amountLabel->setPosition({ rewardX, rewardY - 14.f });
                labelNode->addChild(amountLabel, 11);
            }
            else if (!badgeID.empty()) {
                if (auto binfo = ::g_streakData.getBadgeInfo(badgeID)) {
                    auto categoryLabel = CCLabelBMFont::create(::g_streakData.getCategoryName(
                        binfo->category).c_str(), "goldFont.fnt"
                    );

                    categoryLabel->setScale(0.45f);
                    categoryLabel->setAnchorPoint({ 0.5f, 1.0f });
                    categoryLabel->setPosition(
                        { rewardX, rewardY - 14.f }
                    );

                    if (binfo->category == ::StreakData::BadgeCategory::MYTHIC) m_mythicLabels.push_back(categoryLabel);

                    else categoryLabel->setColor(::g_streakData.getCategoryColor(binfo->category));
                    labelNode->addChild(categoryLabel, 11);
                }
            }

            auto dayLabel = CCLabelBMFont::create(
                fmt::format("Day {}",
                dayRequired).c_str(),
                "goldFont.fnt"
            );

            dayLabel->setScale(0.45f);
            dayLabel->setPosition({ rewardX, barCenterY - 20.f });
            labelNode->addChild(dayLabel);

            std::string rachaSpriteName = (currentStreak >= dayRequired) ? g_streakData.getRachaSprite(dayRequired) : "racha0.png"_spr;
            auto rachaIcon = CCSprite::create(rachaSpriteName.c_str());
            if (!rachaIcon) rachaIcon = CCSprite::createWithSpriteFrameName("GJ_lock_001.png");
            rachaIcon->setScale(0.14f); 
            rachaIcon->setPosition({ rewardX, barCenterY });
            labelNode->addChild(rachaIcon, 6);

            if (isClaimed) {
                btn->setEnabled(false);
                auto check = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
                check->setScale(1.0f);
                check->setPosition({ rewardX, rewardY });
                labelNode->addChild(check, 10);
            }
            else if (!isAvailable) {
                btn->setEnabled(false); 
                rewardSprite->setColor({ 120, 120, 120 });
            }
            else {
                btn->runAction(CCRepeatForever::create(
                    CCSequence::create(CCScaleTo::create(0.7f, 1.1f), 
                        CCScaleTo::create(0.7f, 1.0f),
                        nullptr))
                );
            }
        }
    }

    void onClaim(CCObject* sender) {
        if (m_isClaiming) return;
        m_isClaiming = true;
        if (m_rewardMenu) m_rewardMenu->setEnabled(false);
        m_statusLabel->setString("Claiming...");
        m_statusLabel->setVisible(true);
        auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
        m_claimingID = static_cast<CCString*>(btn->getUserObject("claim-id"_spr))->getCString();
        matjson::Value payload = matjson::Value::object();


        payload.set(
            "accountID", GJAccountManager::sharedState()->m_accountID
        );
        payload.set(
            "eventID", m_eventID
        ); 
        payload.set(
            "claimID", m_claimingID
        );
        auto req = web::WebRequest();
        m_claimListener.setFilter(req.bodyJSON(payload).post("https://streak-servidor.onrender.com/event/claim"));
    }

    void onClaimResponse(web::WebTask::Event* e) {
        m_isClaiming = false; 
        m_statusLabel->setVisible(false); 
        if (e->isCancelled()) { this->buildList();
        return; 
    }

        if (web::WebResponse* res = e->getValue()) {
            if (res->ok()) {
                auto rewardData = m_eventRewards[m_claimingID];

                int starsWon = 0;
                int ticketsWon = 0;
                std::string badgeID = "";

              
                if (rewardData.contains("super_stars")) starsWon = rewardData["super_stars"].as<int>().unwrapOr(0);
                if (rewardData.contains("star_tickets")) ticketsWon = rewardData["star_tickets"].as<int>().unwrapOr(0);
                if (rewardData.contains("badge")) badgeID = rewardData["badge"].as<std::string>().unwrapOr("");

          
                int starsStart = g_streakData.superStars;
                int ticketsStart = g_streakData.starTickets;

                g_streakData.superStars += starsWon;
                g_streakData.starTickets += ticketsWon;
                bool isNewBadge = false;
                if (!badgeID.empty()) {
                    isNewBadge = !g_streakData.isBadgeUnlocked(badgeID);
                    g_streakData.unlockBadge(badgeID);
                }
                g_streakData.save();

                auto showConsumables = [starsWon, ticketsWon, starsStart, ticketsStart]() {
                    if (starsWon > 0) RewardNotification::show("super_star.png"_spr, starsStart, starsWon);
                    if (ticketsWon > 0) RewardNotification::show("star_tiket.png"_spr, ticketsStart, ticketsWon);
                    };

               
                if (!badgeID.empty() && isNewBadge) {
                    auto* badgeInfo = g_streakData.getBadgeInfo(badgeID);
                    if (badgeInfo && badgeInfo->category == StreakData::BadgeCategory::MYTHIC) {
                        auto animLayer = MythicAnimationLayer::create(*badgeInfo, [badgeID, showConsumables]() {
                            BadgeNotification::show(badgeID);
                            showConsumables();
                            });
                        CCDirector::sharedDirector()->getRunningScene()->addChild(animLayer, 99999);
                    }
                    else {
                        BadgeNotification::show(badgeID);
                        showConsumables();
                    }
                }
                else {
                    showConsumables();
                }

                m_playerProgress.set(m_claimingID, true);
                this->buildList();
            }
            else this->buildList();
        }
        else this->buildList();
    }

public:
    static EventPopup* create() {
        auto ret = new EventPopup();
        if (ret && ret->initAnchored(380.f, 280.f)) { ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};
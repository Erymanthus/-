#pragma once
#include "StreakCommon.h"
#include "../StreakData.h"
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp> 
#include <Geode/utils/web.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/ProfilePage.hpp>
#include <matjson.hpp>
#include "StatusSpinner.h"

using namespace geode::prelude;

class LeaderboardCell : public CCLayer {
protected:
    matjson::Value m_playerData;
    CCLabelBMFont* m_nameLabel = nullptr;
    float m_time = 0.f;

    void updateRainbow(float dt) {
        if (!m_nameLabel) return;
        m_time += dt;
        float r = (sin(m_time * 0.7f) + 1.0f) / 2.0f;
        float g = (sin(m_time * 0.7f + 2.0f * M_PI / 3.0f) + 1.0f) / 2.0f;
        float b = (sin(m_time * 0.7f + 4.0f * M_PI / 3.0f) + 1.0f) / 2.0f;
        m_nameLabel->setColor({
            (GLubyte)(r * 255),
            (GLubyte)(g * 255),
            (GLubyte)(b * 255)
            });
    }

    bool init(matjson::Value playerData, int rank, bool isLocalPlayer, bool showHighlight = true) {
        if (!CCLayer::init()) return false;
        this->m_playerData = playerData;

        const float cellWidth = 300.f;
        const float cellHeight = 40.f;
        this->setContentSize({ cellWidth, cellHeight });

        auto bg = CCLayerColor::create(
            { 0, 0, 0, 80 },
            cellWidth,
            cellHeight - 2
        );
        bg->setPosition({ 0, 1 });
        this->addChild(bg, -2);

        std::string bannerID = playerData["equipped_banner_id"].as<std::string>().unwrapOr("");
        if (isLocalPlayer) {
            bannerID = g_streakData.equippedBanner;
        }

        StreakData::BannerInfo* bannerInfo = nullptr;
        if (!bannerID.empty()) {
            bannerInfo = g_streakData.getBannerInfo(bannerID);
        }

        if (bannerInfo) {
            auto bannerSprite = CCSprite::create(bannerInfo->spriteName.c_str());
            if (bannerSprite) {
                float scaleX = cellWidth / bannerSprite->getContentSize().width;
                float targetHeight = cellHeight - 2.0f;
                float scaleY = targetHeight / bannerSprite->getContentSize().height;

                bannerSprite->setScaleX(scaleX);
                bannerSprite->setScaleY(scaleY);
                bannerSprite->setPosition({ cellWidth / 2, cellHeight / 2 });
                this->addChild(bannerSprite, -1);
            }
        }

        if (rank >= 1 && rank <= 3) {
            std::string spriteName = fmt::format("top{}.png"_spr, rank);
            auto rankSprite = CCSprite::create(spriteName.c_str());
            if (rankSprite) {
                rankSprite->setScale(0.35f);
                rankSprite->setPosition({ 25.f, cellHeight / 2 });
                this->addChild(rankSprite, 1);

                if (rank == 1) {
                    auto glow = CCSprite::createWithSpriteFrameName("shineBurst_001.png");
                    if (glow) {
                        glow->setColor({ 255, 200, 0 });
                        glow->setScale(3.0f);
                        glow->setOpacity(150);
                        glow->setPosition(rankSprite->getContentSize() / 2);
                        glow->setBlendFunc({ GL_SRC_ALPHA, GL_ONE });
                        glow->runAction(CCRepeatForever::create(
                            CCRotateBy::create(4.0f, 360.f)
                        ));
                        rankSprite->addChild(glow, -1);
                    }
                }
            }
            else {
                auto rLabel = CCLabelBMFont::create(
                    fmt::format("#{}", rank).c_str(),
                    "goldFont.fnt"
                );
                rLabel->setPosition({ 25.f, cellHeight / 2 });
                rLabel->setScale(0.7f);
                this->addChild(rLabel, 1);
            }
        }
        else {
            std::string rankTxt = (rank > 0) ? fmt::format("#{}", rank) : "-";
            auto rankLabel = CCLabelBMFont::create(rankTxt.c_str(), "goldFont.fnt");
            rankLabel->setScale(rank > 999 ? 0.5f : 0.7f);
            rankLabel->limitLabelWidth(45.f, rankLabel->getScale(), 0.1f);
            rankLabel->setPosition({ 25.f, cellHeight / 2 });
            this->addChild(rankLabel, 1);
        }

        std::string badgeID = playerData["equipped_badge_id"].as<std::string>().unwrapOr("");
        bool isMythic = false;

        if (!badgeID.empty()) {
            if (auto badgeInfo = g_streakData.getBadgeInfo(badgeID)) {
                auto badgeSprite = CCSprite::create(badgeInfo->spriteName.c_str());
                if (badgeSprite) {
                    badgeSprite->setScale(0.15f);
                    badgeSprite->setPosition({ 60.f, cellHeight / 2 });
                    this->addChild(badgeSprite, 1);
                }
                if (badgeInfo->category == StreakData::BadgeCategory::MYTHIC) {
                    isMythic = true;
                }
            }
        }

        std::string username = playerData["username"].as<std::string>().unwrapOr("-");
        m_nameLabel = CCLabelBMFont::create(username.c_str(), "goldFont.fnt");
        m_nameLabel->limitLabelWidth(105.f, 0.6f, 0.1f);
        m_nameLabel->setColor({ 255, 255, 255 });

        auto usernameBtn = CCMenuItemSpriteExtra::create(
            m_nameLabel,
            this,
            menu_selector(LeaderboardCell::onViewProfile)
        );
        usernameBtn->setAnchorPoint({ 0.f, 0.5f });
        usernameBtn->setPosition({ 85.f, cellHeight / 2 + 7.f });

        auto menu = CCMenu::createWithItem(usernameBtn);
        menu->setPosition({ 0, 0 });
        this->addChild(menu, 1);

        if (isMythic) {
            this->schedule(schedule_selector(LeaderboardCell::updateRainbow));
        }

        int playerLevel = playerData["current_level"].as<int>().unwrapOr(1);
        auto levelNode = CCNode::create();
        levelNode->setAnchorPoint({ 0.f, 0.5f });
        levelNode->setPosition({ 85.f, cellHeight / 2 - 9.f });
        this->addChild(levelNode, 1);

        float cursorX = 0.f;
        auto xpIcon = CCSprite::create("xp.png"_spr);
        if (!xpIcon) {
            xpIcon = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
        }
        if (xpIcon) {
            xpIcon->setScale(0.10f);
            xpIcon->setAnchorPoint({ 0.f, 0.5f });
            xpIcon->setPosition({ cursorX, 0.f });
            levelNode->addChild(xpIcon);
            cursorX += (xpIcon->getContentSize().width * xpIcon->getScaleX()) + 5.f;
        }

        auto levelLabel = CCLabelBMFont::create(
            fmt::format("Lvl {}", playerLevel).c_str(),
            "bigFont.fnt"
        );
        levelLabel->setScale(0.3f);
        levelLabel->setColor({ 0, 255, 255 });
        levelLabel->setAnchorPoint({ 0.f, 0.5f });
        levelLabel->setPosition({ cursorX, 0.f });
        levelNode->addChild(levelLabel);

        int streakDays = playerData["current_streak_days"].as<int>().unwrapOr(0);
        auto streakIcon = CCSprite::create(g_streakData.getRachaSprite(streakDays).c_str());
        if (!streakIcon) {
            streakIcon = CCSprite::create("racha0.png"_spr);
        }
        streakIcon->setScale(0.15f);
        streakIcon->setPosition({ cellWidth - 80.f, cellHeight / 2 });
        this->addChild(streakIcon, 1);

        auto streakLabel = CCLabelBMFont::create(
            std::to_string(streakDays).c_str(),
            "bigFont.fnt"
        );
        streakLabel->setScale(0.35f);
        streakLabel->setAnchorPoint({ 0.f, 0.5f });
        streakLabel->setPosition({
            streakIcon->getPositionX() + 12.f,
            cellHeight / 2
            });
        this->addChild(streakLabel, 1);

        int streakPoints = playerData["total_streak_points"].as<int>().unwrapOr(0);
        auto pointIcon = CCSprite::create("streak_point.png"_spr);
        pointIcon->setScale(0.12f);
        pointIcon->setPosition({ cellWidth - 30.f, cellHeight / 2 + 8.f });
        this->addChild(pointIcon, 1);

        auto pointsLabel = CCLabelBMFont::create(
            std::to_string(streakPoints).c_str(),
            "goldFont.fnt"
        );
        pointsLabel->setScale(0.3f);
        pointsLabel->setAnchorPoint({ 0.5f, 0.5f });
        pointsLabel->setPosition({ cellWidth - 30.f, cellHeight / 2 - 8.f });
        this->addChild(pointsLabel, 1);

        return true;
    }

    void onViewProfile(CCObject*) {
        int accountID = m_playerData["accountID"].as<int>().unwrapOr(0);
        if (accountID != 0) {
            ProfilePage::create(accountID, false)->show();
        }
    }

public:
    static LeaderboardCell* create(matjson::Value playerData, int rank, bool isLocalPlayer, bool showHighlight = true) {
        auto ret = new LeaderboardCell();
        if (ret && ret->init(playerData, rank, isLocalPlayer, showHighlight)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class LeaderboardPopup : public Popup<> {
protected:
    struct Fields {
        EventListener<web::WebTask> m_leaderboardListener;
        StatusSpinner* m_spinner = nullptr;
        ScrollLayer* m_scrollLayer = nullptr;
        CCLabelBMFont* m_myRankLabel = nullptr;
    };

    Fields m_fields;

    void handleLeaderboardResponse(web::WebTask::Event* e) {
        if (web::WebResponse* res = e->getValue()) {
            if (res->ok() && res->json().isOk()) {
                if (this->m_fields.m_spinner) {
                    this->m_fields.m_spinner->hide();
                }

                auto data = res->json().unwrap();
                if (data.as<std::vector<matjson::Value>>().isOk()) {
                    auto playersVec = data.as<std::vector<matjson::Value>>().unwrap();
                    this->populateScroll(playersVec);
                    this->updateMyRankUI(playersVec);
                }
            }
            else {
                if (this->m_fields.m_spinner) {
                    this->m_fields.m_spinner->setError("Failed to load.");
                }
            }
        }
        else if (e->isCancelled()) {
            if (this->m_fields.m_spinner) {
                this->m_fields.m_spinner->setError("Cancelled.");
            }
        }
    }

    void populateScroll(const std::vector<matjson::Value>& players) {
        if (!this->m_fields.m_scrollLayer) return;

        auto contentLayer = this->m_fields.m_scrollLayer->m_contentLayer;
        contentLayer->removeAllChildren();

        int capacityIdx = Mod::get()->getSavedValue<int>("leaderboard_capacity_idx", 0);
        size_t limit = (capacityIdx == 0) ? 10 : 50;

        size_t countToShow = std::min(players.size(), limit);

        float cellHeight = 42.f;
        float totalHeight = std::max(
            this->m_fields.m_scrollLayer->getContentSize().height,
            countToShow * cellHeight
        );

        contentLayer->setContentSize({ 300.f, totalHeight });

        int localAccountID = GJAccountManager::sharedState()->m_accountID;

        for (size_t i = 0; i < countToShow; ++i) {
            int rank = i + 1;
            const auto& playerJson = players[i];

            int playerAccountID = 0;
            if (playerJson["accountID"].isNumber()) {
                playerAccountID = playerJson["accountID"].as<int>().unwrapOr(0);
            }
            else if (playerJson["accountID"].isString()) {
                try {
                    playerAccountID = std::stoi(playerJson["accountID"].as<std::string>().unwrapOr("0"));
                }
                catch (...) {}
            }

            bool isMe = (localAccountID != 0 && playerAccountID == localAccountID);

            auto cell = LeaderboardCell::create(playerJson, rank, isMe);
            float yPos = totalHeight - (i * cellHeight) - (cellHeight / 2);
            cell->setPosition(0, yPos - (cellHeight / 2));

            contentLayer->addChild(cell);
        }

        contentLayer->setPositionY(
            this->m_fields.m_scrollLayer->getContentSize().height - totalHeight
        );
    }

    void updateMyRankUI(const std::vector<matjson::Value>& players) {
        int myRank = -1;
        int localAccountID = GJAccountManager::sharedState()->m_accountID;

        if (localAccountID != 0) {
            for (size_t i = 0; i < players.size(); ++i) {
                int playerID = 0;
                if (players[i]["accountID"].isNumber()) {
                    playerID = players[i]["accountID"].as<int>().unwrapOr(0);
                }
                else if (players[i]["accountID"].isString()) {
                    try {
                        playerID = std::stoi(players[i]["accountID"].as<std::string>().unwrapOr("0"));
                    }
                    catch (...) {}
                }

                if (playerID == localAccountID) {
                    myRank = static_cast<int>(i) + 1;
                    break;
                }
            }
        }

        if (myRank == -1 && g_streakData.globalRank > 0) {
            myRank = g_streakData.globalRank;
        }

        if (this->m_fields.m_myRankLabel) {
            if (myRank != -1) {
                this->m_fields.m_myRankLabel->setString(
                    fmt::format("My Rank: #{}", myRank).c_str()
                );
                this->m_fields.m_myRankLabel->setColor(
                    (myRank <= 10) ? ccColor3B{ 255, 215, 0 } : ccColor3B{ 255, 255, 255 }
                );
            }
            else {
                this->m_fields.m_myRankLabel->setString("My Rank: -");
                this->m_fields.m_myRankLabel->setColor({ 150, 150, 150 });
            }
        }

        if (auto idLabel = typeinfo_cast<CCLabelBMFont*>(m_mainLayer->getChildByID("streak-id-label"))) {
            if (!g_streakData.streakID.empty()) {
                idLabel->setString(
                    fmt::format("ID: {}", g_streakData.streakID).c_str()
                );
            }
        }
    }

    bool setup() override {
        m_mainLayer->setContentSize({ 340.f, 280.f });
        this->setTitle("Top Streaks");
        auto winSize = this->m_mainLayer->getContentSize();

        auto listBg = cocos2d::extension::CCScale9Sprite::create("square02_001.png");
        listBg->setColor({ 0, 0, 0 });
        listBg->setOpacity(120);

        CCSize listSize = { 300.f, 210.f };
        listBg->setContentSize(listSize + CCSize{ 10.f, 10.f });
        listBg->setPosition({ winSize.width / 2, winSize.height / 2 - 5.f });
        this->m_mainLayer->addChild(listBg);

        this->m_fields.m_scrollLayer = ScrollLayer::create(listSize);
        this->m_fields.m_scrollLayer->setPosition({
            (winSize.width - listSize.width) / 2,
            (winSize.height - listSize.height) / 2 - 5.f
            });
        this->m_mainLayer->addChild(this->m_fields.m_scrollLayer);

        this->m_fields.m_myRankLabel = CCLabelBMFont::create(
            "My Rank: ...",
            "goldFont.fnt"
        );
        this->m_fields.m_myRankLabel->setScale(0.5f);
        this->m_fields.m_myRankLabel->setAnchorPoint({ 0.f, 0.5f });
        this->m_fields.m_myRankLabel->setPosition({ 22.f, 18.f });
        this->m_fields.m_myRankLabel->setColor({ 200, 200, 200 });
        this->m_mainLayer->addChild(this->m_fields.m_myRankLabel);

        std::string idText = g_streakData.streakID.empty() ? "Loading ID..." : g_streakData.streakID;

        auto idLabel = CCLabelBMFont::create(
            fmt::format("ID: {}", idText).c_str(),
            "chatFont.fnt"
        );
        idLabel->setScale(0.4f);
        idLabel->setColor({ 150, 150, 150 });
        idLabel->setAnchorPoint({ 1.f, 0.5f });
        idLabel->setPosition({ winSize.width - 22.f, 18.f });
        idLabel->setID("streak-id-label");
        m_mainLayer->addChild(idLabel);

        this->m_fields.m_spinner = StatusSpinner::create();
        this->m_fields.m_spinner->setPosition(winSize / 2);
        this->m_mainLayer->addChild(this->m_fields.m_spinner, 100);
        this->m_fields.m_spinner->setLoading("Loading...");

        this->m_fields.m_leaderboardListener.bind([this](web::WebTask::Event* e) {
            this->handleLeaderboardResponse(e);
            });
        auto req = web::WebRequest();
        this->m_fields.m_leaderboardListener.setFilter(
            req.get("https://streak-servidor.onrender.com/leaderboard")
        );

        return true;
    }

public:
    static LeaderboardPopup* create() {
        auto ret = new LeaderboardPopup();
        if (ret && ret->initAnchored(340.f, 280.f, "geode.loader/GE_square03.png")) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
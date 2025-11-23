#pragma once
#include "StreakCommon.h"
#include "../StreakData.h"
#include <Geode/ui/Popup.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/ProfilePage.hpp>
#include <matjson.hpp>

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

        float cellWidth = 300.f;
        float cellHeight = 40.f;
        this->setContentSize({ cellWidth, cellHeight });

      
        ccColor3B nameColor = isLocalPlayer ? ccColor3B{ 0, 255, 100 } : ccColor3B{ 255, 255, 255 };

 
        if (rank >= 1 && rank <= 3) {
            std::string spriteName = fmt::format("top{}.png"_spr, rank);
            auto rankSprite = CCSprite::create(spriteName.c_str());

            if (rankSprite) {
                rankSprite->setScale(0.35f);
                rankSprite->setPosition({ 25.f, cellHeight / 2 });
                this->addChild(rankSprite);

                if (rank == 1) {
                    auto glowSprite = CCSprite::createWithSpriteFrameName("shineBurst_001.png");
                    if (glowSprite) {
                        glowSprite->setColor({ 255, 200, 0 });
                        glowSprite->setScale(3.0f);
                        glowSprite->setOpacity(150);
                        glowSprite->setPosition(rankSprite->getContentSize() / 2);
                        glowSprite->setBlendFunc({ GL_SRC_ALPHA, GL_ONE });
                        glowSprite->runAction(CCRepeatForever::create(CCRotateBy::create(4.0f, 360.f)));
                        rankSprite->addChild(glowSprite, -1);
                    }
                    auto pulseSequence = CCSequence::create(
                        CCEaseSineInOut::create(CCScaleTo::create(0.8f, 0.38f)),
                        CCEaseSineInOut::create(CCScaleTo::create(0.8f, 0.35f)),
                        nullptr
                    );
                    rankSprite->runAction(CCRepeatForever::create(pulseSequence));
                }
            }
            else {
              
                auto rankLabel = CCLabelBMFont::create(fmt::format("#{}", rank).c_str(), "goldFont.fnt");
                rankLabel->setPosition({ 25.f, cellHeight / 2 });
                rankLabel->setScale(0.7f);
                this->addChild(rankLabel);
            }
        }
        else {
            std::string rankTxt = (rank > 0) ? fmt::format("#{}", rank) : "#?";
            auto rankLabel = CCLabelBMFont::create(rankTxt.c_str(), "goldFont.fnt");
            rankLabel->setScale(rank > 999 ? 0.5f : 0.7f);
            rankLabel->limitLabelWidth(45.f, rankLabel->getScale(), 0.1f);
            rankLabel->setPosition({ 25.f, cellHeight / 2 });
            this->addChild(rankLabel);
        }

       
        std::string badgeID = playerData["equipped_badge_id"].as<std::string>().unwrapOr("");
        bool isMythic = false;

        if (!badgeID.empty()) {
            if (auto badgeInfo = g_streakData.getBadgeInfo(badgeID)) {
                auto badgeSprite = CCSprite::create(badgeInfo->spriteName.c_str());
                if (badgeSprite) {
                    badgeSprite->setScale(0.15f);
                    badgeSprite->setPosition({ 60.f, cellHeight / 2 });
                    this->addChild(badgeSprite);
                }
                if (badgeInfo->category == StreakData::BadgeCategory::MYTHIC) {
                    isMythic = true;
                }
            }
        }

      
        std::string username = playerData["username"].as<std::string>().unwrapOr("-");
        m_nameLabel = CCLabelBMFont::create(username.c_str(), "goldFont.fnt");
        m_nameLabel->limitLabelWidth(120.f, 0.6f, 0.1f);
        m_nameLabel->setColor(nameColor);

        auto usernameBtn = CCMenuItemSpriteExtra::create(
            m_nameLabel, this, menu_selector(LeaderboardCell::onViewProfile)
        );
        usernameBtn->setAnchorPoint({ 0.f, 0.5f });
        usernameBtn->setPosition({ 85.f, cellHeight / 2 });

        auto menu = CCMenu::createWithItem(usernameBtn);
        menu->setPosition({ 0, 0 });
        this->addChild(menu);

        if (isMythic) {
            this->schedule(schedule_selector(LeaderboardCell::updateRainbow));
        }

     
        int streakDays = playerData["current_streak_days"].as<int>().unwrapOr(0);
        std::string spriteName = g_streakData.getRachaSprite(streakDays);

        auto streakIcon = CCSprite::create(spriteName.c_str());
        if (!streakIcon) streakIcon = CCSprite::create("racha0.png"_spr);
        streakIcon->setScale(0.15f);
        streakIcon->setPosition({ cellWidth - 80.f, cellHeight / 2 });
        this->addChild(streakIcon);

        auto streakLabel = CCLabelBMFont::create(std::to_string(streakDays).c_str(), "bigFont.fnt");
        streakLabel->setScale(0.35f);
        streakLabel->setAnchorPoint({ 0.f, 0.5f });
        streakLabel->setPosition({ streakIcon->getPositionX() + 12.f, cellHeight / 2 });
        this->addChild(streakLabel);

      
        int streakPoints = playerData["total_streak_points"].as<int>().unwrapOr(0);
        auto pointIcon = CCSprite::create("streak_point.png"_spr);
        pointIcon->setScale(0.12f);
        pointIcon->setPosition({ cellWidth - 30.f, cellHeight / 2 + 8.f });
        this->addChild(pointIcon);

        auto pointsLabel = CCLabelBMFont::create(std::to_string(streakPoints).c_str(), "goldFont.fnt");
        pointsLabel->setScale(0.3f);
        pointsLabel->setAnchorPoint({ 0.5f, 0.5f });
        pointsLabel->setPosition({ cellWidth - 30.f, cellHeight / 2 - 8.f });
        this->addChild(pointsLabel);

        return true;
    }

    void onViewProfile(CCObject*) {
        int accountID = m_playerData["accountID"].as<int>().unwrapOr(0);
        if (accountID != 0) {
            ProfilePage::create(accountID, false)->show();
        }
    }

public:
    static LeaderboardCell* create(
        matjson::Value playerData,
        int rank, 
        bool isLocalPlayer,
        bool showHighlight = true
    ) 
      {
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
        CCLabelBMFont* m_loadingLabel = nullptr;
        CCNode* m_listContainer = nullptr;
        CCMenuItemSpriteExtra* m_leftArrow = nullptr;
        CCMenuItemSpriteExtra* m_rightArrow = nullptr;
        std::vector<matjson::Value> m_leaderboardData;
        int m_currentPage = 0;
        int m_totalPages = 0;
        const int m_itemsPerPage = 5;
        CCLabelBMFont* m_errorLabel = nullptr;
        CCLabelBMFont* m_myRankLabel = nullptr;
    };

    Fields m_fields;

    void buildList(const matjson::Value& playersValue, CCSize listSize, CCSize winSize) {
        if (this->m_fields.m_errorLabel) {
            this->m_fields.m_errorLabel->removeFromParent();
            this->m_fields.m_errorLabel = nullptr;
        }

        this->m_fields.m_listContainer->removeAllChildrenWithCleanup(true);

        auto layout = ColumnLayout::create()
            ->setAxisAlignment(AxisAlignment::End)
            ->setGap(2.f);
        this->m_fields.m_listContainer->setLayout(layout);

        int startIndex = this->m_fields.m_currentPage * this->m_fields.m_itemsPerPage;
        int endIndex = std::min(
            startIndex + this->m_fields.m_itemsPerPage,
            static_cast<int>(
                this->m_fields.m_leaderboardData.size()
                ));

        int localAccountID = GJAccountManager::sharedState()->m_accountID;

        for (int i = endIndex - 1; i >= startIndex; --i) {
            int rank = i + 1;
            const matjson::Value& playerJson = this->m_fields.m_leaderboardData[i];
            if (playerJson.as<std::map<std::string, matjson::Value>>().isOk()) {
                int playerAccountID = 0;
                if (playerJson["accountID"].isNumber()) {
                    playerAccountID = playerJson["accountID"].as<int>().unwrapOr(0);
                }
                else if (playerJson["accountID"].isString()) {
                    try { playerAccountID = std::stoi(playerJson["accountID"].as<std::string>().unwrapOr("0")); }
                    catch (...) { playerAccountID = 0; }
                }

                bool isMe = (localAccountID != 0 && playerAccountID == localAccountID);
                this->m_fields.m_listContainer->addChild(LeaderboardCell::create(playerJson, rank, isMe));
            }
        }
        this->m_fields.m_listContainer->updateLayout();
        this->updatePageControls();
    }

    void handleLeaderboardResponse(web::WebTask::Event* e, CCSize listSize, CCSize winSize) {
        if (this->m_fields.m_loadingLabel) {
            this->m_fields.m_loadingLabel->setVisible(false);
        }

        if (web::WebResponse* res = e->getValue()) {
            if (res->ok() && res->json().isOk()) {
                auto data = res->json().unwrap();
                if (data.as<std::vector<matjson::Value>>().isOk()) {
                    auto playersVec = data.as<std::vector<matjson::Value>>().unwrap();
                    this->m_fields.m_leaderboardData = playersVec;
                    this->m_fields.m_totalPages = static_cast<int>(ceil(static_cast<float>(playersVec.size()) / this->m_fields.m_itemsPerPage));
                    if (this->m_fields.m_totalPages == 0) this->m_fields.m_totalPages = 1;
                    this->m_fields.m_currentPage = 0;

                  
                    int myRank = -1;
                    int localAccountID = GJAccountManager::sharedState()->m_accountID;

                    if (localAccountID != 0) {
                        for (size_t i = 0; i < playersVec.size(); ++i) {
                            int playerID = 0;            
                            if (playersVec[i]["accountID"].isNumber()) {
                                playerID = playersVec[i]["accountID"].as<int>().unwrapOr(0);
                            }
                            else if (playersVec[i]["accountID"].isString()) {
                                try { playerID = std::stoi(playersVec[i]["accountID"].as<std::string>().unwrapOr("0")); }
                                catch (...) { playerID = 0; }
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
                            this->m_fields.m_myRankLabel->setString(fmt::format("My Rank: #{}", myRank).c_str());
                            this->m_fields.m_myRankLabel->setColor((myRank <= 10) ? ccColor3B{ 255, 215, 0 } : ccColor3B{ 255, 255, 255 });
                        }
                        else {
                            this->m_fields.m_myRankLabel->setString("My Rank: Unknown");
                            this->m_fields.m_myRankLabel->setColor({ 150, 150, 150 });
                        }
                    }

                    if (auto idLabel = typeinfo_cast<CCLabelBMFont*>(m_mainLayer->getChildByID("streak-id-label"))) {
                        if (!g_streakData.streakID.empty()) {
                            idLabel->setString(fmt::format("ID: {}", g_streakData.streakID).c_str());
                        }
                    }
                    this->buildList(data, listSize, winSize);
                }
            }
            else {
                this->showError("Failed to load.");
            }
        }
        else if (e->isCancelled()) {
            this->showError("Request cancelled.");
        }
    }

    void updatePageControls() {
        bool hasData = !this->m_fields.m_leaderboardData.empty();
        bool showError = this->m_fields.m_errorLabel != nullptr;

        if (this->m_fields.m_leftArrow)
            this->m_fields.m_leftArrow->setVisible(hasData && !showError && this->m_fields.m_currentPage > 0);

        if (this->m_fields.m_rightArrow)
            this->m_fields.m_rightArrow->setVisible(hasData && !showError && this->m_fields.m_currentPage < this->m_fields.m_totalPages - 1);

        if (hasData && !showError) {
            this->setTitle(
                fmt::format(
                    "Top Streaks ({}/{})",
                    this->m_fields.m_currentPage + 1,
                    this->m_fields.m_totalPages).c_str()
            );
        }
        else if (!showError) {
            this->setTitle("Top Streaks");
        }
    }

    void onPrevPage(CCObject*) {
        if (this->m_fields.m_currentPage > 0) {
            this->m_fields.m_currentPage--;
            this->buildList(matjson::Value(this->m_fields.m_leaderboardData),
                this->m_fields.m_listContainer->getContentSize(),
                this->m_mainLayer->getContentSize());
        }
    }

    void onNextPage(CCObject*) {
        if (this->m_fields.m_currentPage < this->m_fields.m_totalPages - 1) {
            this->m_fields.m_currentPage++;
            this->buildList(matjson::Value(this->m_fields.m_leaderboardData),
                this->m_fields.m_listContainer->getContentSize(),
                this->m_mainLayer->getContentSize());
        }
    }

    void showError(std::string error) {
        if (this->m_fields.m_loadingLabel) this->m_fields.m_loadingLabel->setVisible(false);

        if (!this->m_fields.m_errorLabel) {
            this->m_fields.m_errorLabel = CCLabelBMFont::create(error.c_str(), "bigFont.fnt");
            this->m_fields.m_errorLabel->setPosition(this->m_mainLayer->getContentSize() / 2);
            this->m_fields.m_errorLabel->setScale(0.4f);
            this->m_fields.m_errorLabel->setColor({ 255, 100, 100 });
            this->m_mainLayer->addChild(this->m_fields.m_errorLabel);
        }
        this->updatePageControls();
    }

    bool setup() override {
        m_mainLayer->setContentSize({ 340.f, 280.f });
        this->setTitle("Top Streaks");
        auto winSize = this->m_mainLayer->getContentSize();

        auto listBg = cocos2d::extension::CCScale9Sprite::create("square02_001.png");
        listBg->setColor({ 0, 0, 0 }); listBg->setOpacity(120);
        CCSize listSize = { 300.f, 210.f };
        listBg->setContentSize(listSize);
        listBg->setPosition({ winSize.width / 2, winSize.height / 2 - 5.f });
        this->m_mainLayer->addChild(listBg);

        this->m_fields.m_listContainer = CCNode::create();
        this->m_fields.m_listContainer->setContentSize(listSize);
        this->m_fields.m_listContainer->setPosition(listBg->getPosition() - listBg->getContentSize() / 2);
        this->m_mainLayer->addChild(this->m_fields.m_listContainer);

        this->m_fields.m_myRankLabel = CCLabelBMFont::create("My Rank: ...", "goldFont.fnt");
        this->m_fields.m_myRankLabel->setScale(0.5f);
        this->m_fields.m_myRankLabel->setAnchorPoint({ 0.f, 0.5f });
        this->m_fields.m_myRankLabel->setPosition({ 22.f, 18.f });
        this->m_fields.m_myRankLabel->setColor({ 200, 200, 200 });
        this->m_mainLayer->addChild(this->m_fields.m_myRankLabel);

        std::string idText = g_streakData.streakID.empty() ? "Loading ID..." : g_streakData.streakID;
        auto idLabel = CCLabelBMFont::create(fmt::format("ID: {}", idText).c_str(), "chatFont.fnt");
        idLabel->setScale(0.4f);
        idLabel->setColor({ 150, 150, 150 });
        idLabel->setAnchorPoint({ 1.f, 0.5f });
        idLabel->setPosition({ winSize.width - 22.f, 18.f });
        idLabel->setID("streak-id-label");
        m_mainLayer->addChild(idLabel);

        auto sprLeft = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        this->m_fields.m_leftArrow = CCMenuItemSpriteExtra::create(
            sprLeft,
            this, 
            menu_selector(LeaderboardPopup::onPrevPage
            ));

        this->m_fields.m_leftArrow->setPosition({ -175.f, -5.f });
        this->m_fields.m_leftArrow->setVisible(false);

        auto sprRight = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        sprRight->setFlipX(true);
        this->m_fields.m_rightArrow = CCMenuItemSpriteExtra::create(
            sprRight,
            this, 
            menu_selector(LeaderboardPopup::onNextPage
            ));
        this->m_fields.m_rightArrow->setPosition({ 175.f, -5.f });
        this->m_fields.m_rightArrow->setVisible(false);

        auto arrowMenu = CCMenu::create();
        arrowMenu->addChild(this->m_fields.m_leftArrow);
        arrowMenu->addChild(this->m_fields.m_rightArrow);
        arrowMenu->setPosition(winSize / 2);
        this->m_mainLayer->addChild(arrowMenu);

        this->m_fields.m_loadingLabel = CCLabelBMFont::create("Loading...", "bigFont.fnt");
        this->m_fields.m_loadingLabel->setPosition(winSize / 2);
        this->m_fields.m_loadingLabel->setScale(0.6f);
        this->m_mainLayer->addChild(this->m_fields.m_loadingLabel, 100);

        this->m_fields.m_leaderboardListener.bind([this, listSize, winSize](web::WebTask::Event* e) {
            this->handleLeaderboardResponse(e, listSize, winSize);
            });
        auto req = web::WebRequest();
        this->m_fields.m_leaderboardListener.setFilter(req.get("https://streak-servidor.onrender.com/leaderboard"));

        return true;
    }

public:
    static LeaderboardPopup* create() {
        auto ret = new LeaderboardPopup();
        if (ret && ret->initAnchored(340.f, 280.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
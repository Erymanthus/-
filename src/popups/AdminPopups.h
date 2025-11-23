#pragma once
#include "StreakCommon.h"
#include "../StreakData.h"
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/ui/Notification.hpp>
#include "../BadgeNotification.h"
#include "../RewardNotification.h"
#include "SharedVisuals.h"

using namespace geode::prelude;


class BadgeSelectorPopup : public Popup<std::function<void(std::string)>> {
protected:
    std::function<void(std::string)> m_onSelect;

    bool setup(std::function<void(std::string)> onSelect) override {
        m_onSelect = onSelect;
        this->setTitle("Select Badge");
        auto winSize = m_mainLayer->getContentSize();
        auto listSize = CCSize{ 320.f, 180.f };

        auto scroll = ScrollLayer::create(listSize);
        scroll->setPosition((winSize - listSize) / 2);

        auto bg = cocos2d::extension::CCScale9Sprite::create("square02_001.png");
        bg->setContentSize(listSize);
        bg->setColor({ 0,0,0 }); bg->setOpacity(80);
        bg->setPosition(winSize / 2);
        m_mainLayer->addChild(bg);
        m_mainLayer->addChild(scroll);

        auto menu = CCMenu::create();
        menu->setPosition({ 0,0 });

        int cols = 4;
        float spacingX = 75.f;
        float spacingY = 75.f;
        int rows = (g_streakData.badges.size() + cols - 1) / cols;
        float totalHeight = std::max(listSize.height, rows * spacingY + 20.f);
        menu->setContentSize({ listSize.width, totalHeight });

        float startX = (listSize.width - ((cols - 1) * spacingX)) / 2;
        float startY = totalHeight - 50.f;

        for (int i = 0; i < g_streakData.badges.size(); ++i) {
            int col = i % cols;
            int row = i / cols;
            auto& badge = g_streakData.badges[i];

            auto spr = CCSprite::create(badge.spriteName.c_str());
            if (!spr) spr = CCSprite::createWithSpriteFrameName("GJ_unknownBtn_001.png");
            spr->setScale(0.4f);

            auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(BadgeSelectorPopup::onBadgeSelected));
            btn->setUserObject("badge"_spr, CCString::create(badge.badgeID));
            btn->setPosition({ startX + col * spacingX, startY - row * spacingY });
            menu->addChild(btn);
        }

        scroll->m_contentLayer->addChild(menu);
        scroll->m_contentLayer->setContentSize(menu->getContentSize());
        scroll->moveToTop();
        return true;
    }

    void onBadgeSelected(CCObject* sender) {
        if (m_onSelect) {
            m_onSelect(static_cast<CCString*>(static_cast<CCNode*>(sender)->getUserObject("badge"_spr))->getCString());
        }
        this->onClose(nullptr);
    }

public:
    static BadgeSelectorPopup* create(std::function<void(std::string)> onSelect) {
        auto ret = new BadgeSelectorPopup();
        if (ret && ret->initAnchored(360.f, 250.f, onSelect)) {
            ret->autorelease(); return ret;
        }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};


class MyCodesPopup : public Popup<> {
protected:
    ScrollLayer* m_listLayer;
    CCLabelBMFont* m_loadingLabel = nullptr;
    EventListener<web::WebTask> m_listener;

    bool setup() override {
        this->setTitle("My Codes");
        auto winSize = m_mainLayer->getContentSize();
        auto listSize = CCSize{ 320.f, 180.f };
        m_listLayer = ScrollLayer::create(listSize);
        m_listLayer->setPosition((winSize - listSize) / 2);

        auto bg = cocos2d::extension::CCScale9Sprite::create("square02_001.png");
        bg->setContentSize(listSize);
        bg->setColor({ 0,0,0 }); bg->setOpacity(80);
        bg->setPosition(winSize / 2);
        m_mainLayer->addChild(bg);
        m_mainLayer->addChild(m_listLayer);

        m_loadingLabel = CCLabelBMFont::create("Loading...", "bigFont.fnt");
        m_loadingLabel->setPosition(winSize / 2);
        m_loadingLabel->setScale(0.5f);
        m_mainLayer->addChild(m_loadingLabel, 99);

        this->loadCodes();
        return true;
    }

    void loadCodes() {
        auto am = GJAccountManager::sharedState();
        matjson::Value payload = matjson::Value::object();
        payload.set("accountID", am->m_accountID);

        m_listener.bind([this](web::WebTask::Event* e) {
            if (web::WebResponse* res = e->getValue()) {
                if (m_loadingLabel) m_loadingLabel->setVisible(false);
                if (res->ok() && res->json().isOk()) {
                    this->populateList(res->json().unwrap());
                }
                else {
                    FLAlertLayer::create("Error", "Failed to fetch codes.", "OK")->show();
                    this->onClose(nullptr);
                }
            }
            else if (e->isCancelled()) {
                if (m_loadingLabel) m_loadingLabel->setVisible(false);
            }
            });

        auto req = web::WebRequest();
        m_listener.setFilter(req.bodyJSON(payload).post("https://streak-servidor.onrender.com/my-codes"));
    }

    void populateList(matjson::Value data) {
        if (!data.isArray()) return;
        auto codes = data.as<std::vector<matjson::Value>>().unwrap();

        if (codes.empty()) {
            auto emptyLabel = CCLabelBMFont::create("No codes created yet.", "bigFont.fnt");
            emptyLabel->setScale(0.5f);
            emptyLabel->setPosition(m_listLayer->getContentSize() / 2);
            m_listLayer->addChild(emptyLabel);
            return;
        }

        auto content = CCNode::create();
        float itemHeight = 35.f;
        float totalHeight = std::max(m_listLayer->getContentSize().height, codes.size() * itemHeight + 10.f);
        content->setContentSize({ m_listLayer->getContentSize().width, totalHeight });

        for (size_t i = 0; i < codes.size(); ++i) {
            std::string name = codes[i]["name"].as<std::string>().unwrapOr("???");
            int used = codes[i]["used"].as<int>().unwrapOr(0);
            int max = codes[i]["max"].as<int>().unwrapOr(0);
            bool active = codes[i]["active"].as<bool>().unwrapOr(true);
            float yPos = totalHeight - (i * itemHeight) - 20.f;

            auto nameLabel = CCLabelBMFont::create(name.c_str(), "goldFont.fnt");
            nameLabel->setScale(0.5f);
            nameLabel->setAnchorPoint({ 0.f, 0.5f });
            nameLabel->setPosition({ 20.f, yPos });
            if (!active) nameLabel->setColor({ 150, 150, 150 });
            content->addChild(nameLabel);

            auto usageLabel = CCLabelBMFont::create(fmt::format("Used: {}/{}", used, max).c_str(), "chatFont.fnt");
            usageLabel->setScale(0.5f);
            usageLabel->setAnchorPoint({ 1.f, 0.5f });
            usageLabel->setPosition({ 300.f, yPos });
            content->addChild(usageLabel);
        }
        m_listLayer->m_contentLayer->addChild(content);
        m_listLayer->m_contentLayer->setContentSize(content->getContentSize());
        m_listLayer->moveToTop();
    }

public:
    static MyCodesPopup* create() {
        auto ret = new MyCodesPopup();
        if (ret && ret->initAnchored(360.f, 220.f)) { ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};


class CreateCodePopup : public Popup<> {
protected:
    TextInput* m_nameInput; TextInput* m_usesInput;
    TextInput* m_starsInput; TextInput* m_ticketsInput;
    CCLabelBMFont* m_badgeLabel = nullptr;
    std::string m_selectedBadgeID = "";
    CCMenuItemSpriteExtra* m_createBtn = nullptr;
    EventListener<web::WebTask> m_createListener;
    int m_pendingStars = 0; int m_pendingTickets = 0;

    bool setup() override {
        this->setTitle("Create Code");
        auto winSize = m_mainLayer->getContentSize();
        bool isAdmin = (g_streakData.userRole >= 2);

        auto myCodesSpr = CCSprite::createWithSpriteFrameName("GJ_menuBtn_001.png");
        myCodesSpr->setScale(0.65f);
        auto myCodesBtn = CCMenuItemSpriteExtra::create(
            myCodesSpr, 
            this,
            menu_selector(CreateCodePopup::onOpenMyCodes)
        );

        auto sideMenu = CCMenu::createWithItem(myCodesBtn);
        sideMenu->setPosition({ 35.f, 35.f });
        m_mainLayer->addChild(sideMenu);

        m_nameInput = TextInput::create(
            150.f, 
            "Code Name", 
            "chatFont.fnt"
        );

        m_nameInput->setPosition({
            winSize.width / 2 - 90.f,
            winSize.height - 65.f 
            });

        m_nameInput->setMaxCharCount(20);
        m_nameInput->setFilter("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-");
        m_mainLayer->addChild(m_nameInput);

        m_usesInput = TextInput::create(80.f, "Uses", "chatFont.fnt");
        m_usesInput->setPosition({ winSize.width / 2 + 90.f, winSize.height - 65.f });
        m_usesInput->setFilter("0123456789");
        m_mainLayer->addChild(m_usesInput);

        float row2Y = winSize.height / 2 + 5.f;

        auto starIcon = CCSprite::create("super_star.png"_spr);
        starIcon->setScale(0.2f);
        starIcon->setPosition({ 85.f, row2Y + 22.f });
        m_mainLayer->addChild(starIcon);
        m_starsInput = TextInput::create(110.f, "Total Stars", "chatFont.fnt");
        m_starsInput->setPosition({ 85.f, row2Y - 8.f });
        m_starsInput->setFilter("0123456789");
        m_mainLayer->addChild(m_starsInput);

        auto ticketIcon = CCSprite::create("star_tiket.png"_spr);
        ticketIcon->setScale(0.25f);
        ticketIcon->setPosition({ 215.f, row2Y + 22.f });
        m_mainLayer->addChild(ticketIcon);
        m_ticketsInput = TextInput::create(110.f, "Total Tickets", "chatFont.fnt");
        m_ticketsInput->setPosition({ 215.f, row2Y - 8.f });
        m_ticketsInput->setFilter("0123456789");
        m_mainLayer->addChild(m_ticketsInput);

        if (isAdmin) {
            auto badgeBtn = CCMenuItemSpriteExtra::create(
                ButtonSprite::create(
                    "Badge",
                    0,
                    0, 
                    "goldFont.fnt",
                    "GJ_button_05.png",
                    0,
                    0.6f
                ),
                this, 
                menu_selector(CreateCodePopup::onSelectBadge)
            );

            auto bMenu = CCMenu::createWithItem(badgeBtn);
            bMenu->setPosition({ 330.f, row2Y + 12.f });
            m_mainLayer->addChild(bMenu);
            m_badgeLabel = CCLabelBMFont::create("None", "chatFont.fnt");
            m_badgeLabel->setScale(0.35f);
            m_badgeLabel->setPosition({ 330.f, row2Y - 20.f });
            m_badgeLabel->setColor({ 200, 200, 200 });
            m_mainLayer->addChild(m_badgeLabel);
        }

        m_createBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create(
                "Create",
                0, 
                0,
                "goldFont.fnt",
                "GJ_button_01.png",
                0,
                1.0f
            ),
            this,
            menu_selector(CreateCodePopup::onCreate)
        );

        auto cMenu = CCMenu::createWithItem(m_createBtn);
        cMenu->setPosition({ winSize.width / 2, 45.f });
        m_mainLayer->addChild(cMenu);

        m_createListener.bind(
            this, 
            &CreateCodePopup::onWebResponse
        );
        return true;
    }

    void onOpenMyCodes(CCObject*) {
        MyCodesPopup::create()->show(); 
    }
    void onSelectBadge(CCObject*) {
        BadgeSelectorPopup::create([this](std::string id) {
            m_selectedBadgeID = id; if (m_badgeLabel) {
                m_badgeLabel->setString(id.c_str());
                m_badgeLabel->setColor({ 0, 255, 100 }); 
            }
         })->show();
    }

    void onCreate(CCObject*) {
        std::string name = m_nameInput->getString();
        std::string usesStr = m_usesInput->getString();
        if (name.empty() || usesStr.empty()) {
            Notification::create("Name and Uses required", 
                NotificationIcon::Error)->show(); 
            return;
        }

        int uses = numFromString<int>(usesStr).unwrapOrDefault();
        if (uses <= 0) {
            Notification::create("Invalid uses",
                NotificationIcon::Error)->show(); 
            return; 
        }

        m_pendingStars = numFromString<int>(m_starsInput->getString()).unwrapOrDefault();
        m_pendingTickets = numFromString<int>(m_ticketsInput->getString()).unwrapOrDefault();

        if (m_pendingStars == 0 && m_pendingTickets == 0 && m_selectedBadgeID.empty()) { 
            Notification::create("Add rewards",
                NotificationIcon::Error)->show();
            return; 
        }

        m_createBtn->setEnabled(false);
        m_createBtn->setNormalImage(
            ButtonSprite::create(
                "Loading...",
                0,
                0, 
                "goldFont.fnt",
                "GJ_button_01.png",
                0,
                1.0f)
        );

        matjson::Value rewards = matjson::Value::object();

        if (m_pendingStars > 0) rewards.set(
            "super_stars",
            m_pendingStars
        );

        if (m_pendingTickets > 0) rewards.set(
            "star_tickets",
            m_pendingTickets
        );

        if (!m_selectedBadgeID.empty()) rewards.set(
            "badge", 
            m_selectedBadgeID
        );

        matjson::Value payload = matjson::Value::object();
        payload.set("modAccountID", GJAccountManager::sharedState()->m_accountID);
        payload.set("codeName", name); payload.set("maxUses", uses);
        payload.set("rewards", rewards);
        auto req = web::WebRequest();
        m_createListener.setFilter(req.bodyJSON(payload).post("https://streak-servidor.onrender.com/create-code"));
    }

    void onWebResponse(web::WebTask::Event* e) {
        if (!e->getValue() && !e->isCancelled()) return;
        if (web::WebResponse* res = e->getValue()) {
            if (res->ok()) {
                if (g_streakData.userRole < 2) { 
                    g_streakData.superStars -= m_pendingStars; 
                    g_streakData.starTickets -= m_pendingTickets;
                    g_streakData.save();
                }

                FLAlertLayer::create("Success", "Code Created Successfully!", "OK")->show();
                this->onClose(nullptr);
            }
            else {
                m_createBtn->setEnabled(true);
                m_createBtn->setNormalImage(ButtonSprite::create("Create", 0, 0, "goldFont.fnt", "GJ_button_01.png", 0, 1.0f));
                std::string err = "Creation Failed";
                if (res->json().isOk()) err = res->json().unwrap()["error"].as<std::string>().unwrapOr(err);
                FLAlertLayer::create("Error", err.c_str(), "OK")->show();
            }
        }
    }

public:
    static CreateCodePopup* create() {
        auto ret = new CreateCodePopup();
        if (ret && ret->initAnchored(420.f, 260.f)) { ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};

class RedeemCodePopup : public Popup<> {
protected:
    TextInput* m_textInput;
    CCMenuItemSpriteExtra* m_redeemBtn;
    EventListener<web::WebTask> m_redeemListener;
    bool m_isRequesting = false;

    bool setup() override {
        this->setTitle("Redeem Code");
        auto winSize = m_mainLayer->getContentSize();

        m_textInput = TextInput::create(280.f, "Enter Code Here");
        m_textInput->setPosition({ winSize.width / 2, winSize.height / 2 + 10.f });
        m_textInput->setFilter("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-");
        m_textInput->setMaxCharCount(24);
        m_mainLayer->addChild(m_textInput);

        m_redeemBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create(
                "Redeem", 
                0,
                0, 
                "goldFont.fnt",
                "GJ_button_01.png",
                0,
                1.0f
            ),
            this, 
            menu_selector(RedeemCodePopup::onOk)
        );

        auto menu = CCMenu::createWithItem(m_redeemBtn);
        menu->setPosition({ winSize.width / 2, winSize.height / 2 - 45.f });
        m_mainLayer->addChild(menu);

        if (g_streakData.userRole >= 1) {
            auto newSpr = ButtonSprite::create("New", 0, 0, "goldFont.fnt", "GJ_button_01.png", 0, 0.6f);
            newSpr->setScale(0.8f);
            auto newBtn = CCMenuItemSpriteExtra::create(
                newSpr,
                this,
                menu_selector(RedeemCodePopup::onOpenCreate)
            );

            auto topMenu = CCMenu::createWithItem(newBtn);
            topMenu->setPosition({ winSize.width - 30.f, winSize.height - 30.f });
            m_mainLayer->addChild(topMenu);
        }
        m_redeemListener.bind(this, &RedeemCodePopup::onWebResponse);
        return true;
    }

    void onOpenCreate(CCObject*) { this->onClose(nullptr); CreateCodePopup::create()->show(); }

    void onOk(CCObject*) {
        if (m_isRequesting) return;
        std::string code = m_textInput->getString();
        if (code.empty()) { 
            FLAlertLayer::create(
                "Error",
                "Please enter a code.",
                "OK"
            )
              ->show();
            return;
        }

        m_isRequesting = true;
        m_redeemBtn->setEnabled(false);
        m_redeemBtn->setNormalImage(ButtonSprite::create("Loading...", 0, 0, "goldFont.fnt", "GJ_button_01.png", 0, 1.0f));

        matjson::Value payload = matjson::Value::object();
        payload.set("code", code);
        payload.set("accountID", GJAccountManager::sharedState()->m_accountID);
        auto req = web::WebRequest();
        m_redeemListener.setFilter(req.bodyJSON(payload).post("https://streak-servidor.onrender.com/redeem-code"));
    }

    void onWebResponse(web::WebTask::Event* e) {
        if (!e->getValue() && !e->isCancelled()) return;
        m_isRequesting = false;

        if (web::WebResponse* res = e->getValue()) {
            if (res->ok() && res->json().isOk()) {
                auto json = res->json().unwrap();

                if (json.contains("rewards")) {
                    auto r = json["rewards"];

                
                    int starsWon = 0;
                    int ticketsWon = 0;
                    std::string badgeID = "";

                    if (r.contains("super_stars")) starsWon = r["super_stars"].as<int>().unwrapOr(0);
                    if (r.contains("star_tickets")) ticketsWon = r["star_tickets"].as<int>().unwrapOr(0);
                    if (r.contains("badge")) badgeID = r["badge"].as<std::string>().unwrapOr("");
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
                   
                        if (starsWon > 0) {
                            RewardNotification::show("super_star.png"_spr, starsStart, starsWon);
                        }
                        if (ticketsWon > 0) {
                            RewardNotification::show("star_tiket.png"_spr, ticketsStart, ticketsWon);
                        }
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
                }

                this->onClose(nullptr);
            }
            else {
                m_redeemBtn->setEnabled(true);
                m_redeemBtn->setNormalImage(
                    ButtonSprite::create(
                        "Redeem",
                        0,
                        0, 
                        "goldFont.fnt",
                        "GJ_button_01.png", 
                        0,
                        1.0f)
                );

                std::string err = "Invalid code.";
                if (res->json().isOk()) err = res->json().unwrap()["error"].as<std::string>().unwrapOr(err);
                FLAlertLayer::create("Error", err.c_str(), "OK")->show();
            }
        }
    }

public:
    static RedeemCodePopup* create() {
        auto ret = new RedeemCodePopup();
        if (ret && ret->initAnchored(340.f, 180.f)) { ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};


class BanUserPopup : public Popup<> {
protected:
    TextInput* m_idInput;
    TextInput* m_reasonInput;
    EventListener<web::WebTask> m_banListener;
    CCMenu* m_buttonMenu = nullptr;

    bool setup() override {
        this->setTitle("Moderation Tool");
        auto winSize = m_mainLayer->getContentSize();

        m_idInput = TextInput::create(200.f, "Target Account ID", "chatFont.fnt");
        m_idInput->setPosition({ winSize.width / 2, winSize.height / 2 + 35.f });
        m_idInput->setFilter("0123456789");
        m_mainLayer->addChild(m_idInput);

        m_reasonInput = TextInput::create(200.f, "Ban Reason (Optional for Unban)", "chatFont.fnt");
        m_reasonInput->setPosition({ winSize.width / 2, winSize.height / 2 - 15.f });
        m_reasonInput->setMaxCharCount(100);
        m_mainLayer->addChild(m_reasonInput);

        m_buttonMenu = CCMenu::create();
        m_buttonMenu->setPosition({ winSize.width / 2, 40.f });
        m_mainLayer->addChild(m_buttonMenu);

        auto banSpr = ButtonSprite::create("BAN", 0, 0, "goldFont.fnt", "GJ_button_06.png", 0, 0.8f);
        auto banBtn = CCMenuItemSpriteExtra::create(banSpr, this, menu_selector(BanUserPopup::onExecuteBan));
        m_buttonMenu->addChild(banBtn);

        auto unbanSpr = ButtonSprite::create("UNBAN", 0, 0, "goldFont.fnt", "GJ_button_01.png", 0, 0.8f);
        auto unbanBtn = CCMenuItemSpriteExtra::create(unbanSpr, this, menu_selector(BanUserPopup::onExecuteUnban));
        m_buttonMenu->addChild(unbanBtn);

        m_buttonMenu->alignItemsHorizontallyWithPadding(20.f);
        m_banListener.bind(this, &BanUserPopup::onWebResponse);
        return true;
    }

    void onExecuteBan(CCObject*) {
        std::string targetIDStr = m_idInput->getString();
        std::string reason = m_reasonInput->getString();
        if (targetIDStr.empty()) {
            Notification::create(
                "Target ID is empty!",
                NotificationIcon::Error)->show();
            return;
        }
        if (reason.empty()) { 
            Notification::create("Ban Reason is required!",
                NotificationIcon::Error)->show();
            return; 
        }

        createQuickPopup(
            "Confirm Ban", 
            "Are you sure you want to <cr>BAN</c> this user?", 
            "Cancel",
            "BAN",
            [this, targetIDStr, reason](FLAlertLayer*, bool btn2) {
                if (btn2) this->sendRequest(
                    "ban",
                    targetIDStr,
                    reason);
            }
        );
    }

    void onExecuteUnban(CCObject*) {
        std::string targetIDStr = m_idInput->getString();
        if (targetIDStr.empty()) { Notification::create("Account ID required!", NotificationIcon::Error)->show(); return; }

        createQuickPopup(
            "Confirm Unban",
            "Are you sure you want to <cg>UNBAN</c> this user?",
            "Cancel",
            "UNBAN",
            [this, targetIDStr](FLAlertLayer*, bool btn2) { if (btn2) this->sendRequest("unban", targetIDStr, ""); }
        );
    }

    void sendRequest(std::string endpoint, std::string targetIDStr, std::string reason) {
        m_buttonMenu->setEnabled(false);
        auto targetID = numFromString<int>(targetIDStr);
        if (!targetID.isOk()) {
            Notification::create(
                "Invalid ID number!",
                NotificationIcon::Error)->show();
            m_buttonMenu->setEnabled(true);
            return;
        }

        auto am = GJAccountManager::sharedState();
        matjson::Value payload = matjson::Value::object();
        payload.set("modAccountID", am->m_accountID);
        payload.set("targetAccountID", targetID.unwrap());
        if (!reason.empty()) payload.set("reason", reason);

        auto req = web::WebRequest();
        std::string url = fmt::format("https://streak-servidor.onrender.com/{}", endpoint);
        m_banListener.setFilter(req.bodyJSON(payload).post(url));
    }

    void onWebResponse(web::WebTask::Event* e) {
        if (!e->getValue() && !e->isCancelled()) return;
        if (m_buttonMenu) m_buttonMenu->setEnabled(true);

        if (web::WebResponse* res = e->getValue()) {
            if (res->ok()) {
                Notification::create("Action completed!", NotificationIcon::Success)->show();
                this->onClose(nullptr);
            }
            else {
                std::string errorMsg = fmt::format("Error: Server returned {}", res->code());
                if (res->json().isOk()) errorMsg = res->json().unwrap()["error"].as<std::string>().unwrapOr(errorMsg);
                Notification::create(errorMsg.c_str(), NotificationIcon::Error)->show();
            }
        }
    }

public:
    static BanUserPopup* create() {
        auto ret = new BanUserPopup();
        if (ret && ret->initAnchored(300.f, 260.f)) { ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};
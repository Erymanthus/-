#include "StreakData.h"
#include "Popups.h"
#include "FirebaseManager.h"

#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/CommentCell.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/GJUserScore.hpp>
#include <Geode/binding/GJComment.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <matjson.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/modify/OptionsLayer.hpp>


/* =================================================================
 * PlayLayer (Ganar puntos con VERIFICACIÓN ANTI-CHEAT)
 * =================================================================*/
class $modify(MyPlayLayer, PlayLayer) {
    void levelComplete() {
        // 1. Capturar porcentaje ANTES de que el juego procese la victoria
        // Esto sirve para saber si ya lo habías completado antes.
        int percentBefore = this->m_level->m_normalPercent;

        // 2. Ejecutar la lógica original del juego
        // Esto es lo que debería guardar el progreso y poner el nivel al 100%.
        PlayLayer::levelComplete();

        // 3. Capturar porcentaje DESPUÉS de la victoria
        int percentAfter = this->m_level->m_normalPercent;

        // --- VERIFICACIONES DE SEGURIDAD ---

        // A) Si es modo práctica, ignorar siempre.
        if (this->m_isPracticeMode) return;

        // B) Anti-Farm: Si ya estaba al 100% antes de jugar, no dar puntos de nuevo.
        if (percentBefore >= 100) return;

        // C) Anti-Hack: Si después de "completar", el porcentaje NO llegó a 100,
        // significa que algo raro pasó (hack de noclip, instant complete falso, etc.).
        if (percentAfter < 100) {
            log::warn("⚠️ Anti-Cheat: levelComplete se ejecutó pero el porcentaje final es solo {}%", percentAfter);
            return;
        }

        // --- SI LLEGAMOS AQUÍ, ES UNA VICTORIA VÁLIDA ---

        int stars = this->m_level->m_stars;
        // Solo niveles con estrellas (rated) dan puntos
        if (stars > 0) {
            int points = 0;
            if (stars <= 3) points = 1;       // Auto/Easy/Normal
            else if (stars <= 5) points = 3;  // Hard
            else if (stars <= 7) points = 4;  // Harder
            else if (stars <= 9) points = 5;  // Insane
            else points = 6;                  // Demon

            if (points > 0) {
                int before = g_streakData.streakPointsToday;
                int required = g_streakData.getRequiredPoints();

                log::info("✅ Nivel completado legalmente ({} stars -> {} points)", stars, points);
                g_streakData.addPoints(points);

                // Mostrar barra de progreso
                if (auto scene = CCDirector::sharedDirector()->getRunningScene()) {
                    scene->addChild(StreakProgressBar::create(points, before, required), 100);
                }
            }
        }
    }
};

class $modify(MyMenuLayer, MenuLayer) {

    
    struct Fields {
        EventListener<web::WebTask> m_playerDataListener;
    };

    bool init() {
        if (!MenuLayer::init()) return false;
        this->createLoadingButton();

       
        this->loadPlayerData();

        return true;
    }

    void createLoadingButton() {
        auto menu = this->getChildByID("bottom-menu");
        if (!menu) return;

        if (auto oldBtn = menu->getChildByID("streak-button"_spr)) {
            oldBtn->removeFromParentAndCleanup(true);
        }

        auto loadingIcon = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
        if (!loadingIcon) return;

        loadingIcon->runAction(CCRepeatForever::create(CCRotateBy::create(1.0f, 360)));
        loadingIcon->setScale(0.4f);

        auto circle = CircleButtonSprite::create(loadingIcon, CircleBaseColor::Gray, CircleBaseSize::Medium);
        if (!circle) return;

        auto btn = CCMenuItemSpriteExtra::create(circle, this, nullptr);
        btn->setEnabled(false);
        btn->setID("streak-button"_spr);
        btn->setPositionY(btn->getPositionY() + 5);
        menu->addChild(btn);
        menu->updateLayout();
    }

  

    void loadPlayerData() {
        auto accountManager = GJAccountManager::sharedState();

        log::info("Resetting local g_streakData before loading...");
        g_streakData.resetToDefault();
      
       
        if (!accountManager || accountManager->m_accountID == 0) {
            log::info("Player not logged in or accountManager is null. Using default data.");
            g_streakData.m_initialized = true;
            g_streakData.isDataLoaded = true;
            
            g_streakData.dailyUpdate();
            this->createFinalButton(); 
            return;
        }

      
        m_fields->m_playerDataListener.bind([this](web::WebTask::Event* e) {
           
            bool loadSuccess = false; 

            if (web::WebResponse* res = e->getValue()) {
              
                if (res->ok() && res->json().isOk()) {
                    log::info("Player data found, parsing response.");
                   
                    g_streakData.parseServerResponse(res->json().unwrap());
                    loadSuccess = true;
                }
              
                else if (res->code() == 404) {
                    log::info("New player detected. Creating profile on server.");
                  
                    g_streakData.dailyUpdate();
                    updatePlayerDataInFirebase(); 
                    loadSuccess = true; 
                }
              
                else {
                    log::error("Failed to load player data. Response code: {}", res->code());
                  
                }

               
                if (loadSuccess) {
                    log::info("Calling dailyUpdate after successful data load/init.");
                   
                    
                    g_streakData.dailyUpdate();
                    
                    g_streakData.m_initialized = true;
                    g_streakData.isDataLoaded = true;
                  
                    this->createFinalButton();
                }
                else {
           
                    this->createErrorButton();
                }
              

            }
            else if (e->isCancelled()) {
             
                log::error("Player data request cancelled or network failed.");
                this->createErrorButton();
               
            }
            });

      
        std::string url = fmt::format("https://streak-servidor.onrender.com/players/{}", accountManager->m_accountID);
        auto req = web::WebRequest();
       
        m_fields->m_playerDataListener.setFilter(req.get(url));
        log::info("Sent request to load player data for account {}", accountManager->m_accountID);
    }

    void createFinalButton() {
      
        auto menu = this->getChildByID("bottom-menu");
        if (!menu) return;

        if (auto oldBtn = menu->getChildByID("streak-button"_spr)) {
            oldBtn->removeFromParentAndCleanup(true);
        }

        std::string spriteName = g_streakData.getRachaSprite();
        CCSprite* icon = CCSprite::create(spriteName.c_str());

        if (!icon) {
            icon = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        }
        if (!icon) return; 

        icon->setScale(0.5f);
        auto circle = CircleButtonSprite::create(icon, CircleBaseColor::Green, CircleBaseSize::Medium);
        if (!circle) return;

        int requiredPoints = g_streakData.getRequiredPoints();
        bool streakInactive = (g_streakData.streakPointsToday < requiredPoints);

        if (streakInactive) {
            auto alertSprite = CCSprite::createWithSpriteFrameName("exMark_001.png");
            if (alertSprite) {
                alertSprite->setScale(0.4f);
                alertSprite->setPosition({ circle->getContentSize().width - 12, circle->getContentSize().height - 12 });
                alertSprite->setZOrder(10);
                alertSprite->runAction(CCRepeatForever::create(CCBlink::create(2.0f, 3)));
                circle->addChild(alertSprite);
            }
        }

        auto newBtn = CCMenuItemSpriteExtra::create(circle, this, menu_selector(MyMenuLayer::onOpenPopup));
        if (!newBtn) return;

        newBtn->setID("streak-button"_spr);
        newBtn->setPositionY(newBtn->getPositionY() + 5);
        newBtn->setEnabled(true);
        menu->addChild(newBtn);
        menu->updateLayout();
    }

    void createErrorButton() {
       
        auto menu = this->getChildByID("bottom-menu");
        if (!menu) return;

        if (auto oldBtn = menu->getChildByID("streak-button"_spr)) {
            oldBtn->removeFromParentAndCleanup(true);
        }

        auto errorIcon = CCSprite::createWithSpriteFrameName("exMark_001.png");
        if (!errorIcon) return;

        errorIcon->setScale(0.5f);
        auto circle = CircleButtonSprite::create(errorIcon, CircleBaseColor::Gray, CircleBaseSize::Medium);
        if (!circle) return;

        auto errorBtn = CCMenuItemSpriteExtra::create(circle, this, nullptr); 
        errorBtn->setEnabled(false); 
        errorBtn->setID("streak-button"_spr);
        errorBtn->setPositionY(errorBtn->getPositionY() + 5);
        menu->addChild(errorBtn);
        menu->updateLayout();
    }

    void onOpenPopup(CCObject * sender) {
        if (g_streakData.isInitialized()) {
            InfoPopup::create()->show();
        }
        else {
            FLAlertLayer::create("Loading", "The streak data is still loading. Please wait.", "OK")->show();
        }
    }
};

class $modify(MyCommentCell, CommentCell) {
    struct Fields {
        CCMenuItemSpriteExtra* badgeButton = nullptr;
        EventListener<web::WebTask> m_badgeListener;
    };

    void onBadgeInfoClick(CCObject * sender) {
        if (auto badgeID = static_cast<CCString*>(static_cast<CCNode*>(sender)->getUserObject())) {
            if (auto badgeInfo = g_streakData.getBadgeInfo(badgeID->getCString())) {
                std::string title = badgeInfo->displayName;
                std::string category = g_streakData.getCategoryName(badgeInfo->category);
                std::string message = fmt::format(
                    "<cy>{}</c>\n\n<cg>Unlocked at {} days</c>",
                    category,
                    badgeInfo->daysRequired
                );
                FLAlertLayer::create(title.c_str(), message, "OK")->show();
            }
        }
    }

    void loadFromComment(GJComment * p0) {
        CommentCell::loadFromComment(p0);
        if (p0->m_accountID == GJAccountManager::get()->get()->m_accountID) {
            if (auto username_menu = m_mainLayer->getChildByIDRecursive("username-menu")) {
                auto equippedBadge = g_streakData.getEquippedBadge();
                if (equippedBadge) {
                    auto badgeSprite = CCSprite::create(equippedBadge->spriteName.c_str());
                    if (badgeSprite) {
                        badgeSprite->setScale(0.15f);
                        auto badgeButton = CCMenuItemSpriteExtra::create(
                            badgeSprite, this, menu_selector(MyCommentCell::onBadgeInfoClick)
                        );
                        badgeButton->setUserObject(CCString::create(equippedBadge->badgeID));
                        badgeButton->setID("streak-badge"_spr);
                        username_menu->addChild(badgeButton);
                        username_menu->updateLayout();
                    }
                }
            }
        }
        else {
            std::string url = fmt::format("https://streak-servidor.onrender.com/players/{}", p0->m_accountID);
            m_fields->m_badgeListener.bind([this, p0](web::WebTask::Event* e) {
                if (web::WebResponse* res = e->getValue()) {
                    if (res->ok() && res->json().isOk()) {
                        try {
                            auto playerData = res->json().unwrap();
                            std::string badgeId = playerData["equipped_badge_id"].as<std::string>().unwrap();
                            if (!badgeId.empty()) {
                                auto badgeInfo = g_streakData.getBadgeInfo(badgeId);
                                if (badgeInfo) {
                                    if (auto username_menu = m_mainLayer->getChildByIDRecursive("username-menu")) {
                                        auto badgeSprite = CCSprite::create(badgeInfo->spriteName.c_str());
                                        badgeSprite->setScale(0.15f);
                                        auto badgeButton = CCMenuItemSpriteExtra::create(
                                            badgeSprite, this, menu_selector(MyCommentCell::onBadgeInfoClick)
                                        );
                                        badgeButton->setUserObject(CCString::create(badgeInfo->badgeID));
                                        username_menu->addChild(badgeButton);
                                        username_menu->updateLayout();
                                    }
                                }
                            }
                        }
                        catch (const std::exception& ex) {
                            log::debug("Player {} (commenter) has no badge: {}", p0->m_accountID, ex.what());
                        }
                    }
                }
                });
            auto req = web::WebRequest();
            m_fields->m_badgeListener.setFilter(req.get(url));
        }
    }
};

class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (geode::Mod::get()->getSettingValue<bool>("show-in-pause")) {
            auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
            double posX = Mod::get()->getSettingValue<double>("pause-pos-x");
            double posY = Mod::get()->getSettingValue<double>("pause-pos-y");
            int pointsToday = g_streakData.streakPointsToday;
            int requiredPoints = g_streakData.getRequiredPoints();
            int streakDays = g_streakData.currentStreak;
            auto streakNode = CCNode::create();
            auto streakIcon = CCSprite::create(g_streakData.getRachaSprite().c_str());
            streakIcon->setScale(0.2f);
            streakNode->addChild(streakIcon);
            auto daysLabel = CCLabelBMFont::create(
                CCString::createWithFormat("Day %d", streakDays)->getCString(), "goldFont.fnt"
            );
            daysLabel->setScale(0.35f);
            daysLabel->setPosition({ 0, -22 });
            streakNode->addChild(daysLabel);
            auto pointCounterNode = CCNode::create();
            pointCounterNode->setPosition({ 0, -37 });
            streakNode->addChild(pointCounterNode);
            auto pointLabel = CCLabelBMFont::create(
                CCString::createWithFormat("%d / %d", pointsToday, requiredPoints)->getCString(), "bigFont.fnt"
            );
            pointLabel->setScale(0.35f);
            pointCounterNode->addChild(pointLabel);
            auto pointIcon = CCSprite::create("streak_point.png"_spr);
            pointIcon->setScale(0.18f);
            pointCounterNode->addChild(pointIcon);
            pointCounterNode->setContentSize({ pointLabel->getScaledContentSize().width + pointIcon->getScaledContentSize().width + 5, pointLabel->getScaledContentSize().height });
            pointLabel->setPosition({ -pointIcon->getScaledContentSize().width / 2, 0 });
            pointIcon->setPosition({ pointLabel->getScaledContentSize().width / 2 + 5, 0 });
            streakNode->setPosition({ winSize.width * static_cast<float>(posX), winSize.height * static_cast<float>(posY) });
            this->addChild(streakNode);
        }
    }
};
#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include "StreakData.h"
#include "FirebaseManager.h" // <-- ESTO LE DICE DONDE ENCONTRAR LA FUNCIÓN

using namespace geode::prelude;

static int g_lastAccountID = -1;

class $modify(AccountWatcher, GameManager) {
    void update(float dt) {
        GameManager::update(dt);

        auto am = GJAccountManager::sharedState();
        if (!am) return;

        int currentID = am->m_accountID;

        // 1. Inicialización
        if (g_lastAccountID == -1) {
            g_lastAccountID = currentID;
            // Si arranca ya logueado, intentamos cargar
            if (currentID != 0 && !g_streakData.isInitialized()) {
                g_streakData.isDataLoaded = false;
                loadPlayerDataFromServer(); // <-- Ahora sí debería encontrarla
            }
            return;
        }

        // 2. Detección de cambio
        if (currentID != g_lastAccountID) {
            log::info("Cambio de cuenta detectado ({} -> {})", g_lastAccountID, currentID);

            if (currentID == 0) {
                log::info("Cierre de sesión. Reseteando datos.");
                g_streakData.resetToDefault();
                g_streakData.isDataLoaded = true;
            }
            else {
                log::info("Inicio de sesión (ID: {}). Cargando datos...", currentID);
                g_streakData.resetToDefault();
                g_streakData.isDataLoaded = false;
                loadPlayerDataFromServer(); // <-- Y aquí también
            }

            g_lastAccountID = currentID;
        }
    }
};
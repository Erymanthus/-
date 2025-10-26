#include "FirebaseManager.h" // Asegúrate de que este declare updatePlayerDataInFirebase()
#include <Geode/utils/web.hpp>
#include <matjson.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include "StreakData.h" // Necesario para g_streakData
#include <Geode/loader/Event.hpp> // Para EventListener
#include <string> // Para std::string y std::to_string
#include <vector> // Para std::vector
#include <map>    // Para std::map
#include <Geode/binding/GameManager.hpp>

// --- Definición Completa de updatePlayerDataInFirebase ---
void updatePlayerDataInFirebase() {
    auto accountManager = GJAccountManager::sharedState();

    // Salir si no está logueado o el manager no es válido
    if (!accountManager || accountManager->m_accountID == 0) {
        // log::warn("Player not logged in or accountManager null. Update canceled."); // Log opcional
        return;
    }

    int accountID = accountManager->m_accountID;
    int userID = GameManager::sharedState()->m_playerUserID;
    matjson::Value playerData = matjson::Value::object();

    // --- Rellenar playerData (Incluir TODOS los campos de g_streakData) ---
    playerData.set("username", std::string(accountManager->m_username)); // Usar std::string() para convertir char*
    playerData.set("accountID", accountID);
    playerData.set("current_streak_days", g_streakData.currentStreak);
    playerData.set("total_streak_points", g_streakData.totalStreakPoints);
    playerData.set("equipped_badge_id", g_streakData.equippedBadge);
    playerData.set("super_stars", g_streakData.superStars);
    playerData.set("star_tickets", g_streakData.starTickets);
    playerData.set("last_roulette_index", g_streakData.lastRouletteIndex);
    playerData.set("total_spins", g_streakData.totalSpins);
    playerData.set("last_day", g_streakData.lastDay);
    playerData.set("streakPointsToday", g_streakData.streakPointsToday);
    playerData.set("userID", userID);

    // Insignias desbloqueadas (Convertir vector<bool> a vector<string>)
    std::vector<std::string> unlocked_badges_vec;
    // Comprobar que los tamaños coincidan antes de acceder
    if (g_streakData.unlockedBadges.size() == g_streakData.badges.size()) {
        for (size_t i = 0; i < g_streakData.badges.size(); ++i) {
            // Comprobar índice por si acaso (aunque la condición anterior debería bastar)
            if (i < g_streakData.unlockedBadges.size() && g_streakData.unlockedBadges[i]) {
                unlocked_badges_vec.push_back(g_streakData.badges[i].badgeID);
            }
        }
    }
    else {
        // Log de advertencia si los tamaños no coinciden (puede indicar un problema)
        log::warn("updatePlayerData: unlockedBadges size ({}) mismatch with badges size ({}). Saving potentially incomplete badge list.",
            g_streakData.unlockedBadges.size(), g_streakData.badges.size());
        // Opcional: Podrías intentar salvar solo las que coincidan hasta el tamaño menor
    }
    playerData.set("unlocked_badges", unlocked_badges_vec);

    // Misiones diarias (Convertir bools a objeto JSON)
    matjson::Value missions_obj = matjson::Value::object();
    missions_obj.set("pm1", g_streakData.pointMission1Claimed);
    missions_obj.set("pm2", g_streakData.pointMission2Claimed);
    missions_obj.set("pm3", g_streakData.pointMission3Claimed);
    missions_obj.set("pm4", g_streakData.pointMission4Claimed);
    missions_obj.set("pm5", g_streakData.pointMission5Claimed);
    missions_obj.set("pm6", g_streakData.pointMission6Claimed);
    playerData.set("missions", missions_obj);

    // Historial de puntos (Convertir map<string, int> a objeto JSON)
    matjson::Value history_obj = matjson::Value::object();
    for (const auto& pair : g_streakData.streakPointsHistory) {
        history_obj.set(pair.first, pair.second); // pair.first es string (fecha), pair.second es int (puntos)
    }
    playerData.set("history", history_obj);

    // Color Mítico (Comprobar insignia equipada)
    bool hasMythicEquipped = false;
    if (auto* equippedBadge = g_streakData.getEquippedBadge()) { // Usar getEquippedBadge() que devuelve BadgeInfo*
        if (equippedBadge->category == StreakData::BadgeCategory::MYTHIC) {
            hasMythicEquipped = true;
        }
    }
    playerData.set("has_mythic_color", hasMythicEquipped);

    // Misiones de nivel completadas (Convertir set<int> a objeto JSON)
    matjson::Value completed_levels_obj = matjson::Value::object();
    for (int levelID : g_streakData.completedLevelMissions) {
        // Convertir el int a string para usarlo como clave JSON
        completed_levels_obj.set(std::to_string(levelID), true); // Formato { "ID_NIVEL_STR": true }
    }
    playerData.set("completedLevelMissions", completed_levels_obj);
    // --- Fin Rellenar playerData ---


    // URL del servidor
    std::string url = fmt::format(
        "https://streak-servidor.onrender.com/players/{}",
        accountID
    );

    // Listener estático para la respuesta (NO capturar 'this')
    static EventListener<web::WebTask> s_updateListener;

    // Reasignar el bind cada vez (reemplaza cualquier bind anterior)
    s_updateListener.bind([](web::WebTask::Event* e) {
        if (web::WebResponse* res = e->getValue()) {
            if (!res->ok()) { // Solo loguear si hay error del servidor
                log::error("Server error on update. Code: {}", res->code());
                // Mostrar cuerpo de la respuesta si existe y es texto
                auto bodyResult = res->string();
                if (bodyResult.isOk()) {
                    log::error("Server response body: {}", bodyResult.unwrap());
                }
                else {
                    log::error("Server response body: (Could not read as string)");
                }
            }
            else {
                // Log opcional para éxito
                // log::info("Update successful (Code: {})", res->code());
            }
        }
        else if (e->isCancelled()) {
            // Log si la petición se cancela (ej. por una nueva llamada a save() o error de red)
            log::warn("Update request to server was cancelled or network failed.");
        }
        // No necesitamos limpiar el filtro manualmente aquí con el listener estático
        // y re-bind. setFilter se encargará de cancelar la tarea anterior.
        });


    // Crear y enviar la petición POST
    auto req = web::WebRequest();
    // bodyJSON convierte playerData a JSON y lo pone en el cuerpo
    // post(url) envía la petición
    // setFilter cancela la tarea anterior de s_updateListener (si existe) y asigna esta nueva
    s_updateListener.setFilter(req.bodyJSON(playerData).post(url));

    // Opcional: Log para confirmar que la petición se envió
    // log::debug("Sent update request to server for account {}", accountID);
}
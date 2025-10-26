#include "StreakData.h"
#include "FirebaseManager.h" // Necesario para updatePlayerDataInFirebase
#include <Geode/utils/cocos.hpp> // Para log::
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>
#include <stdexcept> // Para std::stoi excepciones

// Definición de la variable global (importante)
StreakData g_streakData;

// --- Implementación de Funciones ---

void StreakData::resetToDefault() {
    currentStreak = 0;
    streakPointsToday = 0;
    totalStreakPoints = 0;
    hasNewStreak = false;
    lastDay = "";
    equippedBadge = "";
    superStars = 0;
    starTickets = 0;
    lastRouletteIndex = 0;
    totalSpins = 0;
    streakCompletedLevels.clear(); // ¿Aún necesario?
    streakPointsHistory.clear();
    pointMission1Claimed = false;
    pointMission2Claimed = false;
    pointMission3Claimed = false;
    pointMission4Claimed = false;
    pointMission5Claimed = false;
    pointMission6Claimed = false;
    // Asegurar tamaño correcto y resetear a false
    if (unlockedBadges.size() != badges.size()) {
        unlockedBadges.assign(badges.size(), false);
    }
    else {
        std::fill(unlockedBadges.begin(), unlockedBadges.end(), false);
    }
    completedLevelMissions.clear(); // <-- Resetear misiones de nivel
    isDataLoaded = false;
    m_initialized = false;
}

void StreakData::load() {
    // Vacía a propósito - la carga real es en MenuLayer
}

void StreakData::save() {
    // Llama a la función que envía los datos al servidor.
    updatePlayerDataInFirebase();
}

void StreakData::parseServerResponse(const matjson::Value& data) {
    currentStreak = data["current_streak_days"].as<int>().unwrapOr(0);
    totalStreakPoints = data["total_streak_points"].as<int>().unwrapOr(0);
    equippedBadge = data["equipped_badge_id"].as<std::string>().unwrapOr("");
    superStars = data["super_stars"].as<int>().unwrapOr(0);
    starTickets = data["star_tickets"].as<int>().unwrapOr(0);
    lastRouletteIndex = data["last_roulette_index"].as<int>().unwrapOr(0);
    totalSpins = data["total_spins"].as<int>().unwrapOr(0);
    lastDay = data["last_day"].as<std::string>().unwrapOr("");
    streakPointsToday = data["streakPointsToday"].as<int>().unwrapOr(0);

    // Cargar insignias desbloqueadas
    if (unlockedBadges.size() != badges.size()) { // Asegurar tamaño
        unlockedBadges.assign(badges.size(), false);
    }
    else {
        std::fill(unlockedBadges.begin(), unlockedBadges.end(), false); // Resetear
    }
    if (data.contains("unlocked_badges")) {
        auto badgesResult = data["unlocked_badges"].as<std::vector<matjson::Value>>();
        if (badgesResult.isOk()) {
            for (const auto& badge_id_json : badgesResult.unwrap()) {
                // Llamar a unlockBadge para actualizar el estado local (sin guardar)
                unlockBadge(badge_id_json.as<std::string>().unwrapOr(""));
            }
        }
        else {
            log::warn("Could not parse 'unlocked_badges' as array.");
        }
    }
    else {
        // log::info("No 'unlocked_badges' field found in server response."); // Opcional
    }

    // Cargar estado de misiones diarias
    pointMission1Claimed = false; pointMission2Claimed = false; pointMission3Claimed = false;
    pointMission4Claimed = false; pointMission5Claimed = false; pointMission6Claimed = false;
    if (data.contains("missions")) {
        auto missionsResult = data["missions"].as<std::map<std::string, matjson::Value>>();
        if (missionsResult.isOk()) {
            auto missions = missionsResult.unwrap();
            if (missions.count("pm1")) pointMission1Claimed = missions.at("pm1").as<bool>().unwrapOr(false);
            if (missions.count("pm2")) pointMission2Claimed = missions.at("pm2").as<bool>().unwrapOr(false);
            if (missions.count("pm3")) pointMission3Claimed = missions.at("pm3").as<bool>().unwrapOr(false);
            if (missions.count("pm4")) pointMission4Claimed = missions.at("pm4").as<bool>().unwrapOr(false);
            if (missions.count("pm5")) pointMission5Claimed = missions.at("pm5").as<bool>().unwrapOr(false);
            if (missions.count("pm6")) pointMission6Claimed = missions.at("pm6").as<bool>().unwrapOr(false);
        }
        else {
            log::warn("Could not parse 'missions' as object.");
        }
    }

    // Cargar historial de puntos
    streakPointsHistory.clear();
    if (data.contains("history")) {
        auto historyResult = data["history"].as<std::map<std::string, matjson::Value>>();
        if (historyResult.isOk()) {
            for (const auto& [date, pointsValue] : historyResult.unwrap()) {
                streakPointsHistory[date] = pointsValue.as<int>().unwrapOr(0);
            }
        }
        else {
            log::warn("No se pudo leer 'history' como un objeto desde el servidor.");
        }
    }

    // --- Cargar Misiones de Nivel Completadas ---
    completedLevelMissions.clear(); // Limpiar antes de cargar
    if (data.contains("completedLevelMissions")) {
        auto missionsResult = data["completedLevelMissions"].as<std::map<std::string, matjson::Value>>();
        if (missionsResult.isOk()) {
            for (const auto& [levelIDStr, _] : missionsResult.unwrap()) {
                try {
                    // Convertir la clave (string) de vuelta a int
                    completedLevelMissions.insert(std::stoi(levelIDStr));
                }
                catch (const std::invalid_argument& e) {
                    log::warn("Error al convertir ID '{}' de misión completada: {}", levelIDStr, e.what());
                }
                catch (const std::out_of_range& e) {
                    log::warn("Error al convertir ID '{}' (fuera de rango) de misión completada: {}", levelIDStr, e.what());
                }
            }
            log::info("Loaded {} completed level missions from server.", completedLevelMissions.size());
        }
        else {
            log::warn("Could not parse 'completedLevelMissions' as object from server.");
        }
    }
    else {
        // log::info("No 'completedLevelMissions' field found in server data."); // Opcional
    }
    // --- Fin Carga Misiones Nivel ---

    // Llamar a checkRewards DESPUÉS de cargar todo, incluyendo currentStreak y unlockedBadges
    this->checkRewards();

    isDataLoaded = true;
    m_initialized = true;
}

// --- Implementación de isLevelMissionClaimed ---
bool StreakData::isLevelMissionClaimed(int levelID) const {
    // Comprueba si el ID del nivel existe en el set
    return completedLevelMissions.count(levelID) > 0;
}
// --- Fin Implementación ---

int StreakData::getRequiredPoints() {
    if (currentStreak >= 80) return 10;
    if (currentStreak >= 70) return 9;
    if (currentStreak >= 60) return 8;
    if (currentStreak >= 50) return 7;
    if (currentStreak >= 40) return 6;
    if (currentStreak >= 30) return 5;
    if (currentStreak >= 20) return 4;
    if (currentStreak >= 10) return 3;
    if (currentStreak >= 1)  return 2;
    return 2;
}

int StreakData::getTicketValueForRarity(BadgeCategory category) {
    switch (category) {
    case BadgeCategory::COMMON:   return 5;
    case BadgeCategory::SPECIAL:  return 20;
    case BadgeCategory::EPIC:     return 50;
    case BadgeCategory::LEGENDARY: return 100;
    case BadgeCategory::MYTHIC:   return 500;
    default:                      return 0;
    }
}

// Actualiza solo el estado local
void StreakData::unlockBadge(const std::string& badgeID) {
    if (badgeID.empty()) return;
    // Asegurar tamaño correcto antes de modificar
    if (unlockedBadges.size() != badges.size()) {
        unlockedBadges.assign(badges.size(), false);
    }
    for (size_t i = 0; i < badges.size(); ++i) {
        // Comprobar índice antes de acceder
        if (i < unlockedBadges.size() && badges[i].badgeID == badgeID) {
            unlockedBadges[i] = true; // Marcar como desbloqueado localmente
            return;
        }
    }
    log::warn("Attempted to unlock unknown badge locally: {}", badgeID);
}

std::string StreakData::getCurrentDate() {
    time_t t = time(nullptr);
    tm* now = localtime(&t);
    if (!now) {
        log::error("Failed to get current local time.");
        return ""; // Devolver string vacío en error
    }
    char buf[16];
    // Usar %F (YYYY-MM-DD)
    if (strftime(buf, sizeof(buf), "%F", now) == 0) {
        log::error("Failed to format current date.");
        return ""; // Devolver string vacío en error
    }
    return std::string(buf);
}

void StreakData::unequipBadge() {
    if (!equippedBadge.empty()) { // Solo guardar si cambia
        equippedBadge = "";
        save();
    }
}

bool StreakData::isBadgeEquipped(const std::string& badgeID) {
    // Comprobar si badgeID no está vacío antes de comparar
    return !badgeID.empty() && equippedBadge == badgeID;
}

void StreakData::dailyUpdate() {
    time_t now_t = time(nullptr);
    std::string today = getCurrentDate();
    if (today.empty()) {
        log::error("dailyUpdate: Failed to get current date, aborting daily update.");
        return; // No continuar si no podemos obtener la fecha
    }

    if (lastDay.empty()) {
        log::info("dailyUpdate: First run or data reset. Setting lastDay to today.");
        lastDay = today;
        streakPointsToday = 0;
        // No guardamos aquí, solo inicializamos. Se guardará al añadir puntos.
        return;
    }

    if (lastDay == today) return; // Mismo día, no hacer nada

    // --- Parseo Seguro de lastDay ---
    tm last_tm = {};
    std::stringstream ss(lastDay);
    // Usar >> std::get_time
    ss >> std::get_time(&last_tm, "%Y-%m-%d");

    // Comprobar si el parseo falló
    if (ss.fail() || ss.bad()) {
        log::error("dailyUpdate: Failed to parse lastDay '{}'. Resetting day.", lastDay);
        lastDay = today;
        streakPointsToday = 0;
        pointMission1Claimed = false; pointMission2Claimed = false; pointMission3Claimed = false;
        pointMission4Claimed = false; pointMission5Claimed = false; pointMission6Claimed = false;
        save(); // Guardar el estado reseteado
        return;
    }

    // Intentar obtener time_t
    last_tm.tm_isdst = -1; // Dejar que mktime determine DST
    time_t last_t = mktime(&last_tm);

    // Comprobar si mktime falló
    if (last_t == -1) {
        log::error("dailyUpdate: Failed to convert last_tm ('{}') to time_t. Resetting day.", lastDay);
        lastDay = today;
        streakPointsToday = 0;
        pointMission1Claimed = false; pointMission2Claimed = false; pointMission3Claimed = false;
        pointMission4Claimed = false; pointMission5Claimed = false; pointMission6Claimed = false;
        save(); // Guardar el estado reseteado
        return;
    }
    // --- Fin Parseo Seguro ---

    double seconds_passed = difftime(now_t, last_t);
    const double seconds_in_day = 86400.0; // 60*60*24
    // double days_passed = floor(seconds_passed / seconds_in_day); // Ya no necesitamos floor

    bool streak_should_be_lost = false;
    bool showAlert = false;
    bool needsSave = false; // Bandera para decidir si guardar al final

    // Comprobar si han pasado >= 48 horas (2 días completos)
    if (seconds_passed >= 2.0 * seconds_in_day) {
        log::info("dailyUpdate: >= 48 hours passed. Streak lost.");
        streak_should_be_lost = true;
    }
    // Comprobar si han pasado >= 24 horas pero < 48 horas
    else if (seconds_passed >= 1.0 * seconds_in_day) {
        log::info("dailyUpdate: >= 24 hours passed. Checking previous day's requirement.");
        // Comprobar si se cumplió la racha del día ANTERIOR
        if (streakPointsToday < getRequiredPoints()) {
            log::info("dailyUpdate: Previous day's streak NOT met. Streak lost.");
            streak_should_be_lost = true;
        }
        else {
            log::info("dailyUpdate: Previous day's streak met. Streak continues.");
        }
    }
    else {
        // Menos de 24h, pero la fecha cambió (ej. justo después de medianoche)
        log::info("dailyUpdate: < 24 hours passed, but date changed. Resetting daily counters.");
        // No se pierde la racha aquí, solo se resetean contadores diarios
    }

    // Aplicar pérdida de racha si es necesario
    if (streak_should_be_lost && currentStreak > 0) {
        log::info("dailyUpdate: Resetting streak from {} to 0.", currentStreak);
        currentStreak = 0;
        streakPointsHistory.clear(); // Limpiar historial al perder racha
        // totalStreakPoints = 0; // Opcional: ¿Resetear puntos totales?
        showAlert = true;
        needsSave = true; // Marcar para guardar
    }

    // Siempre resetear contadores diarios si la fecha cambió
    log::info("dailyUpdate: Resetting daily points and missions for new day: {}", today);
    streakPointsToday = 0;
    lastDay = today;
    pointMission1Claimed = false; pointMission2Claimed = false; pointMission3Claimed = false;
    pointMission4Claimed = false; pointMission5Claimed = false; pointMission6Claimed = false;
    needsSave = true; // Marcar que necesitamos guardar el reseteo diario

    // Guardar en el servidor si hubo cambios
    if (needsSave) {
        log::info("dailyUpdate: Saving updated daily state.");
        updatePlayerDataInFirebase(); // Llamar a la función que usa POST /players/:id
    }

    // Mostrar alerta si se perdió la racha (en el hilo principal)
    if (showAlert) {
        Loader::get()->queueInMainThread([=]() {
            if (auto scene = CCDirector::sharedDirector()->getRunningScene()) {
                FLAlertLayer::create("Streak Lost", "You missed a day!", "OK")->show();
            }
            else {
                log::warn("dailyUpdate: Tried to show streak lost alert, but no scene running.");
            }
            });
    }
}

void StreakData::checkRewards() {
    bool changed = false;
    // Asegurarse de que el vector tenga el tamaño correcto antes de iterar/escribir
    if (unlockedBadges.size() != badges.size()) {
        unlockedBadges.assign(badges.size(), false);
    }

    for (size_t i = 0; i < badges.size(); i++) {
        // Comprobar índice (aunque assign debería garantizarlo)
        if (i >= unlockedBadges.size()) continue;

        // Saltar insignias de ruleta y las ya desbloqueadas
        if (badges[i].isFromRoulette || unlockedBadges[i]) continue;

        // Si cumple los días requeridos y no estaba desbloqueada
        if (currentStreak >= badges[i].daysRequired) {
            log::info("checkRewards: Unlocking badge '{}' locally for reaching {} days.", badges[i].badgeID, badges[i].daysRequired);
            unlockedBadges[i] = true;
            changed = true;
        }
    }
    // Guardar SOLO si se desbloqueó alguna insignia nueva
    if (changed) {
        log::info("checkRewards: New streak badges unlocked, saving state.");
        save();
    }
}

void StreakData::addPoints(int count) {
    if (count <= 0) return; // Ignorar puntos no positivos

    dailyUpdate(); // Asegurarse de que el día esté actualizado

    int currentRequired = getRequiredPoints();
    bool alreadyHadStreakToday = (streakPointsToday >= currentRequired);

    streakPointsToday += count;
    totalStreakPoints += count;

    std::string today = getCurrentDate();
    if (!today.empty()) { // Solo actualizar historial si la fecha es válida
        streakPointsHistory[today] = streakPointsToday;
    }
    else {
        log::error("addPoints: Could not get current date to update history.");
    }

    // Comprobar si JUSTO AHORA se alcanzó el requisito
    bool justCompletedStreak = (!alreadyHadStreakToday && streakPointsToday >= currentRequired);

    if (justCompletedStreak) {
        currentStreak++;
        hasNewStreak = true; // Marcar para la animación
        log::info("addPoints: Streak requirement met! New streak: {}. Checking rewards.", currentStreak);
        checkRewards(); // Comprobar recompensas de racha inmediatamente
    }
    else {
        // log::info("addPoints: Added {} points. Today: {}, Required: {}. Streak: {}", count, streakPointsToday, currentRequired, currentStreak); // Log opcional
    }

    save(); // Guardar los puntos añadidos y el posible aumento de racha
}

bool StreakData::shouldShowAnimation() {
    if (hasNewStreak) {
        hasNewStreak = false; // Resetear la bandera después de consultarla
        return true;
    }
    return false;
}

std::string StreakData::getRachaSprite() {
    int streak = currentStreak; // Usar copia local
    if (streak >= 80) return "racha9.png"_spr;
    if (streak >= 70) return "racha8.png"_spr;
    if (streak >= 60) return "racha7.png"_spr;
    if (streak >= 50) return "racha6.png"_spr;
    if (streak >= 40) return "racha5.png"_spr;
    if (streak >= 30) return "racha4.png"_spr;
    if (streak >= 20) return "racha3.png"_spr;
    if (streak >= 10) return "racha2.png"_spr;
    if (streak >= 1)  return "racha1.png"_spr;
    return "racha0.png"_spr;
}

std::string StreakData::getCategoryName(BadgeCategory category) {
    switch (category) {
    case BadgeCategory::COMMON: return "Common";
    case BadgeCategory::SPECIAL: return "Special";
    case BadgeCategory::EPIC: return "Epic";
    case BadgeCategory::LEGENDARY: return "Legendary";
    case BadgeCategory::MYTHIC: return "Mythic";
    default: return "Unknown";
    }
}

ccColor3B StreakData::getCategoryColor(BadgeCategory category) {
    // Usar inicializadores de lista {} para ccc3
    switch (category) {
    case BadgeCategory::COMMON: return { 200, 200, 200 };
    case BadgeCategory::SPECIAL: return { 0, 170, 0 };
    case BadgeCategory::EPIC: return { 170, 0, 255 };
    case BadgeCategory::LEGENDARY: return { 255, 165, 0 };
    case BadgeCategory::MYTHIC: return { 255, 50, 50 };
    default: return { 255, 255, 255 };
    }
}

StreakData::BadgeInfo* StreakData::getBadgeInfo(const std::string& badgeID) {
    if (badgeID.empty()) return nullptr; // Devolver null si el ID está vacío
    for (auto& badge : badges) {
        if (badge.badgeID == badgeID) return &badge;
    }
    return nullptr; // No encontrado
}

bool StreakData::isBadgeUnlocked(const std::string& badgeID) {
    if (badgeID.empty()) return false; // Insignia vacía no puede estar desbloqueada
    // Asegurar tamaño correcto antes de comprobar
    if (unlockedBadges.size() != badges.size()) {
        // log::warn("isBadgeUnlocked: unlockedBadges size mismatch. Assuming false.");
        return false;
    }
    for (size_t i = 0; i < badges.size(); ++i) {
        // Comprobar índice antes de acceder
        if (i < unlockedBadges.size() && badges[i].badgeID == badgeID) {
            return unlockedBadges[i]; // Devolver el valor booleano
        }
    }
    return false; // ID de insignia no encontrado en la lista 'badges'
}

void StreakData::equipBadge(const std::string& badgeID) {
    if (badgeID.empty()) {
        log::warn("Attempted to equip an empty badge ID.");
        return;
    }
    if (isBadgeUnlocked(badgeID)) {
        if (equippedBadge != badgeID) { // Solo guardar si cambia
            equippedBadge = badgeID;
            log::info("Equipped badge: {}", badgeID);
            save();
        }
    }
    else {
        log::warn("Attempted to equip a locked or unknown badge: {}", badgeID);
    }
}

StreakData::BadgeInfo* StreakData::getEquippedBadge() {
    // getBadgeInfo ya maneja el caso de equippedBadge vacío
    return getBadgeInfo(equippedBadge);
}
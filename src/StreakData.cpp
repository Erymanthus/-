#include "StreakData.h"
#include "FirebaseManager.h"
#include <Geode/utils/cocos.hpp> 
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>
#include <Geode/binding/GJAccountManager.hpp> 
#include <algorithm>
#include <cctype>   
#include "SystemNotification.h"
#include "RewardNotification.h"

std::queue<NotificationData> SystemNotification::s_queue;
SystemNotification* SystemNotification::s_activeNotification = nullptr;

StreakData g_streakData;

void StreakData::resetToDefault() {
    currentStreak = 0;
    streakPointsToday = 0;
    totalStreakPoints = 0;
    hasNewStreak = false;
    lastDay = "";
    equippedBadge = "";
    superStars = 0;
    globalRank = 0;
    streakID = "";
    lastStreakAnimated = 0;
    needsRegistration = false;
    isBanned = false;
    banReason = "";
    starTickets = 0;
    lastRouletteIndex = 0;
    totalSpins = 0;
    currentXP = 0;
    currentLevel = 1;
    isTaskEnabled = false;
    taskStatuses.clear();
    streakCompletedLevels.clear();
    streakPointsHistory.clear();
    pointMission1Claimed = false;
    pointMission2Claimed = false;
    pointMission3Claimed = false;
    pointMission4Claimed = false;
    pointMission5Claimed = false;
    pointMission6Claimed = false;

    if (unlockedBadges.size() != badges.size()) {
        unlockedBadges.assign(badges.size(), false);
    }
    else {
        std::fill(unlockedBadges.begin(), unlockedBadges.end(), false);
    }

    equippedBanner = "";
    if (unlockedBanners.size() != banners.size()) {
        unlockedBanners.assign(banners.size(), false);
    }
    else {
        std::fill(unlockedBanners.begin(), unlockedBanners.end(), false);
    }

    completedLevelMissions.clear();
    userRole = 0;
    dailyMsgCount = 0;
    isDataLoaded = false;
    m_initialized = false;
}

void StreakData::load() {}

void StreakData::save() {
    if (!isDataLoaded && !m_initialized) return;
    updatePlayerDataInFirebase();
}

void StreakData::parseServerResponse(const matjson::Value& data) {
    currentStreak = data["current_streak_days"].as<int>().unwrapOr(0);
    lastStreakAnimated = data["last_streak_animated"].as<int>().unwrapOr(0);
    totalStreakPoints = data["total_streak_points"].as<int>().unwrapOr(0);
    equippedBadge = data["equipped_badge_id"].as<std::string>().unwrapOr("");
    equippedBanner = data["equipped_banner_id"].as<std::string>().unwrapOr("");
    superStars = data["super_stars"].as<int>().unwrapOr(0);
    starTickets = data["star_tickets"].as<int>().unwrapOr(0);
    lastRouletteIndex = data["last_roulette_index"].as<int>().unwrapOr(0);
    totalSpins = data["total_spins"].as<int>().unwrapOr(0);
    lastDay = data["last_day"].as<std::string>().unwrapOr("");
    streakPointsToday = data["streakPointsToday"].as<int>().unwrapOr(0);
    streakID = data["streakID"].as<std::string>().unwrapOr("Pending...");
    currentXP = data["current_xp"].as<int>().unwrapOr(0);
    currentLevel = data["current_level"].as<int>().unwrapOr(1);

    hasNewStreak = false;

    if (data.contains("rank")) {
        globalRank = data["rank"].as<int>().unwrapOr(0);
    }
    else if (data.contains("global_rank")) {
        globalRank = data["global_rank"].as<int>().unwrapOr(0);
    }

    if (data.contains("task_enabled")) {
        isTaskEnabled = data["task_enabled"].as<bool>().unwrapOr(false);
    }
    else {
        isTaskEnabled = false;
    }

    taskStatuses.clear();
    if (data.contains("taskStatuses")) {
        auto statusesVal = data["taskStatuses"];
        if (statusesVal.isObject()) {
            auto res = statusesVal.as<std::map<std::string, matjson::Value>>();
            if (res.isOk()) {
                for (auto const& [key, value] : res.unwrap()) {
                    taskStatuses[key] = value.as<std::string>().unwrapOr("");
                }
            }
        }
    }

    userRole = 0;
    if (data.contains("role")) {
        if (data["role"].isString()) {
            std::string roleStr = data["role"].as<std::string>().unwrapOr("");
            std::transform(roleStr.begin(), roleStr.end(), roleStr.begin(), [](unsigned char c) { return std::tolower(c); });
            if (roleStr == "admin" || roleStr == "administrator") userRole = 2;
            else if (roleStr == "moderator" || roleStr == "mod") userRole = 1;
        }
        else if (data["role"].isNumber()) {
            userRole = data["role"].as<int>().unwrapOr(0);
        }
    }

    dailyMsgCount = data["daily_msg_count"].as<int>().unwrapOr(0);
    isBanned = data["ban"].as<bool>().unwrapOr(false);
    banReason = data["ban_reason"].as<std::string>().unwrapOr("No reason provided.");

    if (unlockedBadges.size() != badges.size()) {
        unlockedBadges.assign(badges.size(), false);
    }
    else {
        std::fill(unlockedBadges.begin(), unlockedBadges.end(), false);
    }

    if (data.contains("unlocked_badges")) {
        auto badgesResult = data["unlocked_badges"].as<std::vector<matjson::Value>>();
        if (badgesResult.isOk()) {
            for (const auto& badge_id_json : badgesResult.unwrap()) {
                unlockBadge(badge_id_json.as<std::string>().unwrapOr(""));
            }
        }
    }

    if (unlockedBanners.size() != banners.size()) {
        unlockedBanners.assign(banners.size(), false);
    }
    else {
        std::fill(unlockedBanners.begin(), unlockedBanners.end(), false);
    }

    if (data.contains("unlocked_banners")) {
        auto bannersResult = data["unlocked_banners"].as<std::vector<matjson::Value>>();
        if (bannersResult.isOk()) {
            for (const auto& banner_id_json : bannersResult.unwrap()) {
                unlockBanner(banner_id_json.as<std::string>().unwrapOr(""));
            }
        }
    }

    pointMission1Claimed = false;
    pointMission2Claimed = false;
    pointMission3Claimed = false;
    pointMission4Claimed = false;
    pointMission5Claimed = false;
    pointMission6Claimed = false;

    if (data.contains("missions")) {
        auto missionsResult = data["missions"].as<std::map<std::string, matjson::Value>>();
        if (missionsResult.isOk()) {
            auto m = missionsResult.unwrap();
            if (m.count("pm1")) pointMission1Claimed = m.at("pm1").as<bool>().unwrapOr(false);
            if (m.count("pm2")) pointMission2Claimed = m.at("pm2").as<bool>().unwrapOr(false);
            if (m.count("pm3")) pointMission3Claimed = m.at("pm3").as<bool>().unwrapOr(false);
            if (m.count("pm4")) pointMission4Claimed = m.at("pm4").as<bool>().unwrapOr(false);
            if (m.count("pm5")) pointMission5Claimed = m.at("pm5").as<bool>().unwrapOr(false);
            if (m.count("pm6")) pointMission6Claimed = m.at("pm6").as<bool>().unwrapOr(false);
        }
    }

    streakPointsHistory.clear();
    if (data.contains("history")) {
        auto h = data["history"].as<std::map<std::string, matjson::Value>>();
        if (h.isOk()) {
            for (const auto& [date, val] : h.unwrap()) streakPointsHistory[date] = val.as<int>().unwrapOr(0);
        }
    }

    completedLevelMissions.clear();
    if (data.contains("completedLevelMissions")) {
        auto m = data["completedLevelMissions"].as<std::map<std::string, matjson::Value>>();
        if (m.isOk()) {
            for (const auto& [idStr, _] : m.unwrap()) {
                if (auto id = numFromString<int>(idStr)) completedLevelMissions.insert(id.unwrap());
            }
        }
    }

    this->checkRewards();
    isDataLoaded = true;
    m_initialized = true;
}

bool StreakData::isLevelMissionClaimed(int levelID) const {
    return completedLevelMissions.count(levelID) > 0;
}

int StreakData::getRequiredPoints() {
    if (currentStreak >= 100) return 12;
    if (currentStreak >= 90)  return 11;
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
    case BadgeCategory::COMMON: return 5;
    case BadgeCategory::SPECIAL: return 20;
    case BadgeCategory::EPIC: return 50;
    case BadgeCategory::LEGENDARY: return 100;
    case BadgeCategory::MYTHIC: return 500;
    default: return 0;
    }
}

void StreakData::unlockBadge(const std::string& badgeID) {
    if (badgeID.empty()) return;
    if (unlockedBadges.size() != badges.size()) unlockedBadges.assign(badges.size(), false);
    for (size_t i = 0; i < badges.size(); ++i) {
        if (i < unlockedBadges.size() && badges[i].badgeID == badgeID) {
            unlockedBadges[i] = true;
            return;
        }
    }
}

std::string StreakData::getCurrentDate() {
    time_t t = time(nullptr);
    tm* now = localtime(&t);
    if (!now) return "";
    char buf[16];
    if (strftime(buf, sizeof(buf), "%F", now) == 0) return "";
    return std::string(buf);
}

void StreakData::unequipBadge() {
    if (!equippedBadge.empty()) {
        equippedBadge = "";
        save();
    }
}

bool StreakData::isBadgeEquipped(const std::string& badgeID) {
    return !badgeID.empty() && equippedBadge == badgeID;
}

void StreakData::dailyUpdate() {
    if (!isDataLoaded) return;

    time_t now_t = time(nullptr);
    std::string today = getCurrentDate();
    if (today.empty()) return;

    if (lastDay.empty()) {
        lastDay = today;
        streakPointsToday = 0;
        dailyMsgCount = 0;
        pointMission1Claimed = false;
        pointMission2Claimed = false;
        pointMission3Claimed = false;
        pointMission4Claimed = false;
        pointMission5Claimed = false;
        pointMission6Claimed = false;
        save();
        return;
    }

    if (lastDay == today) return;

    streakPointsToday = 0;
    dailyMsgCount = 0;
    lastDay = today;
    hasNewStreak = false;
    pointMission1Claimed = false;
    pointMission2Claimed = false;
    pointMission3Claimed = false;
    pointMission4Claimed = false;
    pointMission5Claimed = false;
    pointMission6Claimed = false;

    save();
}

void StreakData::checkRewards() {
    bool changed = false;
    if (unlockedBadges.size() != badges.size()) unlockedBadges.assign(badges.size(), false);

    for (size_t i = 0; i < badges.size(); i++) {
        if (i >= unlockedBadges.size()) continue;
        if (badges[i].isFromRoulette || unlockedBadges[i]) continue;

        if (currentStreak >= badges[i].daysRequired) {
            unlockedBadges[i] = true;
            changed = true;
        }
    }
    if (changed) save();
}

void StreakData::addPoints(int count) {
    if (!isDataLoaded) return;
    if (count <= 0) return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPointTime).count();
    if (elapsed < 1000) {
        return;
    }
    lastPointTime = now;

    dailyUpdate();

    int currentRequired = getRequiredPoints();
    bool alreadyReachedGoalToday = (streakPointsToday >= currentRequired);

  
    streakPointsToday += count;
    totalStreakPoints += count;

    std::string today = getCurrentDate();
    if (!today.empty()) streakPointsHistory[today] = streakPointsToday;

    bool stuckAtZero = (currentStreak == 0 && streakPointsToday >= currentRequired);

    if ((!alreadyReachedGoalToday || stuckAtZero) && streakPointsToday >= currentRequired && !hasNewStreak) {
        currentStreak++;
        hasNewStreak = true;

 
        int starsToSend = 1;
        if (count >= 6) starsToSend = 10;
        else if (count >= 5) starsToSend = 9;
        else if (count >= 4) starsToSend = 7;
        else if (count >= 3) starsToSend = 5;
        else if (count >= 2) starsToSend = 4;
        else starsToSend = 1;

        completeLevelInFirebase(starsToSend);

    }
    else {
      
        int starsToSend = 1;
        if (count >= 6) starsToSend = 10;
        else if (count >= 5) starsToSend = 9;
        else if (count >= 4) starsToSend = 7;
        else if (count >= 3) starsToSend = 5;
        else if (count >= 2) starsToSend = 4;
        else starsToSend = 1;

        completeLevelInFirebase(starsToSend);

       
    }
}

bool StreakData::shouldShowAnimation() {
    if (currentStreak > 0 && currentStreak > lastStreakAnimated) return true;
    return false;
}

std::string StreakData::getRachaSprite(int streak) {
    if (streak >= 100) return "racha11.png"_spr;
    if (streak >= 90) return "racha10.png"_spr;
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

std::string StreakData::getRachaSprite() {
    return getRachaSprite(this->currentStreak);
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
    if (badgeID.empty()) return nullptr;
    for (auto& badge : badges) {
        if (badge.badgeID == badgeID) return &badge;
    }
    return nullptr;
}

bool StreakData::isBadgeUnlocked(const std::string& badgeID) {
    if (badgeID.empty()) return false;
    if (unlockedBadges.size() != badges.size()) return false;
    for (size_t i = 0; i < badges.size(); ++i) {
        if (i < unlockedBadges.size() && badges[i].badgeID == badgeID) return unlockedBadges[i];
    }
    return false;
}

void StreakData::equipBadge(const std::string& badgeID) {
    if (badgeID.empty()) return;
    if (isBadgeUnlocked(badgeID)) {
        if (equippedBadge != badgeID) {
            equippedBadge = badgeID;
            save();
        }
    }
}

StreakData::BadgeInfo* StreakData::getEquippedBadge() {
    return getBadgeInfo(equippedBadge);
}

int StreakData::getXPForCurrentStreak() {
    int bonus = (currentStreak / 10) * 15;
    return 25 + bonus;
}

int StreakData::getXPRequiredForNextLevel() {
    return currentLevel * 100;
}

float StreakData::getXPPercentage() {
    int req = getXPRequiredForNextLevel();
    if (req == 0) return 0.0f;
    return std::clamp((float)currentXP / (float)req, 0.0f, 1.0f);
}

void StreakData::addXP(int amount) {
    if (amount <= 0) return;

    int preLevel = currentLevel;

    currentXP += amount;
    while (currentXP >= getXPRequiredForNextLevel()) {
        currentXP -= getXPRequiredForNextLevel();
        currentLevel++;
    }

    int levelsGained = currentLevel - preLevel;

    if (levelsGained > 0) {
        int totalStarsGained = 0;
        int totalTicketsGained = 0;

        for (int i = 1; i <= levelsGained; i++) {
            auto rewards = getRewardsForLevel(preLevel + i);
            totalStarsGained += rewards.stars;
            totalTicketsGained += rewards.tickets;
        }

        this->superStars += totalStarsGained;
        this->starTickets += totalTicketsGained;
        this->save();

        SystemNotification::show(
            "LEVEL UP!",
            fmt::format("Welcome to Level {}", currentLevel),
            "xp.png"_spr,
            0.3f
        );
    }
    else {
        this->save();
    }
}

StreakData::LevelRewards StreakData::getRewardsForLevel(int level) {
    int r_tickets = 0;
    int r_stars = 0;

    if (level < 10) r_tickets = 40;
    else if (level < 20) r_tickets = 48;
    else {
        int tier = (level / 10) - 1;
        r_tickets = 48 * std::pow(2, tier);
    }

    if (level < 10) r_stars = 5;
    else if (level < 20) r_stars = 20;
    else {
        int tier = (level / 10) - 1;
        r_stars = 20 + (tier * 20);
    }
    return { r_stars, r_tickets };
}

void StreakData::unlockBanner(const std::string& bannerID) {
    if (bannerID.empty()) return;
    if (unlockedBanners.size() != banners.size()) unlockedBanners.assign(banners.size(), false);
    for (size_t i = 0; i < banners.size(); ++i) {
        if (i < unlockedBanners.size() && banners[i].bannerID == bannerID) {
            unlockedBanners[i] = true;
            return;
        }
    }
}

bool StreakData::isBannerUnlocked(const std::string& bannerID) {
    if (bannerID.empty()) return false;
    if (unlockedBanners.size() != banners.size()) return false;
    for (size_t i = 0; i < banners.size(); ++i) {
        if (i < unlockedBanners.size() && banners[i].bannerID == bannerID) return unlockedBanners[i];
    }
    return false;
}

StreakData::BannerInfo* StreakData::getBannerInfo(const std::string& bannerID) {
    if (bannerID.empty()) return nullptr;
    for (auto& banner : banners) {
        if (banner.bannerID == bannerID) return &banner;
    }
    return nullptr;
}

void StreakData::equipBanner(const std::string& bannerID) {
    if (bannerID.empty()) return;
    if (isBannerUnlocked(bannerID)) {
        if (equippedBanner != bannerID) {
            equippedBanner = bannerID;
            save();
        }
    }
}

void StreakData::unequipBanner() {
    if (!equippedBanner.empty()) {
        equippedBanner = "";
        save();
    }
}

StreakData::BannerInfo* StreakData::getEquippedBanner() {
    return getBannerInfo(equippedBanner);
}
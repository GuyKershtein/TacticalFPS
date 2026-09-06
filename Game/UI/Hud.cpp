#include "Hud.h"

#include "../../Engine/UI/Font.h"
#include "../../Engine/UI/TextRenderer.h"
#include "../../Engine/UI/UIRenderer.h"
#include "../Player/PlayerHealth.h"
#include "../Economy/Wallet.h"
#include "../Player/WeaponInventory.h"
#include "../Weapons/Weapon.h"
#include "../Rounds/RoundManager.h"
#include "../Grenades/GrenadeTypes.h"

#include <cstdio>

namespace Game {

namespace {
constexpr glm::vec3 kWhite(1.0f, 1.0f, 1.0f);
constexpr glm::vec3 kGreen(0.35f, 0.9f, 0.35f);
constexpr glm::vec3 kYellow(0.95f, 0.85f, 0.2f);
constexpr glm::vec3 kRed(0.9f, 0.25f, 0.25f);
constexpr glm::vec3 kPanelBg(0.05f, 0.05f, 0.05f);
constexpr glm::vec3 kCyan(0.3f, 0.85f, 0.9f);

glm::vec3 HealthColor(float fraction) {
    if (fraction > 0.6f) return kGreen;
    if (fraction > 0.3f) return kYellow;
    return kRed;
}
} // namespace

std::string Hud::FormatTime(float seconds) {
    if (seconds < 0.0f) seconds = 0.0f;
    const int totalSeconds = static_cast<int>(seconds + 0.999f); // round up, matches how buy timers "feel" (never shows 0 while phase is still active)
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%d:%02d", totalSeconds / 60, totalSeconds % 60);
    return buffer;
}

const char* Hud::PhaseLabel(RoundPhase phase) {
    switch (phase) {
        case RoundPhase::Buy: return "BUY PHASE";
        case RoundPhase::Active: return "ACTIVE";
        case RoundPhase::End: return "ROUND OVER";
    }
    return "";
}

void Hud::Render(Engine::TextRenderer& text, Engine::UIRenderer& ui, const Engine::Font& font, const HudDrawInfo& info) {
    const float w = static_cast<float>(info.screenWidth);
    const float h = static_cast<float>(info.screenHeight);

    // --- Crosshair: a small gap-cross, not a solid dot, so it doesn't
    // obscure exactly the point being aimed at. ---
    const float cx = w * 0.5f;
    const float cy = h * 0.5f;
    constexpr float kGap = 4.0f, kLen = 8.0f, kThick = 2.0f;
    ui.DrawRect(cx - kGap - kLen, cy - kThick * 0.5f, kLen, kThick, kWhite, 0.85f);
    ui.DrawRect(cx + kGap, cy - kThick * 0.5f, kLen, kThick, kWhite, 0.85f);
    ui.DrawRect(cx - kThick * 0.5f, cy - kGap - kLen, kThick, kLen, kWhite, 0.85f);
    ui.DrawRect(cx - kThick * 0.5f, cy + kGap, kThick, kLen, kWhite, 0.85f);

    // --- Bottom-left: health + armor bars ---
    constexpr float kBarWidth = 180.0f, kBarHeight = 18.0f, kMargin = 20.0f;
    const float healthY = h - kMargin - kBarHeight * 2.0f - 6.0f;
    const float armorY = h - kMargin - kBarHeight;

    if (info.health) {
        const float healthFraction = info.health->GetHealth() / PlayerHealth::kMaxHealth;
        ui.DrawRect(kMargin, healthY, kBarWidth, kBarHeight, kPanelBg, 0.6f);
        ui.DrawRect(kMargin, healthY, kBarWidth * glm::clamp(healthFraction, 0.0f, 1.0f), kBarHeight, HealthColor(healthFraction), 0.9f);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "HP %.0f", info.health->GetHealth());
        text.Draw(font, buf, kMargin + 6.0f, healthY - 1.0f, kWhite);

        const float armorFraction = info.health->GetArmor() / PlayerHealth::kMaxArmor;
        ui.DrawRect(kMargin, armorY, kBarWidth, kBarHeight, kPanelBg, 0.6f);
        ui.DrawRect(kMargin, armorY, kBarWidth * glm::clamp(armorFraction, 0.0f, 1.0f), kBarHeight, kCyan, 0.9f);
        std::snprintf(buf, sizeof(buf), "AR %.0f", info.health->GetArmor());
        text.Draw(font, buf, kMargin + 6.0f, armorY - 1.0f, kWhite);
    }

    // --- Bottom-right: weapon name + ammo ---
    if (info.inventory) {
        if (const Weapon* weapon = info.inventory->GetCurrent()) {
            char ammoBuf[48];
            if (weapon->GetData().isMelee) {
                std::snprintf(ammoBuf, sizeof(ammoBuf), "%s", weapon->GetData().name.c_str());
            } else if (weapon->IsReloading()) {
                std::snprintf(ammoBuf, sizeof(ammoBuf), "%s - RELOADING", weapon->GetData().name.c_str());
            } else {
                std::snprintf(ammoBuf, sizeof(ammoBuf), "%s  %d / %d",
                    weapon->GetData().name.c_str(), weapon->GetMagazineAmmo(), weapon->GetReserveAmmo());
            }
            const float textWidth = font.MeasureWidth(ammoBuf);
            text.Draw(font, ammoBuf, w - kMargin - textWidth, h - kMargin - font.GetPixelHeight(), kWhite);
        }
    }

    // --- Top-left: money ---
    if (info.wallet) {
        char moneyBuf[32];
        std::snprintf(moneyBuf, sizeof(moneyBuf), "$%d", info.wallet->GetBalance());
        text.Draw(font, moneyBuf, kMargin, kMargin, kGreen);
    }

    // --- Top-center: round phase, timer, team score, objective status ---
    if (info.roundManager) {
        const RoundPhase phase = info.roundManager->GetPhase();
        char phaseBuf[64];
        if (info.roundManager->GetCharge().GetState() == ChargeState::Planted) {
            std::snprintf(phaseBuf, sizeof(phaseBuf), "CHARGE ARMED  %s", FormatTime(info.roundManager->GetPhaseTimeRemaining()).c_str());
        } else {
            std::snprintf(phaseBuf, sizeof(phaseBuf), "%s  %s", PhaseLabel(phase), FormatTime(info.roundManager->GetPhaseTimeRemaining()).c_str());
        }
        const float phaseWidth = font.MeasureWidth(phaseBuf);
        const glm::vec3 phaseColor = (info.roundManager->GetCharge().GetState() == ChargeState::Planted) ? kRed : kWhite;
        text.Draw(font, phaseBuf, cx - phaseWidth * 0.5f, kMargin, phaseColor);

        char scoreBuf[32];
        std::snprintf(scoreBuf, sizeof(scoreBuf), "ASSAULT %d - %d GUARDIAN",
            info.roundManager->GetAssaultScore(), info.roundManager->GetGuardianScore());
        const float scoreWidth = font.MeasureWidth(scoreBuf);
        text.Draw(font, scoreBuf, cx - scoreWidth * 0.5f, kMargin + font.GetPixelHeight() + 4.0f, kWhite, 0.85f);
    }

    // --- Top-right: bots alive (debug-useful until there's a real
    // scoreboard), FPS ---
    char botsBuf[32];
    std::snprintf(botsBuf, sizeof(botsBuf), "BOTS %d/%d", info.botsAlive, info.botsTotal);
    float botsWidth = font.MeasureWidth(botsBuf);
    text.Draw(font, botsBuf, w - kMargin - botsWidth, kMargin, kWhite, 0.7f);

    char fpsBuf[16];
    std::snprintf(fpsBuf, sizeof(fpsBuf), "%.0f FPS", info.fps);
    const float fpsWidth = font.MeasureWidth(fpsBuf);
    text.Draw(font, fpsBuf, w - kMargin - fpsWidth, kMargin + font.GetPixelHeight() + 4.0f, kWhite, 0.5f);

    // --- Bottom-center: grenade counts ---
    if (info.grenadeCounts) {
        static const char* kLetters[4] = {"F", "S", "L", "D"};
        constexpr float kSlotSize = 28.0f;
        const float totalWidth = kSlotSize * 4.0f;
        float slotX = cx - totalWidth * 0.5f;
        const float slotY = h - kMargin - kSlotSize;
        for (int i = 0; i < 4; ++i) {
            const bool has = info.grenadeCounts[i] > 0;
            ui.DrawRect(slotX, slotY, kSlotSize - 4.0f, kSlotSize, kPanelBg, has ? 0.7f : 0.3f);
            char label[8];
            std::snprintf(label, sizeof(label), "%s%d", kLetters[i], info.grenadeCounts[i]);
            text.Draw(font, label, slotX + 4.0f, slotY + 4.0f, has ? kWhite : glm::vec3(0.5f), has ? 1.0f : 0.5f);
            slotX += kSlotSize;
        }
    }
}

} // namespace Game

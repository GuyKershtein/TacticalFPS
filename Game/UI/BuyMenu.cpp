#include "BuyMenu.h"

#include "../../Engine/UI/Font.h"
#include "../../Engine/UI/TextRenderer.h"
#include "../../Engine/UI/UIRenderer.h"
#include "../../Engine/Input/InputManager.h"
#include "../Economy/Wallet.h"
#include "../Player/WeaponInventory.h"
#include "../Player/PlayerHealth.h"
#include "../Weapons/Weapon.h"
#include "../Weapons/WeaponPresets.h"
#include "../Grenades/GrenadeTypes.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdio>
#include <memory>
#include <vector>

namespace Game {

namespace {

constexpr int kAmmoResupplyCost = 200;
constexpr int kArmorCost = 650;

// Fixed weapon-inventory slot indices (see GameApplication::Init): 0 is the
// always-owned Sidearm, 3 the always-owned Combat Knife; 1 and 2 start
// empty until bought here.
constexpr size_t kCarbineSlot = 1;
constexpr size_t kShotgunSlot = 2;

enum class RowAction { BuyCarbine, BuyShotgun, BuyArmor, BuyAmmo, BuyFrag, BuySmoke, BuyFlash, BuyDecoy };

struct Row {
    int hotkey;
    std::string label;
    int cost;
    bool disabled; // already owned / not applicable this moment
    RowAction action;
};

std::vector<Row> BuildRows(const WeaponInventory& inventory, const PlayerHealth& health, const int grenadeCounts[4]) {
    std::vector<Row> rows;
    rows.push_back({GLFW_KEY_1, "Carbine (Rifle)", CreateCarbineData().purchaseCost, inventory.HasWeaponAt(kCarbineSlot), RowAction::BuyCarbine});
    rows.push_back({GLFW_KEY_2, "Street Sweeper (Shotgun)", CreateStreetSweeperData().purchaseCost, inventory.HasWeaponAt(kShotgunSlot), RowAction::BuyShotgun});
    rows.push_back({GLFW_KEY_3, "Full Armor", kArmorCost, health.GetArmor() >= PlayerHealth::kMaxArmor, RowAction::BuyArmor});
    rows.push_back({GLFW_KEY_4, "Ammo Resupply", kAmmoResupplyCost, inventory.GetCurrent() == nullptr, RowAction::BuyAmmo});
    rows.push_back({GLFW_KEY_5, GetGrenadeData(GrenadeKind::Fragmentation).name, GetGrenadeData(GrenadeKind::Fragmentation).purchaseCost, grenadeCounts[0] > 0, RowAction::BuyFrag});
    rows.push_back({GLFW_KEY_6, GetGrenadeData(GrenadeKind::Smoke).name, GetGrenadeData(GrenadeKind::Smoke).purchaseCost, grenadeCounts[1] > 0, RowAction::BuySmoke});
    rows.push_back({GLFW_KEY_7, GetGrenadeData(GrenadeKind::Flashbang).name, GetGrenadeData(GrenadeKind::Flashbang).purchaseCost, grenadeCounts[2] > 0, RowAction::BuyFlash});
    rows.push_back({GLFW_KEY_8, GetGrenadeData(GrenadeKind::Decoy).name, GetGrenadeData(GrenadeKind::Decoy).purchaseCost, grenadeCounts[3] > 0, RowAction::BuyDecoy});
    return rows;
}

constexpr float kPanelWidth = 420.0f;
constexpr float kRowHeight = 34.0f;
constexpr float kRowSpacing = 4.0f;
constexpr float kTitleHeight = 40.0f;

} // namespace

void BuyMenu::Update(float deltaTime, Engine::InputManager& input, Wallet& wallet, WeaponInventory& inventory,
    PlayerHealth& health, int grenadeCounts[4]) {
    if (m_messageTimer > 0.0f) {
        m_messageTimer -= deltaTime;
    }
    if (!m_open) return;

    const std::vector<Row> rows = BuildRows(inventory, health, grenadeCounts);

    for (size_t i = 0; i < rows.size(); ++i) {
        const Row& row = rows[i];
        if (row.disabled || !input.WasKeyPressed(row.hotkey)) continue;

        const int cost = row.cost;
        bool success = false;
        const std::string& itemName = row.label;

        if (!wallet.TrySpend(cost)) {
            m_lastMessage = "Need $" + std::to_string(cost) + " for " + itemName;
            m_lastMessageWasError = true;
            m_messageTimer = 2.5f;
            continue;
        }

        switch (row.action) {
            case RowAction::BuyCarbine: {
                auto weapon = std::make_unique<Weapon>();
                weapon->Init(CreateCarbineData());
                inventory.SetWeaponAt(kCarbineSlot, std::move(weapon));
                inventory.SwitchTo(kCarbineSlot);
                success = true;
                break;
            }
            case RowAction::BuyShotgun: {
                auto weapon = std::make_unique<ShotgunWeapon>();
                weapon->Init(CreateStreetSweeperData());
                inventory.SetWeaponAt(kShotgunSlot, std::move(weapon));
                inventory.SwitchTo(kShotgunSlot);
                success = true;
                break;
            }
            case RowAction::BuyArmor:
                health.SetArmor(PlayerHealth::kMaxArmor);
                success = true;
                break;
            case RowAction::BuyAmmo:
                if (Weapon* current = inventory.GetCurrent()) {
                    current->RefillReserveAmmo();
                    success = true;
                }
                break;
            case RowAction::BuyFrag: grenadeCounts[0] = 1; success = true; break;
            case RowAction::BuySmoke: grenadeCounts[1] = 1; success = true; break;
            case RowAction::BuyFlash: grenadeCounts[2] = 1; success = true; break;
            case RowAction::BuyDecoy: grenadeCounts[3] = 1; success = true; break;
        }

        if (success) {
            m_lastMessage = "Purchased " + itemName;
            m_lastMessageWasError = false;
            m_messageTimer = 2.0f;
            std::printf("[Buy] Purchased %s for $%d. Balance: $%d\n", itemName.c_str(), cost, wallet.GetBalance());
        } else {
            // Bought nothing usable (e.g. ammo with no weapon equipped) —
            // refund rather than silently eating the player's money.
            wallet.Add(cost);
        }
        std::fflush(stdout);
    }
}

void BuyMenu::Render(Engine::TextRenderer& text, Engine::UIRenderer& ui, const Engine::Font& font,
    int screenWidth, int screenHeight, const Wallet& wallet, const WeaponInventory& inventory,
    const PlayerHealth& health, const int grenadeCounts[4]) const {
    if (!m_open) return;

    const std::vector<Row> rows = BuildRows(inventory, health, grenadeCounts);
    const float panelHeight = kTitleHeight + (kRowHeight + kRowSpacing) * static_cast<float>(rows.size()) + 40.0f;
    const float panelX = (static_cast<float>(screenWidth) - kPanelWidth) * 0.5f;
    const float panelY = (static_cast<float>(screenHeight) - panelHeight) * 0.5f;

    ui.DrawRect(0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight), glm::vec3(0.0f), 0.5f);
    ui.DrawRect(panelX, panelY, kPanelWidth, panelHeight, glm::vec3(0.08f), 0.92f);

    text.Draw(font, "BUY MENU", panelX + 16.0f, panelY + 10.0f, glm::vec3(1.0f));
    char moneyBuf[32];
    std::snprintf(moneyBuf, sizeof(moneyBuf), "$%d", wallet.GetBalance());
    const float moneyWidth = font.MeasureWidth(moneyBuf);
    text.Draw(font, moneyBuf, panelX + kPanelWidth - 16.0f - moneyWidth, panelY + 10.0f, glm::vec3(0.35f, 0.9f, 0.35f));

    float rowY = panelY + kTitleHeight;
    for (const Row& row : rows) {
        const glm::vec3 bg = row.disabled ? glm::vec3(0.12f) : glm::vec3(0.18f);
        ui.DrawRect(panelX + 12.0f, rowY, kPanelWidth - 24.0f, kRowHeight, bg, 0.9f);

        char rowLabel[80];
        std::snprintf(rowLabel, sizeof(rowLabel), "[%d] %s", row.hotkey - GLFW_KEY_1 + 1, row.label.c_str());
        const glm::vec3 textColor = row.disabled ? glm::vec3(0.5f) : glm::vec3(1.0f);
        text.Draw(font, rowLabel, panelX + 20.0f, rowY + 8.0f, textColor);

        char priceBuf[24];
        std::snprintf(priceBuf, sizeof(priceBuf), row.disabled ? "OWNED" : "$%d", row.cost);
        const float priceWidth = font.MeasureWidth(priceBuf);
        text.Draw(font, priceBuf, panelX + kPanelWidth - 20.0f - priceWidth, rowY + 8.0f, textColor);

        rowY += kRowHeight + kRowSpacing;
    }

    if (m_messageTimer > 0.0f && !m_lastMessage.empty()) {
        const glm::vec3 msgColor = m_lastMessageWasError ? glm::vec3(0.9f, 0.3f, 0.3f) : glm::vec3(0.4f, 0.9f, 0.4f);
        const float msgWidth = font.MeasureWidth(m_lastMessage);
        text.Draw(font, m_lastMessage, panelX + (kPanelWidth - msgWidth) * 0.5f, rowY + 6.0f, msgColor);
    }

    text.Draw(font, "Number keys to buy - ESC to close", panelX + 16.0f, panelY + panelHeight - 24.0f, glm::vec3(0.6f));
}

} // namespace Game

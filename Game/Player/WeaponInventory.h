#pragma once

#include <memory>
#include <vector>
#include "../Weapons/Weapon.h"

namespace Game {

// Holds the player's carried weapons and which one is currently equipped.
// Deliberately simple (a flat list + index) — the buy-menu-driven loadout
// system arrives in Milestone 8; this just needs to prove multiple weapons
// with different firing behavior can coexist and be switched between.
class WeaponInventory {
public:
    void AddWeapon(std::unique_ptr<Weapon> weapon);

    // Fills (or replaces) one fixed slot — how the buy menu grants a
    // purchased weapon without disturbing the other slots. Resizes to fit
    // if index is past the current end, leaving any newly-created
    // in-between slots empty (nullptr) rather than requiring every slot be
    // filled in order.
    void SetWeaponAt(size_t index, std::unique_ptr<Weapon> weapon);
    bool HasWeaponAt(size_t index) const;
    Weapon* GetWeaponAt(size_t index);

    // No-ops if the slot is empty (nothing bought there yet) — the caller
    // keeps whatever was equipped rather than being left holding nothing.
    void SwitchTo(size_t index);

    Weapon* GetCurrent();
    const Weapon* GetCurrent() const;
    size_t GetCurrentIndex() const { return m_currentIndex; }
    size_t GetWeaponCount() const { return m_weapons.size(); }

private:
    std::vector<std::unique_ptr<Weapon>> m_weapons;
    size_t m_currentIndex = 0;
};

} // namespace Game

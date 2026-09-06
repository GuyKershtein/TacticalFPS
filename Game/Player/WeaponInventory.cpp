#include "WeaponInventory.h"

namespace Game {

void WeaponInventory::AddWeapon(std::unique_ptr<Weapon> weapon) {
    m_weapons.push_back(std::move(weapon));
}

void WeaponInventory::SetWeaponAt(size_t index, std::unique_ptr<Weapon> weapon) {
    if (index >= m_weapons.size()) {
        m_weapons.resize(index + 1);
    }
    m_weapons[index] = std::move(weapon);
}

bool WeaponInventory::HasWeaponAt(size_t index) const {
    return index < m_weapons.size() && m_weapons[index] != nullptr;
}

Weapon* WeaponInventory::GetWeaponAt(size_t index) {
    return index < m_weapons.size() ? m_weapons[index].get() : nullptr;
}

void WeaponInventory::SwitchTo(size_t index) {
    if (index < m_weapons.size() && m_weapons[index]) {
        m_currentIndex = index;
    }
}

Weapon* WeaponInventory::GetCurrent() {
    return m_currentIndex < m_weapons.size() ? m_weapons[m_currentIndex].get() : nullptr;
}

const Weapon* WeaponInventory::GetCurrent() const {
    return m_currentIndex < m_weapons.size() ? m_weapons[m_currentIndex].get() : nullptr;
}

} // namespace Game

#include "Wallet.h"

#include <algorithm>

namespace Game {

void Wallet::Reset() {
    m_balance = kStartingMoney;
}

void Wallet::Add(int amount) {
    m_balance = std::min(m_balance + amount, kMaxMoney);
}

bool Wallet::TrySpend(int amount) {
    if (amount > m_balance) return false;
    m_balance -= amount;
    return true;
}

} // namespace Game

#pragma once

namespace Game {

// A single player's money. Round-result/kill/objective payouts are applied
// by RoundManager; spending happens through whatever buy interface exists
// (a temporary keyboard stand-in this milestone — the real buy menu is
// Milestone 8's UI work, per the plan's own split between "Economy" (M5)
// and "Buy system" (M8)).
class Wallet {
public:
    void Reset();

    void Add(int amount);
    bool TrySpend(int amount); // returns false (and spends nothing) if insufficient funds

    int GetBalance() const { return m_balance; }

    static constexpr int kStartingMoney = 800;
    static constexpr int kMaxMoney = 8000;

private:
    int m_balance = kStartingMoney;
};

} // namespace Game

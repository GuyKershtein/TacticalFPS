#include "NetworkManager.h"

#include <cstdio>
#include <cstring>

namespace Game {

bool NetworkManager::StartHost(uint16_t port) {
    if (!m_channel.StartHost(port, kMaxNetPlayers - 1)) return false;
    m_role = NetRole::Host;
    std::printf("[Net] Hosting on port %u (max %d remote players)\n", port, kMaxNetPlayers - 1);
    std::fflush(stdout);
    return true;
}

bool NetworkManager::StartClient(const std::string& address, uint16_t port) {
    if (!m_channel.StartClient(address, port)) return false;
    m_role = NetRole::Client;
    std::printf("[Net] Connecting to %s:%u...\n", address.c_str(), port);
    std::fflush(stdout);
    return true;
}

void NetworkManager::Shutdown() {
    m_channel.Shutdown();
    m_role = NetRole::Offline;
    m_peerToSlot.clear();
    for (bool& used : m_slotUsed) used = false;
    m_clientAssignedSlot = -1;
}

void NetworkManager::HostPoll(std::vector<std::pair<int, ClientInputPacket>>& outInputs,
    std::vector<int>& outJoined, std::vector<int>& outLeft) {
    outInputs.clear();
    outJoined.clear();
    outLeft.clear();
    if (m_role != NetRole::Host) return;

    for (const Engine::NetEvent& event : m_channel.PollEvents()) {
        switch (event.type) {
            case Engine::NetEventType::Connected: {
                int slot = -1;
                for (int i = 1; i < kMaxNetPlayers; ++i) {
                    if (!m_slotUsed[i]) { slot = i; break; }
                }
                if (slot < 0) {
                    std::printf("[Net] Rejecting connection: server full\n");
                    std::fflush(stdout);
                    break; // ENet already accepted at the transport level; the game simply won't spawn an avatar for it
                }
                m_slotUsed[slot] = true;
                m_peerToSlot[event.peer] = slot;
                outJoined.push_back(slot);

                ServerWelcomePacket welcome;
                welcome.assignedSlot = static_cast<uint8_t>(slot);
                m_channel.SendReliable(event.peer, &welcome, sizeof(welcome));
                std::printf("[Net] Client joined as slot %d\n", slot);
                std::fflush(stdout);
                break;
            }
            case Engine::NetEventType::Disconnected: {
                auto it = m_peerToSlot.find(event.peer);
                if (it != m_peerToSlot.end()) {
                    std::printf("[Net] Slot %d disconnected\n", it->second);
                    std::fflush(stdout);
                    m_slotUsed[it->second] = false;
                    outLeft.push_back(it->second);
                    m_peerToSlot.erase(it);
                }
                break;
            }
            case Engine::NetEventType::Data: {
                auto it = m_peerToSlot.find(event.peer);
                if (it == m_peerToSlot.end() || event.data.size() != sizeof(ClientInputPacket)) break;
                ClientInputPacket packet;
                std::memcpy(&packet, event.data.data(), sizeof(packet));
                if (packet.type != PacketType::ClientInput) break;
                outInputs.emplace_back(it->second, packet);
                break;
            }
        }
    }
}

void NetworkManager::HostBroadcastSnapshot(const ServerSnapshotPacket& snapshot) {
    if (m_role != NetRole::Host) return;
    m_channel.Broadcast(&snapshot, sizeof(snapshot), /*reliable*/ false);
}

void NetworkManager::ClientSendInput(const ClientInputPacket& input) {
    if (m_role != NetRole::Client) return;
    m_channel.SendUnreliable(m_channel.GetServerPeer(), &input, sizeof(input));
}

bool NetworkManager::ClientPoll(ServerSnapshotPacket& outSnapshot) {
    if (m_role != NetRole::Client) return false;

    bool gotSnapshot = false;
    for (const Engine::NetEvent& event : m_channel.PollEvents()) {
        switch (event.type) {
            case Engine::NetEventType::Connected:
                std::printf("[Net] Connected to host\n");
                std::fflush(stdout);
                break;
            case Engine::NetEventType::Disconnected:
                std::printf("[Net] Disconnected from host\n");
                std::fflush(stdout);
                m_clientAssignedSlot = -1;
                break;
            case Engine::NetEventType::Data: {
                if (event.data.empty()) break;
                const PacketType type = static_cast<PacketType>(event.data[0]);
                if (type == PacketType::ServerWelcome && event.data.size() == sizeof(ServerWelcomePacket)) {
                    ServerWelcomePacket welcome;
                    std::memcpy(&welcome, event.data.data(), sizeof(welcome));
                    m_clientAssignedSlot = welcome.assignedSlot;
                    std::printf("[Net] Assigned slot %d\n", m_clientAssignedSlot);
                    std::fflush(stdout);
                } else if (type == PacketType::ServerSnapshot && event.data.size() == sizeof(ServerSnapshotPacket)) {
                    // If multiple snapshots arrived this poll, only the
                    // last one matters — earlier ones are already stale.
                    std::memcpy(&outSnapshot, event.data.data(), sizeof(outSnapshot));
                    gotSnapshot = true;
                }
                break;
            }
        }
    }
    return gotSnapshot;
}

} // namespace Game

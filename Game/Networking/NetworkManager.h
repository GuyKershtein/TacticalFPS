#pragma once

#include "../../Engine/Networking/NetChannel.h"
#include "NetProtocol.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Game {

enum class NetRole { Offline, Host, Client };

// Owns the NetChannel and translates its raw byte events into game packet
// types, plus tracks which network slot (see NetProtocol.h) each connected
// peer occupies. GameApplication owns one of these and is the only thing
// that interprets what a slot's ClientInputPacket/PlayerSnapshot actually
// means for gameplay — this class only knows about connections and bytes.
class NetworkManager {
public:
    bool StartHost(uint16_t port);
    bool StartClient(const std::string& address, uint16_t port);
    void Shutdown();
    NetRole GetRole() const { return m_role; }

    // Host: call once per frame. outInputs holds the latest ClientInput
    // received this frame per slot (a slot with nothing new this frame is
    // simply absent — the caller keeps using its last known input rather
    // than snapping to zero). outJoined/outLeft report slots that
    // connected/disconnected this frame (a new slot is welcomed
    // automatically, before HostPoll returns).
    void HostPoll(std::vector<std::pair<int, ClientInputPacket>>& outInputs,
        std::vector<int>& outJoined, std::vector<int>& outLeft);
    void HostBroadcastSnapshot(const ServerSnapshotPacket& snapshot);

    // Client: call once per frame.
    void ClientSendInput(const ClientInputPacket& input);
    // Returns true if at least one new snapshot arrived (the latest one,
    // if several arrived since the last poll — older ones are stale by
    // definition, so there is no reason to process them).
    bool ClientPoll(ServerSnapshotPacket& outSnapshot);
    bool IsConnectedToHost() const { return m_clientAssignedSlot >= 0; }
    int GetLocalSlot() const { return m_clientAssignedSlot; }

private:
    Engine::NetChannel m_channel;
    NetRole m_role = NetRole::Offline;

    std::unordered_map<Engine::PeerId, int> m_peerToSlot; // host-only
    bool m_slotUsed[kMaxNetPlayers] = {}; // host-only; slot 0 is reserved for the host's own local avatar and never assigned

    int m_clientAssignedSlot = -1; // client-only, until the ServerWelcome arrives
};

} // namespace Game

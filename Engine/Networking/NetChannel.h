#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Engine {

// Opaque handle to a connected peer, valid only for this NetChannel
// instance's lifetime — actually the ENetPeer pointer's bit pattern, but
// callers never dereference it, so ENet's types don't need to leak into
// this header (same "keep the third-party type out of the interface" rule
// AudioSystem.h follows for miniaudio's ma_engine).
using PeerId = uintptr_t;
constexpr PeerId kInvalidPeer = 0;

enum class NetEventType { Connected, Disconnected, Data };

struct NetEvent {
    NetEventType type;
    PeerId peer;
    std::vector<uint8_t> data; // populated only for Data events
};

// Thin wrapper over ENet — reliable/unreliable UDP delivery and connection
// management, nothing more. It knows only about bytes and peer handles;
// Game/Networking's NetProtocol (what the bytes mean) and NetworkManager
// (host/client roles, who's connected) are built on top, the same
// division of responsibility GLFW (windowing) and miniaudio (audio device
// I/O) have elsewhere in this engine.
class NetChannel {
public:
    ~NetChannel();

    // Starts as a listen host accepting up to maxPeers connections.
    bool StartHost(uint16_t port, size_t maxPeers);

    // Starts as a client and begins connecting to a host. The connection
    // isn't confirmed until PollEvents() reports a Connected event (or the
    // ENet connection attempt times out, reported as Disconnected).
    bool StartClient(const std::string& address, uint16_t port);

    void Shutdown();
    bool IsActive() const { return m_host != nullptr; }

    // Pumps the ENet event queue; call once per frame. Non-blocking.
    std::vector<NetEvent> PollEvents();

    void SendReliable(PeerId peer, const void* data, size_t size);
    void SendUnreliable(PeerId peer, const void* data, size_t size);

    // Host-only: queues the packet for every currently-connected peer in
    // one call, rather than the caller looping SendX per peer.
    void Broadcast(const void* data, size_t size, bool reliable);

    // Host-only: the single peer a client is connected to, once
    // established — set on the Connected event.
    PeerId GetServerPeer() const { return m_serverPeer; }

private:
    void* m_host = nullptr;       // ENetHost*
    PeerId m_serverPeer = kInvalidPeer; // client-side only: the host's peer handle
    bool m_ownsEnetInit = false;
};

} // namespace Engine

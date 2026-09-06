#include "NetChannel.h"

#include <enet/enet.h>
#include <cstdio>

namespace Engine {

namespace {
// ENet requires exactly one initialize/deinitialize pair per process. Since
// only one NetChannel is ever alive at a time in this game, a simple
// refcount here is enough — no need for a more elaborate singleton.
int g_enetRefCount = 0;

bool EnsureEnetInitialized() {
    if (g_enetRefCount == 0 && enet_initialize() != 0) {
        std::fprintf(stderr, "[NetChannel] enet_initialize failed\n");
        return false;
    }
    ++g_enetRefCount;
    return true;
}

void ReleaseEnetInit() {
    --g_enetRefCount;
    if (g_enetRefCount == 0) {
        enet_deinitialize();
    }
}
} // namespace

NetChannel::~NetChannel() {
    Shutdown();
}

bool NetChannel::StartHost(uint16_t port, size_t maxPeers) {
    if (!EnsureEnetInitialized()) return false;
    m_ownsEnetInit = true;

    ENetAddress address{};
    address.host = ENET_HOST_ANY;
    address.port = port;

    m_host = enet_host_create(&address, maxPeers, /*channelLimit*/ 1, 0, 0);
    if (!m_host) {
        std::fprintf(stderr, "[NetChannel] enet_host_create (listen) failed on port %u\n", port);
        ReleaseEnetInit();
        m_ownsEnetInit = false;
        return false;
    }
    return true;
}

bool NetChannel::StartClient(const std::string& address, uint16_t port) {
    if (!EnsureEnetInitialized()) return false;
    m_ownsEnetInit = true;

    m_host = enet_host_create(nullptr, /*peerCount*/ 1, /*channelLimit*/ 1, 0, 0);
    if (!m_host) {
        std::fprintf(stderr, "[NetChannel] enet_host_create (client) failed\n");
        ReleaseEnetInit();
        m_ownsEnetInit = false;
        return false;
    }

    ENetAddress enetAddress{};
    if (enet_address_set_host(&enetAddress, address.c_str()) != 0) {
        std::fprintf(stderr, "[NetChannel] Could not resolve host: %s\n", address.c_str());
        Shutdown();
        return false;
    }
    enetAddress.port = port;

    ENetPeer* peer = enet_host_connect(static_cast<ENetHost*>(m_host), &enetAddress, 1, 0);
    if (!peer) {
        std::fprintf(stderr, "[NetChannel] enet_host_connect failed\n");
        Shutdown();
        return false;
    }
    m_serverPeer = reinterpret_cast<PeerId>(peer);
    return true;
}

void NetChannel::Shutdown() {
    if (m_host) {
        enet_host_destroy(static_cast<ENetHost*>(m_host));
        m_host = nullptr;
    }
    m_serverPeer = kInvalidPeer;
    if (m_ownsEnetInit) {
        ReleaseEnetInit();
        m_ownsEnetInit = false;
    }
}

std::vector<NetEvent> NetChannel::PollEvents() {
    std::vector<NetEvent> events;
    if (!m_host) return events;

    ENetEvent enetEvent;
    // Non-blocking: a timeout of 0 makes enet_host_service return
    // immediately once its internal queue is drained, so this never stalls
    // the frame waiting on the network.
    while (enet_host_service(static_cast<ENetHost*>(m_host), &enetEvent, 0) > 0) {
        NetEvent event{};
        event.peer = reinterpret_cast<PeerId>(enetEvent.peer);

        switch (enetEvent.type) {
            case ENET_EVENT_TYPE_CONNECT:
                event.type = NetEventType::Connected;
                events.push_back(std::move(event));
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                event.type = NetEventType::Disconnected;
                events.push_back(std::move(event));
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                event.type = NetEventType::Data;
                event.data.assign(enetEvent.packet->data, enetEvent.packet->data + enetEvent.packet->dataLength);
                events.push_back(std::move(event));
                enet_packet_destroy(enetEvent.packet);
                break;
            default:
                break;
        }
    }
    return events;
}

void NetChannel::SendReliable(PeerId peer, const void* data, size_t size) {
    if (!m_host || peer == kInvalidPeer) return;
    ENetPacket* packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(reinterpret_cast<ENetPeer*>(peer), 0, packet);
}

void NetChannel::SendUnreliable(PeerId peer, const void* data, size_t size) {
    if (!m_host || peer == kInvalidPeer) return;
    ENetPacket* packet = enet_packet_create(data, size, 0);
    enet_peer_send(reinterpret_cast<ENetPeer*>(peer), 0, packet);
}

void NetChannel::Broadcast(const void* data, size_t size, bool reliable) {
    if (!m_host) return;
    ENetPacket* packet = enet_packet_create(data, size, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    enet_host_broadcast(static_cast<ENetHost*>(m_host), 0, packet);
}

} // namespace Engine

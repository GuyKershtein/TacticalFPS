#pragma once

#include <cstdint>
#include <cstring>

namespace Game {

// Packets are sent as raw struct bytes (no bitstream/varint encoding, no
// endianness normalization) — a deliberate scope boundary for this
// milestone's demonstration, not an oversight: it only works between
// identically-built binaries on identically-endian machines (true for
// every realistic test of this project: two copies of the same .exe on
// the same or LAN Windows machines, all x86-64/little-endian). A shipping
// engine would replace this with an explicit bitstream writer; the
// host/client architecture and packet flow around it would stay the same.

constexpr int kMaxNetPlayers = 4; // 1 host + up to 3 remote clients
constexpr uint16_t kDefaultPort = 7717;

enum class PacketType : uint8_t { ClientInput, ServerSnapshot, ServerWelcome };

// Button bits for ClientInputPacket::buttons.
enum ClientButton : uint8_t {
    kButtonJump = 1 << 0,
    kButtonCrouch = 1 << 1,
    kButtonWalk = 1 << 2,
    kButtonFire = 1 << 3,
    kButtonReload = 1 << 4,
};

// Sent client -> host every frame. The client applies its own mouse-look
// locally (for immediate visual response) and simply reports the resulting
// view angles, rather than the host replaying raw mouse deltas through its
// own sensitivity/smoothing math — one less thing that has to stay
// bit-for-bit identical between client and host builds.
struct ClientInputPacket {
    PacketType type = PacketType::ClientInput;
    uint32_t sequence = 0;
    float wishForward = 0.0f;
    float wishRight = 0.0f;
    float yawDegrees = 0.0f;
    float pitchDegrees = 0.0f;
    uint8_t buttons = 0;
    uint8_t weaponSwitch = 0;  // 0 = none, else 1-based slot request
    uint8_t grenadeThrow = 0;  // 0 = none, else 1-based GrenadeKind+1
};

// One player's state within a ServerSnapshotPacket.
struct PlayerSnapshot {
    uint8_t occupied = 0; // 0 = this slot has no player; skip it
    uint8_t team = 0;     // TeamId
    uint8_t alive = 0;
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    float yawDegrees = 0.0f, pitchDegrees = 0.0f;
    float health = 0.0f, armor = 0.0f;
    int32_t magazineAmmo = 0, reserveAmmo = 0;
    char weaponName[24] = {};
};

// Sent once, reliably, right after a client connects — tells it which
// players[] slot in every future ServerSnapshotPacket is its own, so the
// snapshot itself can be one identical broadcast to everyone instead of a
// per-recipient unicast.
struct ServerWelcomePacket {
    PacketType type = PacketType::ServerWelcome;
    uint8_t assignedSlot = 0;
};

// Broadcast host -> every client once per frame: full authoritative state
// for every connected player plus round state, so a client is a "dumb
// terminal" that only ever renders what it's told rather than needing its
// own copy of round/economy rules.
struct ServerSnapshotPacket {
    PacketType type = PacketType::ServerSnapshot;
    uint32_t tick = 0;
    uint8_t roundPhase = 0; // RoundPhase
    float phaseTimeRemaining = 0.0f;
    uint8_t assaultScore = 0;
    uint8_t guardianScore = 0;
    PlayerSnapshot players[kMaxNetPlayers];
};

} // namespace Game

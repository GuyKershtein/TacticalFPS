#pragma once

#include "../Engine/Core/Window.h"
#include "../Engine/Core/Time.h"
#include "../Engine/Input/InputManager.h"
#include "../Engine/Graphics/Shader.h"
#include "../Engine/Graphics/Mesh.h"
#include "../Engine/Graphics/DebugLineRenderer.h"
#include "../Engine/Graphics/ParticleSystem.h"
#include "../Engine/Graphics/DecalSystem.h"
#include "../Engine/World/CollisionWorld.h"
#include "../Engine/World/Level.h"
#include "../Engine/World/MaterialDatabase.h"
#include "../Engine/Navigation/NavGraph.h"
#include "../Engine/Audio/AudioSystem.h"
#include "../Engine/Audio/SoundBank.h"
#include "../Engine/UI/Font.h"
#include "../Engine/UI/TextRenderer.h"
#include "../Engine/UI/UIRenderer.h"
#include "UI/Hud.h"
#include "UI/BuyMenu.h"
#include "UI/PauseMenu.h"
#include "Player/PlayerController.h"
#include "Player/WeaponInventory.h"
#include "Player/PlayerHealth.h"
#include "Combat/TargetDummy.h"
#include "Economy/Wallet.h"
#include "Rounds/RoundManager.h"
#include "Bots/Bot.h"
#include "Bots/SoundEventBus.h"
#include "Grenades/Grenade.h"
#include "Grenades/SmokeCloud.h"
#include "Networking/NetworkManager.h"

#include <vector>
#include <memory>
#include <utility>
#include <string>
#include <glm/glm.hpp>

namespace Game {

// A single renderable placed in the world: which mesh, where.
// Deliberately not a full entity/component system yet — a real
// Entity/Transform/Component architecture arrives once there's shared
// behavior across many different object kinds to justify it.
struct SceneObject {
    Engine::Mesh* mesh = nullptr;
    glm::mat4 transform{1.0f};
};

// A lingering fake-gunfire source left behind by a detonated decoy
// grenade — not a Grenade itself (its flight/fuse is already over), just a
// simple timer that periodically pushes a Gunshot SoundEvent for bots to
// investigate.
struct DecoyEmitter {
    glm::vec3 position{0.0f};
    float remainingSeconds = 0.0f;
    float nextBeepTimer = 0.0f;
};

// Command-line-driven launch configuration (Milestone 10). Offline is the
// default and preserves every prior milestone's behavior unchanged.
struct LaunchOptions {
    NetRole role = NetRole::Offline;
    std::string connectAddress; // Client only
    uint16_t port = kDefaultPort;
};

// Host-only: a remote client's avatar, fully simulated by reusing the same
// PlayerController/PlayerHealth code the local player uses — the host is
// authoritative, so it runs real physics/collision for every connected
// player, not just its own. Fixed at a Carbine loadout rather than wired
// into WeaponInventory/BuyMenu — extending the buy system to remote
// players is future work this milestone doesn't cover (see the Milestone
// 10 write-up for the full list of scope boundaries).
struct HostRemotePlayer {
    bool active = false;
    PlayerController controller;
    PlayerHealth health;
    std::unique_ptr<Weapon> weapon;
    TeamId team = TeamId::Assault;
    ClientInputPacket lastInput; // reused on frames where nothing new arrived
};

// Client-only: a render-only view of another connected player, populated
// straight from the latest ServerSnapshotPacket. The client's OWN avatar
// doesn't need one of these — its position/health/weapon come from the
// snapshot too, but drive m_player/m_playerHealth directly (see
// GameApplication::ProcessInput's Client branch).
struct ClientRemoteAvatarView {
    bool active = false;
    glm::vec3 position{0.0f};
    float yawDegrees = 0.0f;
    TeamId team = TeamId::Assault;
    bool alive = true;
};

// Top-level owner of the engine subsystems, the loaded map, the player
// controller, weapons, and the main loop.
//
// This lives in Game/, not Engine/Core, on purpose: it already does
// game-specific things (which map to load, which player controller to run),
// so if it lived in Engine/, Engine would end up depending on Game/ headers
// — backwards from the intended layering, where Game depends on Engine and
// never the reverse.
class GameApplication {
public:
    bool Init(const LaunchOptions& options = {});
    void Run();
    void Shutdown();

private:
    void ProcessInput(float deltaTime);
    void HandleFireInput();
    void HandleGrenadeInput();
    void UpdateBots(float deltaTime);
    void UpdateGrenades(float deltaTime);
    void DetonateGrenade(const Grenade& grenade);
    void PlayImpactEffect(const glm::vec3& point, const glm::vec3& normal, const std::string& material);
    void BuildPlayerHitboxTargets(std::vector<HitscanTarget>& outTargets);

    // Dummies + bots + every other player (local and, on a Host, every
    // remote client) except whichever one is doing the firing — shared by
    // HandleFireInput (firingSlot 0, the local player) and ProcessInputHost
    // (firingSlot = the remote client's own slot) so both fire paths hit
    // the exact same set of things.
    void CollectAllHitscanTargets(std::vector<HitscanTarget>& outTargets, int firingSlot);
    void Render();
    void RenderScreenOverlay();
    void RenderHud();
    void RenderDebugVisualization();

    // Milestone 10: networking. Split out of ProcessInput/Render (rather
    // than interleaving role checks through the existing offline code path
    // line by line) so Offline's behavior is provably untouched — these
    // are only ever called when m_network.GetRole() is Host/Client.
    void ProcessInputHost(float deltaTime);
    void ProcessInputClient(float deltaTime);
    void RenderRemoteAvatars();

    Engine::Window m_window;
    Engine::InputManager m_input;
    Engine::Time m_time;
    Engine::Shader m_shader;
    Engine::Shader m_overlayShader;

    Engine::Mesh m_levelMesh;
    std::vector<SceneObject> m_sceneObjects;
    Engine::MaterialDatabase m_materials;

    Engine::CollisionWorld m_collisionWorld;
    // Kept as a member (not a local in Init()) so its entities — the target
    // dummies among them — stay alive and tickable for the app's lifetime.
    Engine::Level m_level;
    std::vector<TargetDummy*> m_targetDummies;

    PlayerController m_player;
    WeaponInventory m_weaponInventory;
    Engine::DebugLineRenderer m_debugLines;

    PlayerHealth m_playerHealth;
    Wallet m_playerWallet;
    RoundManager m_roundManager;
    RoundPhase m_previousRoundPhase = RoundPhase::Buy;
    ChargeState m_previousChargeState = ChargeState::Carried;
    bool m_wasPlayerAlive = true;

    // Milestone 6: bots.
    Engine::NavGraph m_navGraph;
    std::vector<std::unique_ptr<Bot>> m_bots;
    SoundEventBus m_soundEvents;
    float m_footstepSoundTimer = 0.0f;

    // Simple placeholder visual bodies — no character models/animation
    // system yet (that's a later phase). Separate meshes so bots and
    // dummies are visually distinguishable at a glance.
    Engine::Mesh m_botBodyMesh;
    Engine::Mesh m_dummyBodyMesh;

    // Milestone 7: audio, particles/decals, grenades.
    Engine::AudioSystem m_audio;
    Engine::SoundBank m_soundBank;
    Engine::ParticleSystem m_particles;
    Engine::DecalSystem m_decals;

    std::vector<Grenade> m_grenades;
    std::vector<SmokeCloud> m_smokeClouds;
    std::vector<DecoyEmitter> m_decoyEmitters;
    int m_grenadeCounts[4] = {0, 0, 0, 0}; // indexed by GrenadeKind; must be bought each round via BuyMenu

    float m_playerBlindRemaining = 0.0f;
    float m_playerBlindMax = 0.0f;

    // Raw GL objects for the full-screen flashbang overlay quad — not a
    // Mesh (whose vertex layout is position+normal+color for lit/unlit
    // level geometry, not the plain 2D NDC quad this needs).
    unsigned int m_overlayVao = 0;
    unsigned int m_overlayVbo = 0;

    // Milestone 8: UI (HUD/menus/buy system). Font/TextRenderer/UIRenderer
    // are shared GL resources; Hud is a stateless drawer over them, while
    // BuyMenu/PauseMenu carry their own small bits of state (open/closed,
    // last-purchase message).
    Engine::Font m_font;
    Engine::TextRenderer m_textRenderer;
    Engine::UIRenderer m_uiRenderer;
    BuyMenu m_buyMenu;
    PauseMenu m_pauseMenu;
    bool m_paused = false;

    // Milestone 9: debugging tools. Wireframe edges are computed once at
    // load time (world geometry is static) rather than re-triangulating
    // every frame just to draw lines.
    bool m_debugShowNav = false;
    bool m_debugShowCollision = false;
    bool m_debugShowStats = false;
    std::vector<std::pair<glm::vec3, glm::vec3>> m_collisionWireframeEdges;

    // Milestone 10: networking. See NetworkManager.h and NetProtocol.h for
    // the transport/wire-format side; the members below are purely how
    // GameApplication interprets a connection once one exists.
    NetworkManager m_network;
    uint32_t m_netInputSequence = 0;

    // Host-only: slots 1..kMaxNetPlayers-1 (slot 0 is the host's own
    // m_player, already covered above). Broadcast once per frame.
    HostRemotePlayer m_hostRemotePlayers[kMaxNetPlayers];

    // Client-only: the latest state received for every OTHER connected
    // player, and this client's own assigned slot for interpreting the
    // snapshot's players[] array.
    ClientRemoteAvatarView m_clientRemoteAvatars[kMaxNetPlayers];
    ServerSnapshotPacket m_lastClientSnapshot;
    bool m_hasClientSnapshot = false;

    // Mesh for any networked player avatar (host's remote clients, or the
    // client's view of everyone else) — a third distinct color so it
    // reads as neither "bot" nor "target dummy" at a glance.
    Engine::Mesh m_remotePlayerBodyMesh;
};

} // namespace Game

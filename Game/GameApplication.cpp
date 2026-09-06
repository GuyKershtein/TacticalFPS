#include "GameApplication.h"
#include "../Engine/Graphics/GLFunctions.h"
#include "../Engine/World/MapLoader.h"
#include "../Engine/World/NavWaypointEntity.h"
#include "Weapons/WeaponPresets.h"
#include "Combat/HitscanSystem.h"
#include "Bots/BotDifficulty.h"
#include "../Engine/Physics/CollisionQuery.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace Game {

namespace {
constexpr float kMovingSpeedThreshold = 10.0f; // engine units/sec; below this, spread treats the player as stationary
constexpr float kFootstepIntervalSeconds = 0.35f;
}

bool GameApplication::Init(const LaunchOptions& options) {
    const char* roleSuffix = options.role == NetRole::Host ? " [HOST]" : options.role == NetRole::Client ? " [CLIENT]" : "";
    if (!m_window.Create(1280, 720, std::string("Tactical FPS - Milestone 10") + roleSuffix)) {
        return false;
    }

    m_input.Init(m_window.GetHandle());
    m_input.SetCursorLocked(true);

    m_time.Init();

    if (options.role == NetRole::Host) {
        if (!m_network.StartHost(options.port)) {
            std::fprintf(stderr, "[GameApplication] Failed to start host on port %u\n", options.port);
            return false;
        }
    } else if (options.role == NetRole::Client) {
        if (!m_network.StartClient(options.connectAddress, options.port)) {
            std::fprintf(stderr, "[GameApplication] Failed to connect to %s:%u\n", options.connectAddress.c_str(), options.port);
            return false;
        }
    }

    const std::string shaderDir = std::string(PROJECT_ROOT_DIR) + "/Assets/Shaders/";
    if (!m_shader.LoadFromFiles(shaderDir + "basic.vert", shaderDir + "basic.frag")) {
        std::fprintf(stderr, "[GameApplication] Failed to load basic shader\n");
        return false;
    }
    if (!m_overlayShader.LoadFromFiles(shaderDir + "overlay.vert", shaderDir + "overlay.frag")) {
        std::fprintf(stderr, "[GameApplication] Failed to load overlay shader\n");
        return false;
    }
    m_debugLines.Init();
    m_particles.Init();
    m_decals.Init();

    if (!m_audio.Init()) {
        std::fprintf(stderr, "[GameApplication] Audio device unavailable; continuing without sound\n");
    }
    m_soundBank.Init();

    // Milestone 8: HUD/menu text and 2D rendering. Rasterizes glyphs from
    // Consolas, an OS-installed font every Windows machine ships with —
    // avoids bundling a font asset or fetching one over the network purely
    // for licensing/availability reasons, the same "no extra dependency
    // beyond what the platform already gives us" reasoning as everything
    // else in Engine/.
    if (!m_uiRenderer.Init(shaderDir)) {
        std::fprintf(stderr, "[GameApplication] Failed to init UIRenderer\n");
        return false;
    }
    if (!m_textRenderer.Init(shaderDir)) {
        std::fprintf(stderr, "[GameApplication] Failed to init TextRenderer\n");
        return false;
    }
    if (!m_font.LoadFromFile("C:/Windows/Fonts/consola.ttf", 20.0f) &&
        !m_font.LoadFromFile("C:/Windows/Fonts/arial.ttf", 20.0f)) {
        std::fprintf(stderr, "[GameApplication] No usable system font found; HUD text will not render\n");
    }

    // A full-screen quad in NDC space (two triangles), for the flashbang
    // whiteout — not a Mesh, since that struct's layout is position+normal
    // +color for level geometry, not a plain 2D screen-space quad.
    const float overlayVertices[] = {
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f, 1.0f,
        -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, 1.0f,
    };
    glGenVertexArrays(1, &m_overlayVao);
    glGenBuffers(1, &m_overlayVbo);
    glBindVertexArray(m_overlayVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_overlayVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(overlayVertices), overlayVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);

    const std::string materialsPath = std::string(PROJECT_ROOT_DIR) + "/Assets/Materials/materials.tmat";
    if (!m_materials.LoadFromFile(materialsPath)) {
        std::fprintf(stderr, "[GameApplication] Failed to load materials: %s\n", materialsPath.c_str());
        return false;
    }

    const std::string mapPath = std::string(PROJECT_ROOT_DIR) + "/Assets/Maps/test_room.tmap";
    if (!Engine::MapLoader::Load(mapPath, m_level)) {
        std::fprintf(stderr, "[GameApplication] Failed to load map: %s\n", mapPath.c_str());
        return false;
    }

    // Combine every world brush into one mesh/draw call; per-brush culling
    // is a later optimization (Milestone 9), not needed for one small map.
    std::vector<Engine::Vertex> levelVertices;
    std::vector<unsigned int> levelIndices;
    for (const Engine::Brush& brush : m_level.worldBrushes) {
        brush.Triangulate(levelVertices, levelIndices, m_materials);
    }
    m_levelMesh.Create(levelVertices, levelIndices);
    m_sceneObjects.push_back({&m_levelMesh, glm::mat4(1.0f)});

    // Milestone 9 debug tool: cache collision wireframe edges once, from
    // the same triangulation already computed above for rendering, instead
    // of re-triangulating every frame just to draw lines that never change
    // (world geometry is static).
    for (size_t i = 0; i + 2 < levelIndices.size(); i += 3) {
        const glm::vec3& a = levelVertices[levelIndices[i]].position;
        const glm::vec3& b = levelVertices[levelIndices[i + 1]].position;
        const glm::vec3& c = levelVertices[levelIndices[i + 2]].position;
        m_collisionWireframeEdges.emplace_back(a, b);
        m_collisionWireframeEdges.emplace_back(b, c);
        m_collisionWireframeEdges.emplace_back(c, a);
    }

    m_collisionWorld.Build(m_level.worldBrushes);
    std::printf("[GameApplication] Loaded %zu world brushes; %zu standing-hull / %zu crouching-hull brushes built\n",
        m_level.worldBrushes.size(), m_collisionWorld.GetStandingHullBrushes().size(), m_collisionWorld.GetCrouchingHullBrushes().size());

    for (const std::unique_ptr<Engine::Entity>& entity : m_level.entities) {
        if (auto* dummy = dynamic_cast<TargetDummy*>(entity.get())) {
            m_targetDummies.push_back(dummy);
        }
    }
    std::printf("[GameApplication] Found %zu target dummies\n", m_targetDummies.size());

    // Milestone 6: navigation graph, auto-linked from the map's
    // nav_waypoint entities (see NavGraph.h for why edges aren't authored).
    std::vector<glm::vec3> waypointPositions;
    for (const std::unique_ptr<Engine::Entity>& entity : m_level.entities) {
        if (auto* waypoint = dynamic_cast<Engine::NavWaypointEntity*>(entity.get())) {
            waypointPositions.push_back(waypoint->GetOrigin());
        }
    }
    m_navGraph.Build(waypointPositions, m_level.worldBrushes);
    std::printf("[GameApplication] Built nav graph with %zu waypoints\n", waypointPositions.size());
    std::fflush(stdout);

    m_botBodyMesh = Engine::Mesh::CreateCube(glm::vec3(0.75f, 0.25f, 0.2f));   // reddish: Guardian bots
    m_dummyBodyMesh = Engine::Mesh::CreateCube(glm::vec3(0.2f, 0.55f, 0.6f)); // teal: target dummies
    m_remotePlayerBodyMesh = Engine::Mesh::CreateCube(glm::vec3(0.7f, 0.6f, 0.1f)); // gold: networked human players (Milestone 10)

    // Fixed slots: 0 = Sidearm, 1 = primary rifle, 2 = shotgun, 3 = knife.
    // Only the pistol and knife are owned from the start (free, matching
    // the genre convention) — the Carbine and Street Sweeper slots start
    // empty and are filled by BuyMenu once purchased (Milestone 8).
    m_weaponInventory.SetWeaponAt(0, [] { auto w = std::make_unique<Weapon>(); w->Init(CreateSidearmData()); return w; }());
    m_weaponInventory.SetWeaponAt(3, [] { auto w = std::make_unique<MeleeWeapon>(); w->Init(CreateCombatKnifeData()); return w; }());
    m_weaponInventory.SwitchTo(0); // start with just the sidearm

    // A networked client is a thin presentation layer over the host's
    // authoritative simulation (see NetProtocol.h) — it never runs bots or
    // RoundManager itself; round/economy state arrives in every snapshot
    // instead. It still needs *a* starting camera position before the
    // first snapshot lands, so it spawns at the first map spawn point
    // exactly like a fresh single-player game would, and that position is
    // simply overwritten (see ProcessInputClient) once real data arrives.
    if (options.role == NetRole::Client) {
        const std::vector<Engine::SpawnPointEntity*>& spawns = m_level.GetSpawnPoints();
        if (!spawns.empty()) {
            m_player.Init(spawns.front()->GetOrigin(), spawns.front()->GetYawDegrees(), &m_collisionWorld);
        }
    } else {
        // Spawn a Guardian bot on every "guardian"-tagged spawn point the map
        // has (currently 2 — see test_room.tmap). Offline and Host both run
        // the exact same bot/round setup — a Host is authoritative for
        // itself plus whichever remote clients join, and bots don't care
        // which of those is shooting at them.
        std::vector<Bot*> guardianBotPointers;
        for (const Engine::SpawnPointEntity* spawn : m_level.GetSpawnPoints()) {
            if (spawn->GetTeam() != "guardian") continue;

            auto botWeapon = std::make_unique<Weapon>();
            botWeapon->Init(CreateCarbineData());

            auto bot = std::make_unique<Bot>();
            bot->Init(spawn->GetOrigin(), spawn->GetYawDegrees(), TeamId::Guardian, BotDifficulty::Medium(),
                &m_collisionWorld, &m_navGraph, std::move(botWeapon));

            guardianBotPointers.push_back(bot.get());
            m_bots.push_back(std::move(bot));
        }
        std::printf("[GameApplication] Spawned %zu Guardian bots\n", m_bots.size());
        std::fflush(stdout);

        m_roundManager.RegisterGuardianBots(guardianBotPointers);

        // RoundManager::Init() starts the first Buy phase, which spawns the
        // player at the team-tagged info_player_start (replacing the old
        // "just use whichever spawn point comes first" logic from Milestone 4)
        // and resets every registered bot for round 1.
        m_roundManager.Init(&m_player, &m_playerHealth, &m_playerWallet, &m_level, &m_collisionWorld);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.53f, 0.65f, 0.75f, 1.0f);

    // Opaque geometry always draws with alpha=1, so this has no visible
    // effect there — it's what lets particles/decals/the flashbang overlay
    // fade instead of just popping in/out.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

void GameApplication::HandleFireInput() {
    Weapon* weapon = m_weaponInventory.GetCurrent();
    if (!weapon) return;

    const bool wantsToFire = weapon->GetData().fullAuto
        ? m_input.IsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)
        : m_input.WasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
    if (!wantsToFire) return;

    const glm::vec3 origin = m_player.GetCamera().GetPosition();
    const glm::vec3 aimDirection = m_player.GetCamera().GetForward();
    const bool isMoving = m_player.GetHorizontalSpeed() > kMovingSpeedThreshold;

    float recoilPitch = 0.0f;
    float recoilYaw = 0.0f;
    const std::vector<HitscanRay> rays = weapon->TryFire(origin, aimDirection, isMoving, m_player.IsCrouching(), recoilPitch, recoilYaw);
    if (rays.empty()) return;

    m_player.ApplyRecoil(recoilPitch, recoilYaw);

    // Gunfire is loud enough to hear from most of this small map; bots
    // poll this each frame via BotPerception::FindAudibleSound.
    m_soundEvents.Push({origin, 1800.0f, SoundCategory::Gunshot});
    m_audio.PlaySound3D(m_soundBank.Get(Engine::SoundId::Gunshot), origin);

    std::vector<HitscanTarget> targets;
    CollectAllHitscanTargets(targets, /*firingSlot*/ 0);

    for (const HitscanRay& ray : rays) {
        const HitscanResult hit = ResolveHitscan(ray.origin, ray.direction, ray.maxDistance, m_collisionWorld.GetRawBrushes(), targets);

        glm::vec3 tracerEnd = ray.origin + ray.direction * ray.maxDistance;
        glm::vec3 tracerColor(1.0f, 0.85f, 0.3f);
        if (hit.hit) {
            tracerEnd = hit.point;
            if (hit.hitDamageable) {
                tracerColor = glm::vec3(1.0f, 0.2f, 0.2f);
                hit.hitDamageable->TakeDamage(weapon->ComputeDamage(hit.distance, hit.hitZone), hit.hitZone);
                // "Blood-free hit effects" per the design brief: a neutral
                // flash instead of anything resembling blood.
                m_particles.SpawnBurst(hit.point, glm::vec3(1.0f, 0.95f, 0.85f), 6, 60.0f, 160.0f, 0.25f, 2.0f, false);
            } else {
                PlayImpactEffect(hit.point, hit.normal, hit.material);
            }
        }
        m_debugLines.AddLine(ray.origin, tracerEnd, tracerColor, 0.05f);
    }
}

void GameApplication::PlayImpactEffect(const glm::vec3& point, const glm::vec3& normal, const std::string& material) {
    const Engine::SurfaceMaterial& surface = m_materials.Get(material);

    Engine::SoundId soundId = Engine::SoundId::ImpactConcrete;
    glm::vec3 particleColor = surface.color;
    int particleCount = 8;
    if (material == "metal") {
        soundId = Engine::SoundId::ImpactMetal;
        particleColor = glm::vec3(1.0f, 0.9f, 0.5f); // bright sparks, not the surface's own gray-blue tint
        particleCount = 12;
    } else if (material == "wood") {
        soundId = Engine::SoundId::ImpactWood;
        particleCount = 10;
    }

    m_audio.PlaySound3D(m_soundBank.Get(soundId), point);
    m_particles.SpawnBurst(point, particleColor, particleCount, 80.0f, 220.0f, 0.5f, 1.5f, true);
    m_decals.AddDecal(point, normal, surface.color * 0.4f, 6.0f, 25.0f);
}

void GameApplication::HandleGrenadeInput() {
    // Dedicated keys per type rather than a weapon-wheel "grenade slot" —
    // a real throw-select UX belongs to Milestone 8's UI work; this just
    // proves all four types genuinely work.
    struct Binding { int key; GrenadeKind kind; };
    static const Binding kBindings[] = {
        {GLFW_KEY_G, GrenadeKind::Fragmentation},
        {GLFW_KEY_H, GrenadeKind::Smoke},
        {GLFW_KEY_J, GrenadeKind::Flashbang},
        {GLFW_KEY_K, GrenadeKind::Decoy},
    };

    for (const Binding& binding : kBindings) {
        if (!m_input.WasKeyPressed(binding.key)) continue;

        const int kindIndex = static_cast<int>(binding.kind);
        if (m_grenadeCounts[kindIndex] <= 0) {
            std::printf("[Grenade] Out of %s\n", GetGrenadeData(binding.kind).name.c_str());
            std::fflush(stdout);
            continue;
        }
        m_grenadeCounts[kindIndex]--;

        Grenade grenade;
        const glm::vec3 origin = m_player.GetCamera().GetPosition();
        const glm::vec3 velocity = m_player.GetCamera().GetForward() * GetGrenadeData(binding.kind).throwSpeed;
        grenade.Init(origin, velocity, binding.kind);
        m_grenades.push_back(grenade);

        std::printf("[Grenade] Threw %s (%d remaining)\n", GetGrenadeData(binding.kind).name.c_str(), m_grenadeCounts[kindIndex]);
        std::fflush(stdout);
    }
}

void GameApplication::UpdateGrenades(float deltaTime) {
    for (size_t i = 0; i < m_grenades.size();) {
        if (m_grenades[i].Update(deltaTime, m_collisionWorld.GetRawBrushes())) {
            DetonateGrenade(m_grenades[i]);
            m_grenades.erase(m_grenades.begin() + static_cast<long>(i));
        } else {
            ++i;
        }
    }

    for (size_t i = 0; i < m_smokeClouds.size();) {
        m_smokeClouds[i].remainingSeconds -= deltaTime;
        if (m_smokeClouds[i].remainingSeconds <= 0.0f) {
            m_smokeClouds.erase(m_smokeClouds.begin() + static_cast<long>(i));
        } else {
            ++i;
        }
    }

    for (size_t i = 0; i < m_decoyEmitters.size();) {
        DecoyEmitter& emitter = m_decoyEmitters[i];
        emitter.remainingSeconds -= deltaTime;
        emitter.nextBeepTimer -= deltaTime;
        if (emitter.nextBeepTimer <= 0.0f) {
            emitter.nextBeepTimer = GetGrenadeData(GrenadeKind::Decoy).decoyBeepIntervalSeconds;
            m_soundEvents.Push({emitter.position, 1800.0f, SoundCategory::Gunshot});
            m_audio.PlaySound3D(m_soundBank.Get(Engine::SoundId::Gunshot), emitter.position);
        }
        if (emitter.remainingSeconds <= 0.0f) {
            m_decoyEmitters.erase(m_decoyEmitters.begin() + static_cast<long>(i));
        } else {
            ++i;
        }
    }
}

void GameApplication::DetonateGrenade(const Grenade& grenade) {
    const GrenadeData& data = grenade.GetData();
    const glm::vec3& position = grenade.GetPosition();

    switch (grenade.GetKind()) {
        case GrenadeKind::Fragmentation: {
            m_audio.PlaySound3D(m_soundBank.Get(Engine::SoundId::Explosion), position);
            m_particles.SpawnBurst(position, glm::vec3(1.0f, 0.55f, 0.15f), 24, 150.0f, 500.0f, 0.6f, 3.0f, true);
            m_decals.AddDecal(position, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.15f), 40.0f, 20.0f);

            // Anything alive within blast radius with line of sight takes
            // falloff damage — including the player if they're too close
            // to their own throw, matching real fragmentation grenades.
            auto applyBlastDamage = [&](const glm::vec3& targetPoint, Damageable& target) {
                if (!target.IsAlive()) return;
                const float distance = glm::length(targetPoint - position);
                if (distance > data.blastRadius) return;
                if (Engine::TraceMove(position, targetPoint, m_collisionWorld.GetRawBrushes()).hit) return; // blocked by a wall
                const float falloff = 1.0f - (distance / data.blastRadius);
                target.TakeDamage(data.maxDamage * falloff, HitZone::Body);
            };

            applyBlastDamage(m_player.GetEyePosition(), m_playerHealth);
            for (auto& bot : m_bots) {
                applyBlastDamage(bot->GetController().GetEyePosition(), bot->GetHealth());
            }
            break;
        }
        case GrenadeKind::Smoke: {
            m_audio.PlaySound3D(m_soundBank.Get(Engine::SoundId::ImpactConcrete), position);
            m_particles.SpawnBurst(position, glm::vec3(0.75f), 30, 20.0f, 60.0f, data.smokeDurationSeconds, 6.0f, false);
            m_smokeClouds.push_back({position, data.smokeRadius, data.smokeDurationSeconds});
            break;
        }
        case GrenadeKind::Flashbang: {
            m_audio.PlaySound3D(m_soundBank.Get(Engine::SoundId::Explosion), position);
            m_particles.SpawnBurst(position, glm::vec3(1.0f), 20, 100.0f, 300.0f, 0.3f, 2.5f, false);

            auto tryBlind = [&](const glm::vec3& eyePos, const glm::vec3& forward, bool isPlayer, Bot* bot) {
                const glm::vec3 toFlash = position - eyePos;
                const float distance = glm::length(toFlash);
                if (distance > data.flashRadius) return;
                if (Engine::TraceMove(eyePos, position, m_collisionWorld.GetRawBrushes()).hit) return; // blocked by a wall

                const float distanceFactor = 1.0f - (distance / data.flashRadius);
                const float angleFactor = std::clamp(glm::dot(glm::normalize(forward), glm::normalize(toFlash)) * 0.5f + 0.5f, 0.2f, 1.0f);
                const float blindSeconds = data.maxBlindDurationSeconds * distanceFactor * angleFactor;
                if (blindSeconds <= 0.05f) return;

                if (isPlayer) {
                    m_playerBlindRemaining = std::max(m_playerBlindRemaining, blindSeconds);
                    m_playerBlindMax = std::max(m_playerBlindMax, blindSeconds);
                    m_audio.PlaySound3D(m_soundBank.Get(Engine::SoundId::FlashRing), eyePos, 1.0f);
                } else if (bot) {
                    bot->ApplyBlind(blindSeconds);
                }
            };

            tryBlind(m_player.GetEyePosition(), m_player.GetCamera().GetForward(), true, nullptr);
            for (auto& bot : m_bots) {
                tryBlind(bot->GetController().GetEyePosition(), bot->GetController().GetCamera().GetForward(), false, bot.get());
            }
            break;
        }
        case GrenadeKind::Decoy: {
            m_audio.PlaySound3D(m_soundBank.Get(Engine::SoundId::ImpactConcrete), position);
            m_decoyEmitters.push_back({position, data.decoyDurationSeconds, 0.0f});
            break;
        }
    }
}

void GameApplication::BuildPlayerHitboxTargets(std::vector<HitscanTarget>& outTargets) {
    if (!m_playerHealth.IsAlive()) return;

    // The same three-sphere approximation TargetDummy uses, but tracking
    // the player's live (possibly crouched) feet position every frame
    // instead of a fixed spawn point.
    const glm::vec3 feet = m_player.GetFeetPosition();
    constexpr float kLegsRadius = 16.0f;
    constexpr float kBodyRadius = 18.0f;
    constexpr float kHeadRadius = 12.0f;
    outTargets.push_back({feet + glm::vec3(0.0f, kLegsRadius, 0.0f), kLegsRadius, HitZone::Legs, &m_playerHealth});
    outTargets.push_back({feet + glm::vec3(0.0f, kLegsRadius * 2.0f + kBodyRadius, 0.0f), kBodyRadius, HitZone::Body, &m_playerHealth});
    outTargets.push_back({feet + glm::vec3(0.0f, kLegsRadius * 2.0f + kBodyRadius * 2.0f + kHeadRadius, 0.0f), kHeadRadius, HitZone::Head, &m_playerHealth});
}

void GameApplication::CollectAllHitscanTargets(std::vector<HitscanTarget>& outTargets, int firingSlot) {
    for (TargetDummy* dummy : m_targetDummies) {
        dummy->CollectHitscanTargets(outTargets);
    }
    for (const auto& bot : m_bots) {
        if (!bot->GetHealth().IsAlive()) continue;
        const glm::vec3 feet = bot->GetController().GetFeetPosition();
        outTargets.push_back({feet + glm::vec3(0.0f, 16.0f, 0.0f), 16.0f, HitZone::Legs, &bot->GetHealth()});
        outTargets.push_back({feet + glm::vec3(0.0f, 50.0f, 0.0f), 18.0f, HitZone::Body, &bot->GetHealth()});
        outTargets.push_back({feet + glm::vec3(0.0f, 80.0f, 0.0f), 12.0f, HitZone::Head, &bot->GetHealth()});
    }

    if (firingSlot != 0) {
        BuildPlayerHitboxTargets(outTargets); // the local (host) player, slot 0
    }

    if (m_network.GetRole() == NetRole::Host) {
        for (int slot = 1; slot < kMaxNetPlayers; ++slot) {
            if (slot == firingSlot || !m_hostRemotePlayers[slot].active) continue;
            HostRemotePlayer& remote = m_hostRemotePlayers[slot];
            if (!remote.health.IsAlive()) continue;
            const glm::vec3 feet = remote.controller.GetFeetPosition();
            outTargets.push_back({feet + glm::vec3(0.0f, 16.0f, 0.0f), 16.0f, HitZone::Legs, &remote.health});
            outTargets.push_back({feet + glm::vec3(0.0f, 50.0f, 0.0f), 18.0f, HitZone::Body, &remote.health});
            outTargets.push_back({feet + glm::vec3(0.0f, 80.0f, 0.0f), 12.0f, HitZone::Head, &remote.health});
        }
    }
}

void GameApplication::UpdateBots(float deltaTime) {
    // Freeze time: real tactical shooters don't let combat happen during
    // the buy/round-end phases, and neither should bots — otherwise a bot
    // can wander into the player's room and get free kills before the
    // round (and the player's chance to react) has even started.
    if (m_roundManager.GetPhase() != RoundPhase::Active) return;

    std::vector<HitscanTarget> playerTargets;
    BuildPlayerHitboxTargets(playerTargets);

    const glm::vec3 playerEyePosition = m_player.GetCamera().GetPosition();
    const bool playerAlive = m_playerHealth.IsAlive();
    const TeamId playerTeam = m_roundManager.GetPlayerTeam();

    for (auto& bot : m_bots) {
        bot->Update(deltaTime, playerEyePosition, playerAlive, playerTeam, m_collisionWorld.GetRawBrushes(), m_soundEvents, playerTargets, &m_debugLines, m_smokeClouds);
    }
}

void GameApplication::ProcessInput(float deltaTime) {
    // Escape is the sole owner of this key (see PauseMenu::Update's comment
    // on why it doesn't also read it): closes the Buy menu if that's open,
    // otherwise toggles pause.
    if (m_input.WasKeyPressed(GLFW_KEY_ESCAPE)) {
        if (m_buyMenu.IsOpen()) {
            m_buyMenu.Close();
            m_input.SetCursorLocked(true);
        } else {
            m_paused = !m_paused;
            m_input.SetCursorLocked(!m_paused);
        }
    }

    if (m_paused) {
        bool shouldResume = false;
        const bool shouldQuit = m_pauseMenu.Update(m_input, shouldResume);
        if (shouldQuit) {
            glfwSetWindowShouldClose(m_window.GetHandle(), GLFW_TRUE);
        } else if (shouldResume) {
            m_paused = false;
            m_input.SetCursorLocked(true);
        }
        return; // simulation fully frozen while paused
    }

    // A networked client is a thin presentation layer over the host's
    // authoritative simulation (see NetProtocol.h) — it has an entirely
    // separate, much simpler per-frame flow (send input, receive
    // snapshot, apply it) rather than running any of the offline/host
    // gameplay logic below.
    if (m_network.GetRole() == NetRole::Client) {
        ProcessInputClient(deltaTime);
        return;
    }

    // The Buy menu only makes sense during the Buy phase; auto-close it if
    // the phase moves on while it's open (e.g. the 20-second buy window
    // simply expired with the menu still up).
    if (m_roundManager.GetPhase() != RoundPhase::Buy && m_buyMenu.IsOpen()) {
        m_buyMenu.Close();
        m_input.SetCursorLocked(true);
    }
    if (m_roundManager.GetPhase() == RoundPhase::Buy && m_input.WasKeyPressed(GLFW_KEY_B)) {
        if (m_buyMenu.IsOpen()) {
            m_buyMenu.Close();
            m_input.SetCursorLocked(true);
        } else {
            m_buyMenu.Open();
            m_input.SetCursorLocked(false);
        }
    }
    if (m_buyMenu.IsOpen()) {
        m_buyMenu.Update(deltaTime, m_input, m_playerWallet, m_weaponInventory, m_playerHealth, m_grenadeCounts);
    }

    if (m_input.IsCursorLocked()) {
        m_player.ApplyMouseLook(m_input.GetMouseDeltaX(), m_input.GetMouseDeltaY());
    }

    float wishForward = 0.0f;
    float wishRight = 0.0f;
    if (m_input.IsKeyDown(GLFW_KEY_W)) wishForward += 1.0f;
    if (m_input.IsKeyDown(GLFW_KEY_S)) wishForward -= 1.0f;
    if (m_input.IsKeyDown(GLFW_KEY_D)) wishRight += 1.0f;
    if (m_input.IsKeyDown(GLFW_KEY_A)) wishRight -= 1.0f;

    const bool jumpHeld = m_input.IsKeyDown(GLFW_KEY_SPACE);
    const bool crouchHeld = m_input.IsKeyDown(GLFW_KEY_LEFT_CONTROL);
    const bool walkHeld = m_input.IsKeyDown(GLFW_KEY_LEFT_SHIFT);

    m_player.Update(deltaTime, wishForward, wishRight, jumpHeld, crouchHeld, walkHeld);

    // Running is the loud movement option (Phase 2/12's design): periodic
    // footstep noise bots can hear, distinct from the much louder gunshot.
    if (m_player.IsGrounded() && m_player.GetMovementSpeed() == MovementSpeed::Run && m_player.GetHorizontalSpeed() > kMovingSpeedThreshold) {
        m_footstepSoundTimer -= deltaTime;
        if (m_footstepSoundTimer <= 0.0f) {
            const glm::vec3 feet = m_player.GetFeetPosition();
            m_soundEvents.Push({feet, 700.0f, SoundCategory::Footstep});
            m_footstepSoundTimer = kFootstepIntervalSeconds;

            // A quick trace to find what's underfoot, so concrete/metal/
            // wood floors sound distinct — reuses the same material data
            // TraceResult already carries for bullet impacts.
            const Engine::TraceResult groundTrace = Engine::TraceMove(feet + glm::vec3(0.0f, 4.0f, 0.0f), feet - glm::vec3(0.0f, 4.0f, 0.0f), m_collisionWorld.GetRawBrushes());
            Engine::SoundId footstepSound = Engine::SoundId::FootstepConcrete;
            if (groundTrace.material == "metal") footstepSound = Engine::SoundId::FootstepMetal;
            else if (groundTrace.material == "wood") footstepSound = Engine::SoundId::FootstepWood;
            m_audio.PlaySound3D(m_soundBank.Get(footstepSound), feet, 0.6f);
        }
    } else {
        m_footstepSoundTimer = 0.0f;
    }

    if (m_playerBlindRemaining > 0.0f) {
        m_playerBlindRemaining = std::max(0.0f, m_playerBlindRemaining - deltaTime);
    }

    // TEMP (test-only): verifies defuse, which otherwise has no way to be
    // exercised solo (only Guardian can defuse, and Guardian has no members
    // until Milestone 6's bots exist). Remove once bots can fill that role.
    if (m_input.WasKeyPressed(GLFW_KEY_T)) {
        const TeamId newTeam = m_roundManager.GetPlayerTeam() == TeamId::Assault ? TeamId::Guardian : TeamId::Assault;
        m_roundManager.DebugSetPlayerTeam(newTeam);
        std::printf("[DEBUG] Player team switched to %s\n", GetTeamName(newTeam));
        std::fflush(stdout);
    }

    // Milestone 9 debugging tools: F1/F2/F3 toggle visualization overlays,
    // F4/F5 toggle movement/damage cheats for testing.
    if (m_input.WasKeyPressed(GLFW_KEY_F1)) {
        m_debugShowNav = !m_debugShowNav;
        std::printf("[DEBUG] Nav graph visualization %s\n", m_debugShowNav ? "ON" : "OFF");
        std::fflush(stdout);
    }
    if (m_input.WasKeyPressed(GLFW_KEY_F2)) {
        m_debugShowCollision = !m_debugShowCollision;
        std::printf("[DEBUG] Collision wireframe %s\n", m_debugShowCollision ? "ON" : "OFF");
        std::fflush(stdout);
    }
    if (m_input.WasKeyPressed(GLFW_KEY_F3)) {
        m_debugShowStats = !m_debugShowStats;
        std::printf("[DEBUG] Stats panel %s\n", m_debugShowStats ? "ON" : "OFF");
        std::fflush(stdout);
    }
    if (m_input.WasKeyPressed(GLFW_KEY_F4)) {
        m_player.SetNoclip(!m_player.IsNoclip());
        std::printf("[DEBUG] Noclip %s\n", m_player.IsNoclip() ? "ON" : "OFF");
        std::fflush(stdout);
    }
    if (m_input.WasKeyPressed(GLFW_KEY_F5)) {
        m_playerHealth.SetGodMode(!m_playerHealth.IsGodMode());
        std::printf("[DEBUG] God mode %s\n", m_playerHealth.IsGodMode() ? "ON" : "OFF");
        std::fflush(stdout);
    }

    // Weapon-select/reload/grenade number-and-letter keys are the Buy
    // menu's own hotkeys (1-8) while it's open, so gameplay must not also
    // react to them — otherwise buying item [2] would simultaneously swap
    // to weapon slot 2.
    if (!m_buyMenu.IsOpen()) {
        if (m_input.WasKeyPressed(GLFW_KEY_1)) m_weaponInventory.SwitchTo(0);
        if (m_input.WasKeyPressed(GLFW_KEY_2)) m_weaponInventory.SwitchTo(1);
        if (m_input.WasKeyPressed(GLFW_KEY_3)) m_weaponInventory.SwitchTo(2);
        if (m_input.WasKeyPressed(GLFW_KEY_4)) m_weaponInventory.SwitchTo(3);
        if (m_input.WasKeyPressed(GLFW_KEY_R)) {
            if (Weapon* weapon = m_weaponInventory.GetCurrent()) {
                const bool wasReloading = weapon->IsReloading();
                weapon->StartReload();
                if (!wasReloading && weapon->IsReloading()) {
                    m_audio.PlaySound3D(m_soundBank.Get(Engine::SoundId::ReloadClick), m_player.GetCamera().GetPosition(), 0.5f);
                }
            }
        }

        HandleGrenadeInput();
    }

    const bool interactHeld = m_input.IsKeyDown(GLFW_KEY_E);
    m_roundManager.Update(deltaTime, m_player.GetCamera().GetPosition(), interactHeld);

    // Grenades are consumed on throw and, like real tactical-shooter
    // economies, do NOT carry over — they must be rebought every round
    // (unlike the Carbine/Street Sweeper, which are one-time purchases kept
    // for the rest of the match). Detected as a Buy-phase-entry transition
    // since RoundManager has no grenade-specific hook to call into. Must be
    // checked here, after RoundManager::Update() above (which is what
    // actually flips the phase) and before m_previousRoundPhase is
    // overwritten below — checking beforehand compared this frame's
    // not-yet-updated phase against last frame's already-updated snapshot,
    // which are always equal, so the reset would never fire.
    if (m_previousRoundPhase != RoundPhase::Buy && m_roundManager.GetPhase() == RoundPhase::Buy) {
        for (int& count : m_grenadeCounts) count = 0;
    }

    // Plant/defuse beeps: detected as a charge-state transition, since
    // RoundManager doesn't (and shouldn't) know about AudioSystem.
    const ChargeState chargeState = m_roundManager.GetCharge().GetState();
    if (m_previousChargeState == ChargeState::Carried && chargeState == ChargeState::Planted) {
        m_audio.PlaySound3D(m_soundBank.Get(Engine::SoundId::PlantBeep), m_roundManager.GetCharge().GetPosition());
    } else if (m_previousChargeState == ChargeState::Planted && chargeState == ChargeState::Defused) {
        m_audio.PlaySound3D(m_soundBank.Get(Engine::SoundId::DefuseBeep), m_roundManager.GetCharge().GetPosition());
    }
    m_previousChargeState = chargeState;
    m_previousRoundPhase = m_roundManager.GetPhase();

    if (!m_buyMenu.IsOpen()) {
        HandleFireInput();
    }

    if (Weapon* weapon = m_weaponInventory.GetCurrent()) {
        const float recoilRecoveryDelta = weapon->Update(deltaTime);
        if (recoilRecoveryDelta != 0.0f) {
            m_player.ApplyRecoil(recoilRecoveryDelta, 0.0f);
        }
    }

    for (TargetDummy* dummy : m_targetDummies) {
        dummy->Update(deltaTime);
    }

    UpdateGrenades(deltaTime);

    // Death tone: detected as an alive->dead transition on either side.
    if (m_wasPlayerAlive && !m_playerHealth.IsAlive()) {
        m_audio.PlaySound3D(m_soundBank.Get(Engine::SoundId::DeathTone), m_player.GetCamera().GetPosition());
    }
    m_wasPlayerAlive = m_playerHealth.IsAlive();

    UpdateBots(deltaTime);
    m_soundEvents.Clear(); // one-frame mailbox: cleared after bots have had a chance to hear this frame's events

    m_debugLines.Update(deltaTime);
    m_particles.Update(deltaTime);
    m_decals.Update(deltaTime);

    m_audio.SetListener(m_player.GetCamera().GetPosition(), m_player.GetCamera().GetForward(), m_player.GetCamera().GetUp());
    m_audio.Update();

    if (m_network.GetRole() == NetRole::Host) {
        ProcessInputHost(deltaTime);
    }
}

void GameApplication::ProcessInputHost(float deltaTime) {
    std::vector<std::pair<int, ClientInputPacket>> newInputs;
    std::vector<int> joined, left;
    m_network.HostPoll(newInputs, joined, left);

    for (int slot : joined) {
        HostRemotePlayer& remote = m_hostRemotePlayers[slot];
        remote.active = true;
        remote.team = TeamId::Assault; // remote clients join the host's side this milestone — see the Milestone 10 write-up's scope notes
        remote.health.Reset();
        remote.weapon = std::make_unique<Weapon>();
        remote.weapon->Init(CreateCarbineData());

        glm::vec3 spawnPos(0.0f);
        float spawnYaw = 0.0f;
        for (const Engine::SpawnPointEntity* spawn : m_level.GetSpawnPoints()) {
            if (spawn->GetTeam() == "assault") {
                spawnPos = spawn->GetOrigin();
                spawnYaw = spawn->GetYawDegrees();
                break;
            }
        }
        remote.controller.Init(spawnPos, spawnYaw, &m_collisionWorld);
    }
    for (int slot : left) {
        m_hostRemotePlayers[slot] = HostRemotePlayer{};
    }
    for (const std::pair<int, ClientInputPacket>& entry : newInputs) {
        if (m_hostRemotePlayers[entry.first].active) {
            m_hostRemotePlayers[entry.first].lastInput = entry.second;
        }
    }

    for (int slot = 1; slot < kMaxNetPlayers; ++slot) {
        HostRemotePlayer& remote = m_hostRemotePlayers[slot];
        if (!remote.active) continue;

        const ClientInputPacket& input = remote.lastInput;
        remote.controller.SetViewAngles(input.yawDegrees, input.pitchDegrees);
        remote.controller.Update(deltaTime, input.wishForward, input.wishRight,
            (input.buttons & kButtonJump) != 0, (input.buttons & kButtonCrouch) != 0, (input.buttons & kButtonWalk) != 0);

        if (remote.weapon) {
            if ((input.buttons & kButtonReload) != 0) {
                remote.weapon->StartReload();
            }
            remote.weapon->Update(deltaTime);
        }

        if (remote.health.IsAlive() && remote.weapon && (input.buttons & kButtonFire) != 0) {
            const glm::vec3 origin = remote.controller.GetEyePosition();
            const glm::vec3 aimDirection = remote.controller.GetCamera().GetForward();
            const bool isMoving = remote.controller.GetHorizontalSpeed() > kMovingSpeedThreshold;
            float recoilPitch = 0.0f, recoilYaw = 0.0f;
            const std::vector<HitscanRay> rays = remote.weapon->TryFire(origin, aimDirection, isMoving, remote.controller.IsCrouching(), recoilPitch, recoilYaw);
            if (!rays.empty()) {
                m_soundEvents.Push({origin, 1800.0f, SoundCategory::Gunshot});
                m_audio.PlaySound3D(m_soundBank.Get(Engine::SoundId::Gunshot), origin);

                std::vector<HitscanTarget> targets;
                CollectAllHitscanTargets(targets, slot);
                for (const HitscanRay& ray : rays) {
                    const HitscanResult hit = ResolveHitscan(ray.origin, ray.direction, ray.maxDistance, m_collisionWorld.GetRawBrushes(), targets);
                    glm::vec3 tracerEnd = ray.origin + ray.direction * ray.maxDistance;
                    if (hit.hit) {
                        tracerEnd = hit.point;
                        if (hit.hitDamageable) {
                            hit.hitDamageable->TakeDamage(remote.weapon->ComputeDamage(hit.distance, hit.hitZone), hit.hitZone);
                            m_particles.SpawnBurst(hit.point, glm::vec3(1.0f, 0.95f, 0.85f), 6, 60.0f, 160.0f, 0.25f, 2.0f, false);
                        } else {
                            PlayImpactEffect(hit.point, hit.normal, hit.material);
                        }
                    }
                    m_debugLines.AddLine(ray.origin, tracerEnd, glm::vec3(1.0f, 0.6f, 0.15f), 0.05f);
                }
            }
        }
    }

    // Broadcast every frame — simplest correct choice for a LAN/localhost
    // demo. A shipping game would throttle this well below render
    // framerate (e.g. 20-30Hz) to control bandwidth; nothing about the
    // protocol or this loop would need to change to add that later.
    ServerSnapshotPacket snapshot;
    snapshot.roundPhase = static_cast<uint8_t>(m_roundManager.GetPhase());
    snapshot.phaseTimeRemaining = m_roundManager.GetPhaseTimeRemaining();
    snapshot.assaultScore = static_cast<uint8_t>(m_roundManager.GetAssaultScore());
    snapshot.guardianScore = static_cast<uint8_t>(m_roundManager.GetGuardianScore());

    {
        PlayerSnapshot& p = snapshot.players[0];
        p.occupied = 1;
        p.team = static_cast<uint8_t>(m_roundManager.GetPlayerTeam());
        p.alive = m_playerHealth.IsAlive() ? 1 : 0;
        const glm::vec3 eye = m_player.GetEyePosition();
        p.posX = eye.x; p.posY = eye.y; p.posZ = eye.z;
        p.yawDegrees = m_player.GetCamera().GetYaw();
        p.pitchDegrees = m_player.GetCamera().GetPitch();
        p.health = m_playerHealth.GetHealth();
        p.armor = m_playerHealth.GetArmor();
        if (const Weapon* weapon = m_weaponInventory.GetCurrent()) {
            p.magazineAmmo = weapon->GetMagazineAmmo();
            p.reserveAmmo = weapon->GetReserveAmmo();
            std::snprintf(p.weaponName, sizeof(p.weaponName), "%s", weapon->GetData().name.c_str());
        }
    }
    for (int slot = 1; slot < kMaxNetPlayers; ++slot) {
        const HostRemotePlayer& remote = m_hostRemotePlayers[slot];
        PlayerSnapshot& p = snapshot.players[slot];
        if (!remote.active) {
            p.occupied = 0;
            continue;
        }
        p.occupied = 1;
        p.team = static_cast<uint8_t>(remote.team);
        p.alive = remote.health.IsAlive() ? 1 : 0;
        const glm::vec3 eye = remote.controller.GetEyePosition();
        p.posX = eye.x; p.posY = eye.y; p.posZ = eye.z;
        p.yawDegrees = remote.controller.GetCamera().GetYaw();
        p.pitchDegrees = remote.controller.GetCamera().GetPitch();
        p.health = remote.health.GetHealth();
        p.armor = remote.health.GetArmor();
        if (remote.weapon) {
            p.magazineAmmo = remote.weapon->GetMagazineAmmo();
            p.reserveAmmo = remote.weapon->GetReserveAmmo();
            std::snprintf(p.weaponName, sizeof(p.weaponName), "%s", remote.weapon->GetData().name.c_str());
        }
    }

    m_network.HostBroadcastSnapshot(snapshot);
}

void GameApplication::ProcessInputClient(float deltaTime) {
    (void)deltaTime;

    if (m_input.IsCursorLocked()) {
        m_player.ApplyMouseLook(m_input.GetMouseDeltaX(), m_input.GetMouseDeltaY());
    }

    float wishForward = 0.0f, wishRight = 0.0f;
    if (m_input.IsKeyDown(GLFW_KEY_W)) wishForward += 1.0f;
    if (m_input.IsKeyDown(GLFW_KEY_S)) wishForward -= 1.0f;
    if (m_input.IsKeyDown(GLFW_KEY_D)) wishRight += 1.0f;
    if (m_input.IsKeyDown(GLFW_KEY_A)) wishRight -= 1.0f;

    ClientInputPacket input;
    input.sequence = ++m_netInputSequence;
    input.wishForward = wishForward;
    input.wishRight = wishRight;
    input.yawDegrees = m_player.GetCamera().GetYaw();
    input.pitchDegrees = m_player.GetCamera().GetPitch();
    uint8_t buttons = 0;
    if (m_input.IsKeyDown(GLFW_KEY_SPACE)) buttons |= kButtonJump;
    if (m_input.IsKeyDown(GLFW_KEY_LEFT_CONTROL)) buttons |= kButtonCrouch;
    if (m_input.IsKeyDown(GLFW_KEY_LEFT_SHIFT)) buttons |= kButtonWalk;
    if (m_input.IsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) buttons |= kButtonFire;
    if (m_input.WasKeyPressed(GLFW_KEY_R)) buttons |= kButtonReload;
    input.buttons = buttons;
    m_network.ClientSendInput(input);

    ServerSnapshotPacket snapshot;
    if (m_network.ClientPoll(snapshot)) {
        m_lastClientSnapshot = snapshot;
        m_hasClientSnapshot = true;
    }

    if (!m_hasClientSnapshot || !m_network.IsConnectedToHost()) return;

    const int localSlot = m_network.GetLocalSlot();
    if (localSlot < 0 || localSlot >= kMaxNetPlayers) return;

    const PlayerSnapshot& self = m_lastClientSnapshot.players[localSlot];
    if (self.occupied) {
        // Position is server-authoritative (no local prediction — see
        // NetProtocol.h); rotation was already applied above from local
        // mouse input for immediate response, so it isn't overwritten here.
        m_player.SetPositionDirect(glm::vec3(self.posX, self.posY, self.posZ));
        m_playerHealth.SetHealthDirect(self.health);
        m_playerHealth.SetArmor(self.armor);
    }

    for (int slot = 0; slot < kMaxNetPlayers; ++slot) {
        if (slot == localSlot) continue;
        const PlayerSnapshot& p = m_lastClientSnapshot.players[slot];
        ClientRemoteAvatarView& view = m_clientRemoteAvatars[slot];
        view.active = p.occupied != 0;
        if (view.active) {
            view.position = glm::vec3(p.posX, p.posY, p.posZ);
            view.yawDegrees = p.yawDegrees;
            view.team = static_cast<TeamId>(p.team);
            view.alive = p.alive != 0;
        }
    }
}

void GameApplication::Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const glm::mat4 view = m_player.GetCamera().GetViewMatrix();
    const glm::mat4 projection = m_player.GetCamera().GetProjectionMatrix(m_window.GetAspectRatio());

    m_shader.Use();
    m_shader.SetMat4("uView", view);
    m_shader.SetMat4("uProjection", projection);

    // A single fixed "sun" — dynamic/movable lights are later Phase 16 work.
    static const glm::vec3 kLightDirection = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f));
    m_shader.SetVec3("uLightDirection", kLightDirection);
    m_shader.SetVec3("uLightColor", glm::vec3(1.0f, 0.98f, 0.92f));
    m_shader.SetFloat("uAmbientStrength", 0.35f);

    for (const auto& obj : m_sceneObjects) {
        m_shader.SetMat4("uModel", obj.transform);
        obj.mesh->Draw();
    }

    // Simple placeholder bodies (no character models/animation system yet)
    // for anything shootable so it's actually visible to aim at.
    for (const TargetDummy* dummy : m_targetDummies) {
        if (!dummy->IsAlive()) continue;
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), dummy->GetOrigin() + glm::vec3(0.0f, 46.0f, 0.0f));
        transform = glm::scale(transform, glm::vec3(32.0f, 92.0f, 32.0f));
        m_shader.SetMat4("uModel", transform);
        m_dummyBodyMesh.Draw();
    }
    for (const auto& bot : m_bots) {
        if (!bot->GetHealth().IsAlive()) continue;
        const glm::vec3 feet = bot->GetController().GetFeetPosition();
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), feet + glm::vec3(0.0f, 36.0f, 0.0f));
        transform = glm::scale(transform, glm::vec3(32.0f, 72.0f, 32.0f));
        m_shader.SetMat4("uModel", transform);
        m_botBodyMesh.Draw();
    }
    RenderRemoteAvatars();

    RenderDebugVisualization();
    m_debugLines.Render(view, projection);
    m_decals.Render(view, projection);
    m_particles.Render(view, projection);

    // Everything from here on is 2D screen-space overlay content, drawn
    // with depth testing off in one contiguous block (rather than each
    // piece toggling it individually) so the flashbang whiteout, HUD, and
    // any open menu correctly layer over the 3D scene and over each other
    // in draw order. Culling is also off here: the ortho projection used
    // for this content puts (0,0) at the top-left with y growing downward
    // (matching cursor coordinates), which flips the effective winding
    // order of every quad versus the 3D scene's CCW convention — with
    // GL_CULL_FACE left on, every 2D rect and glyph quad was silently
    // being discarded as back-facing.
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    RenderScreenOverlay();
    RenderHud();
    if (m_paused) {
        m_pauseMenu.Render(m_textRenderer, m_uiRenderer, m_font, m_window.GetWidth(), m_window.GetHeight());
    } else if (m_buyMenu.IsOpen()) {
        m_buyMenu.Render(m_textRenderer, m_uiRenderer, m_font, m_window.GetWidth(), m_window.GetHeight(),
            m_playerWallet, m_weaponInventory, m_playerHealth, m_grenadeCounts);
    }
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

void GameApplication::RenderRemoteAvatars() {
    // m_shader is already bound with uView/uProjection set by the caller
    // (Render(), immediately before this) — same convention the bot/dummy
    // drawing loops just above this call rely on.
    constexpr glm::vec3 kBodyScale(32.0f, 72.0f, 32.0f);
    constexpr float kEyeToBodyCenterOffset = 36.0f; // matches the bot body drawing convention: body center sits this far above feet

    if (m_network.GetRole() == NetRole::Host) {
        for (int slot = 1; slot < kMaxNetPlayers; ++slot) {
            const HostRemotePlayer& remote = m_hostRemotePlayers[slot];
            if (!remote.active || !remote.health.IsAlive()) continue;
            const glm::vec3 feet = remote.controller.GetFeetPosition();
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), feet + glm::vec3(0.0f, kEyeToBodyCenterOffset, 0.0f));
            transform = glm::scale(transform, kBodyScale);
            m_shader.SetMat4("uModel", transform);
            m_remotePlayerBodyMesh.Draw();
        }
    } else if (m_network.GetRole() == NetRole::Client) {
        for (int slot = 0; slot < kMaxNetPlayers; ++slot) {
            const ClientRemoteAvatarView& avatar = m_clientRemoteAvatars[slot];
            if (!avatar.active || !avatar.alive) continue;
            // avatar.position is the eye position (see NetProtocol.h); an
            // approximate feet position (assuming standing height) is
            // enough for a cosmetic body box.
            const glm::vec3 approxFeet = avatar.position - glm::vec3(0.0f, 64.0f, 0.0f);
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), approxFeet + glm::vec3(0.0f, kEyeToBodyCenterOffset, 0.0f));
            transform = glm::scale(transform, kBodyScale);
            m_shader.SetMat4("uModel", transform);
            m_remotePlayerBodyMesh.Draw();
        }
    }
}

void GameApplication::RenderScreenOverlay() {
    if (m_playerBlindRemaining <= 0.0f) return;

    // Fades out over the blind duration rather than cutting off abruptly.
    const float alpha = std::clamp(m_playerBlindRemaining / std::max(m_playerBlindMax, 0.01f), 0.0f, 1.0f);

    m_overlayShader.Use();
    m_overlayShader.SetVec3("uColor", glm::vec3(1.0f));
    m_overlayShader.SetFloat("uAlpha", alpha);
    glBindVertexArray(m_overlayVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void GameApplication::RenderHud() {
    m_textRenderer.BeginFrame(m_window.GetWidth(), m_window.GetHeight());
    m_uiRenderer.BeginFrame(m_window.GetWidth(), m_window.GetHeight());

    if (m_network.GetRole() == NetRole::Client) {
        // A client has no local Wallet/RoundManager/WeaponInventory worth
        // displaying (see NetProtocol.h) — everything shown here comes
        // straight from the last received snapshot instead of the normal
        // Hud::Render path below, which is wired to those local objects.
        const float w = static_cast<float>(m_window.GetWidth());
        const float h = static_cast<float>(m_window.GetHeight());
        constexpr float kGap = 4.0f, kLen = 8.0f, kThick = 2.0f;
        m_uiRenderer.DrawRect(w * 0.5f - kGap - kLen, h * 0.5f - kThick * 0.5f, kLen, kThick, glm::vec3(1.0f), 0.85f);
        m_uiRenderer.DrawRect(w * 0.5f + kGap, h * 0.5f - kThick * 0.5f, kLen, kThick, glm::vec3(1.0f), 0.85f);
        m_uiRenderer.DrawRect(w * 0.5f - kThick * 0.5f, h * 0.5f - kGap - kLen, kThick, kLen, glm::vec3(1.0f), 0.85f);
        m_uiRenderer.DrawRect(w * 0.5f - kThick * 0.5f, h * 0.5f + kGap, kThick, kLen, glm::vec3(1.0f), 0.85f);

        char status[128];
        if (!m_network.IsConnectedToHost()) {
            std::snprintf(status, sizeof(status), "Connecting to host...");
        } else {
            const int slot = m_network.GetLocalSlot();
            const PlayerSnapshot self = (slot >= 0 && slot < kMaxNetPlayers) ? m_lastClientSnapshot.players[slot] : PlayerSnapshot{};
            const char* phaseLabel =
                m_lastClientSnapshot.roundPhase == static_cast<uint8_t>(RoundPhase::Buy) ? "BUY" :
                m_lastClientSnapshot.roundPhase == static_cast<uint8_t>(RoundPhase::Active) ? "ACTIVE" : "ROUND OVER";
            std::snprintf(status, sizeof(status), "Slot %d - %s %.0fs - A%d-G%d - HP %.0f AR %.0f - %s %d/%d",
                slot, phaseLabel, m_lastClientSnapshot.phaseTimeRemaining,
                m_lastClientSnapshot.assaultScore, m_lastClientSnapshot.guardianScore,
                self.health, self.armor, self.weaponName, self.magazineAmmo, self.reserveAmmo);
        }
        m_textRenderer.Draw(m_font, status, 20.0f, 20.0f, glm::vec3(1.0f, 0.9f, 0.5f));
        return;
    }

    int botsAlive = 0;
    for (const auto& bot : m_bots) {
        if (bot->GetHealth().IsAlive()) ++botsAlive;
    }

    HudDrawInfo info;
    info.screenWidth = m_window.GetWidth();
    info.screenHeight = m_window.GetHeight();
    info.health = &m_playerHealth;
    info.wallet = &m_playerWallet;
    info.inventory = &m_weaponInventory;
    info.roundManager = &m_roundManager;
    info.botsAlive = botsAlive;
    info.botsTotal = static_cast<int>(m_bots.size());
    info.grenadeCounts = m_grenadeCounts;
    info.fps = m_time.GetFPS();
    Hud::Render(m_textRenderer, m_uiRenderer, m_font, info);

    if (m_debugShowStats) {
        const glm::vec3 feet = m_player.GetFeetPosition();
        char lines[8][96];
        int lineCount = 0;
        std::snprintf(lines[lineCount++], 96, "F3 STATS  (F1 nav  F2 collision  F4 noclip  F5 god)");
        std::snprintf(lines[lineCount++], 96, "frame %.2fms (%.0f fps)", m_time.GetDeltaTime() * 1000.0f, m_time.GetFPS());
        std::snprintf(lines[lineCount++], 96, "pos %.0f %.0f %.0f", feet.x, feet.y, feet.z);
        std::snprintf(lines[lineCount++], 96, "speed %.0f u/s  %s%s", m_player.GetHorizontalSpeed(),
            m_player.IsGrounded() ? "grounded" : "airborne", m_player.IsNoclip() ? " noclip" : "");
        std::snprintf(lines[lineCount++], 96, "brushes %zu  waypoints %zu  bots %d/%d",
            m_level.worldBrushes.size(), m_navGraph.GetWaypointCount(), botsAlive, static_cast<int>(m_bots.size()));
        std::snprintf(lines[lineCount++], 96, "particles/decals/tracers active");
        std::snprintf(lines[lineCount++], 96, "god mode %s", m_playerHealth.IsGodMode() ? "ON" : "off");

        constexpr float kPanelX = 20.0f;
        const float panelY = 60.0f;
        const float lineHeight = m_font.GetPixelHeight() + 4.0f;
        m_uiRenderer.DrawRect(kPanelX - 8.0f, panelY - 6.0f, 360.0f, lineHeight * lineCount + 12.0f, glm::vec3(0.0f), 0.55f);
        for (int i = 0; i < lineCount; ++i) {
            m_textRenderer.Draw(m_font, lines[i], kPanelX, panelY + lineHeight * i, glm::vec3(0.8f, 0.95f, 0.8f));
        }
    }
}

void GameApplication::RenderDebugVisualization() {
    // Re-added every frame from cached/static data — DebugLineRenderer has
    // no notion of "persistent" lines, only timed ones, so a short fixed
    // duration comfortably longer than one frame keeps these visible
    // continuously without them being genuinely permanent state it has to
    // manage separately from bullet tracers.
    constexpr float kPersistDuration = 0.1f;

    if (m_debugShowCollision) {
        for (const auto& edge : m_collisionWireframeEdges) {
            m_debugLines.AddLine(edge.first, edge.second, glm::vec3(0.2f, 1.0f, 0.3f), kPersistDuration);
        }
    }

    if (m_debugShowNav) {
        constexpr float kCross = 12.0f;
        const glm::vec3 waypointColor(1.0f, 0.9f, 0.2f);
        for (size_t i = 0; i < m_navGraph.GetWaypointCount(); ++i) {
            const glm::vec3& p = m_navGraph.GetWaypointPosition(static_cast<int>(i));
            m_debugLines.AddLine(p - glm::vec3(kCross, 0.0f, 0.0f), p + glm::vec3(kCross, 0.0f, 0.0f), waypointColor, kPersistDuration);
            m_debugLines.AddLine(p - glm::vec3(0.0f, kCross, 0.0f), p + glm::vec3(0.0f, kCross, 0.0f), waypointColor, kPersistDuration);
            m_debugLines.AddLine(p - glm::vec3(0.0f, 0.0f, kCross), p + glm::vec3(0.0f, 0.0f, kCross), waypointColor, kPersistDuration);
        }
        for (const std::pair<int, int>& edge : m_navGraph.GetDebugEdges()) {
            m_debugLines.AddLine(m_navGraph.GetWaypointPosition(edge.first), m_navGraph.GetWaypointPosition(edge.second),
                glm::vec3(0.2f, 0.6f, 1.0f), kPersistDuration);
        }
    }
}

void GameApplication::Run() {
    char titleBuffer[256];

    while (!m_window.ShouldClose()) {
        glfwPollEvents();
        m_input.Update();
        m_time.Tick();

        ProcessInput(m_time.GetDeltaTime());
        Render();

        if (m_network.GetRole() == NetRole::Client) {
            // The client doesn't maintain m_weaponInventory/m_roundManager
            // (see NetProtocol.h) — those would just show stale defaults,
            // so its title bar is built from the network state instead.
            const int slot = m_network.GetLocalSlot();
            const PlayerSnapshot self = (slot >= 0 && slot < kMaxNetPlayers) ? m_lastClientSnapshot.players[slot] : PlayerSnapshot{};
            std::snprintf(titleBuffer, sizeof(titleBuffer),
                "Tactical FPS - Milestone 10 [CLIENT] | FPS: %.0f | %s | Slot %d | A%d-G%d | HP %.0f AR %.0f | %s %d/%d",
                m_time.GetFPS(), m_network.IsConnectedToHost() ? "Connected" : "Connecting...", slot,
                m_lastClientSnapshot.assaultScore, m_lastClientSnapshot.guardianScore,
                self.health, self.armor, self.weaponName, self.magazineAmmo, self.reserveAmmo);
            m_window.SetTitle(titleBuffer);
            m_window.SwapBuffers();
            continue;
        }

        const char* speedLabel = "Run";
        switch (m_player.GetMovementSpeed()) {
            case MovementSpeed::Walk: speedLabel = "Walk"; break;
            case MovementSpeed::Crouch: speedLabel = "Crouch"; break;
            default: break;
        }

        char ammoBuffer[64] = "";
        if (const Weapon* weapon = m_weaponInventory.GetCurrent()) {
            if (weapon->IsReloading()) {
                std::snprintf(ammoBuffer, sizeof(ammoBuffer), "%s RELOADING %.0f%%",
                    weapon->GetData().name.c_str(), weapon->GetReloadProgress01() * 100.0f);
            } else if (weapon->GetData().isMelee) {
                std::snprintf(ammoBuffer, sizeof(ammoBuffer), "%s", weapon->GetData().name.c_str());
            } else {
                std::snprintf(ammoBuffer, sizeof(ammoBuffer), "%s %d/%d",
                    weapon->GetData().name.c_str(), weapon->GetMagazineAmmo(), weapon->GetReserveAmmo());
            }
        }

        char roundBuffer[96];
        const ChargeState chargeState = m_roundManager.GetCharge().GetState();
        if (chargeState == ChargeState::Planted) {
            std::snprintf(roundBuffer, sizeof(roundBuffer), "Charge armed at %s: %.0fs",
                m_roundManager.GetCharge().GetSiteName().c_str(), m_roundManager.GetPhaseTimeRemaining());
        } else {
            const char* phaseLabel = m_roundManager.GetPhase() == RoundPhase::Buy ? "Buy"
                : m_roundManager.GetPhase() == RoundPhase::Active ? "Active" : "Round Over";
            std::snprintf(roundBuffer, sizeof(roundBuffer), "%s: %.0fs", phaseLabel, m_roundManager.GetPhaseTimeRemaining());
        }

        int botsAlive = 0;
        for (const auto& bot : m_bots) {
            if (bot->GetHealth().IsAlive()) ++botsAlive;
        }

        char nadeBuffer[48];
        std::snprintf(nadeBuffer, sizeof(nadeBuffer), "Nades F%d S%d L%d D%d",
            m_grenadeCounts[0], m_grenadeCounts[1], m_grenadeCounts[2], m_grenadeCounts[3]);

        std::string menuState = m_paused ? " | PAUSED" : (m_buyMenu.IsOpen() ? " | BUY MENU" : "");
        if (m_player.IsNoclip()) menuState += " | NOCLIP";
        if (m_playerHealth.IsGodMode()) menuState += " | GOD";
        if (m_debugShowNav) menuState += " | NAV";
        if (m_debugShowCollision) menuState += " | COLLISION";
        if (m_network.GetRole() == NetRole::Host) {
            int remoteCount = 0;
            for (const HostRemotePlayer& remote : m_hostRemotePlayers) {
                if (remote.active) ++remoteCount;
            }
            menuState += " | HOST (" + std::to_string(remoteCount) + " remote)";
        }

        std::snprintf(titleBuffer, sizeof(titleBuffer), "Tactical FPS - Milestone 10 | FPS: %.0f | %s | %s | %s | %s | A%d-G%d | $%d | HP %.0f AR %.0f | Bots %d/%zu | %s%s",
            m_time.GetFPS(), speedLabel, m_player.IsGrounded() ? "Grounded" : "Airborne", ammoBuffer, roundBuffer,
            m_roundManager.GetAssaultScore(), m_roundManager.GetGuardianScore(), m_playerWallet.GetBalance(),
            m_playerHealth.GetHealth(), m_playerHealth.GetArmor(), botsAlive, m_bots.size(), nadeBuffer, menuState.c_str());
        m_window.SetTitle(titleBuffer);

        m_window.SwapBuffers();
    }
}

void GameApplication::Shutdown() {
    m_network.Shutdown();
    m_debugLines.Shutdown();
    m_particles.Shutdown();
    m_decals.Shutdown();
    m_audio.Shutdown();
    m_textRenderer.Shutdown();
    m_uiRenderer.Shutdown();
    m_font.Destroy();
    if (m_overlayVbo) glDeleteBuffers(1, &m_overlayVbo);
    if (m_overlayVao) glDeleteVertexArrays(1, &m_overlayVao);
    m_overlayShader.Destroy();
    m_levelMesh.Destroy();
    m_botBodyMesh.Destroy();
    m_dummyBodyMesh.Destroy();
    m_remotePlayerBodyMesh.Destroy();
    m_shader.Destroy();
    m_window.Destroy();
}

} // namespace Game

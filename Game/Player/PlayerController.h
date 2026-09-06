#pragma once

#include <glm/glm.hpp>
#include "../../Engine/Graphics/Camera.h"
#include "../../Engine/World/CollisionWorld.h"

namespace Game {

enum class MovementSpeed {
    Run,     // default: fastest, loudest
    Walk,    // Shift held: slower, quiet ("silent walking")
    Crouch,  // Ctrl held: slowest, quietest; also shrinks the collision hull
};

// The real first-person movement model, replacing the Milestone 1 free-fly
// test camera entirely: ground/air acceleration and friction (the
// Quake-lineage equations that GoldSrc, and by extension CS 1.6, built its
// movement on — this is what produces "skill-based, not floaty" movement,
// including strafe-jumping/bunnyhopping as an emergent side effect rather
// than a scripted feature), gravity, jump, crouch (which swaps collision
// hulls and smoothly adjusts eye height), and sweep-and-slide collision
// against the level's brushes via Engine::SlideMove.
//
// Internally tracks position as the collision hull's CENTER (matching how
// Brush::ExpandForHull treats the mover as a point at the hull's center),
// not the player's feet or eyes — GetEyePosition() does that conversion.
class PlayerController {
public:
    // spawnPosition is a feet-level origin (as authored in a map's
    // info_player_start), converted internally to hull-center.
    void Init(const glm::vec3& spawnPosition, float spawnYawDegrees, const Engine::CollisionWorld* collisionWorld);

    // Resets position/velocity/crouch/state for a new round without needing
    // the collision world passed again (Init() already stored it) — what
    // RoundManager calls between rounds.
    void Respawn(const glm::vec3& spawnPosition, float spawnYawDegrees);

    // wishForward/wishRight are raw input axes in [-1, 1]; this function
    // handles normalizing/scaling them to a movement speed.
    void Update(float deltaTime, float wishForward, float wishRight, bool jumpHeld, bool crouchHeld, bool walkHeld);

    void ApplyMouseLook(float mouseDeltaX, float mouseDeltaY);

    // Sets view angles directly rather than by delta — for a host applying
    // a remote client's already-computed view direction (Milestone 10),
    // where replaying raw mouse deltas through the host's own
    // sensitivity/smoothing math would just be a second, potentially
    // divergent copy of logic the client already ran once.
    void SetViewAngles(float yawDegrees, float pitchDegrees) { m_camera.SetYawPitch(yawDegrees, pitchDegrees); }

    // A networked client's own avatar has no local physics to run at all
    // (see NetProtocol.h) — its position comes straight from the host's
    // snapshot every frame instead. Assumes standing height; good enough
    // since the result only feeds this client's own cosmetics (its
    // GetFeetPosition()), never a hitbox — the host's copy of this
    // avatar is what every hitscan actually resolves against.
    void SetPositionDirect(const glm::vec3& eyePosition);

    // Debug tool (Milestone 9): flies freely along the view direction,
    // ignoring gravity and collision entirely. A separate code path in
    // Update() rather than a flag threaded through the normal movement
    // math, since noclip shares none of ground/air acceleration, friction,
    // or SlideMove's collision response — trying to reuse that path would
    // mean gating almost every line of it.
    void SetNoclip(bool enabled) { m_noclip = enabled; }
    bool IsNoclip() const { return m_noclip; }

    // Applies an instantaneous rotation bypassing mouse sensitivity — used
    // for weapon recoil view-punch.
    void ApplyRecoil(float pitchDeltaDegrees, float yawDeltaDegrees);

    // Rotates yaw/pitch toward facing targetPosition, capped at
    // maxDegreesPerSecond — how bots aim, as a turn-rate-limited alternative
    // to the instantaneous ApplyMouseLook a human's raw mouse delta drives.
    void TurnToward(const glm::vec3& targetPosition, float maxDegreesPerSecond, float deltaTime);

    const Engine::Camera& GetCamera() const { return m_camera; }
    bool IsGrounded() const { return m_grounded; }
    bool IsCrouching() const { return m_crouching; }
    MovementSpeed GetMovementSpeed() const { return m_currentSpeedTier; }
    float GetHorizontalSpeed() const;

    glm::vec3 GetFeetPosition() const;
    glm::vec3 GetEyePosition() const;
    float GetCurrentHalfHeight() const;

private:

    void ApplyFriction(float deltaTime);
    void Accelerate(const glm::vec3& wishDir, float wishSpeed, float acceleration, float deltaTime);
    void CheckGrounded();
    void UpdateCrouch(bool crouchHeld, float deltaTime);

    // Moves m_position/m_velocity by one SlideMove, but — only while
    // allowStepUp is true (i.e. grounded and not this frame's jump) — also
    // tries the same move lifted up by kStepHeight first, so a stair riser
    // (a near-vertical face SlideMove's plane-clipping alone can't climb)
    // gets stepped over instead of blocking the player outright.
    void MoveWithStepUp(float deltaTime, const std::vector<Engine::Brush>& brushes, bool allowStepUp);
    void UpdateNoclip(float deltaTime, float wishForward, float wishRight, bool jumpHeld, bool crouchHeld);

    bool m_noclip = false;

    Engine::Camera m_camera;
    const Engine::CollisionWorld* m_collisionWorld = nullptr;

    glm::vec3 m_position{0.0f}; // collision hull center
    glm::vec3 m_velocity{0.0f};
    bool m_grounded = false;
    bool m_crouching = false;
    float m_crouchLerp = 0.0f; // 0 = fully standing, 1 = fully crouched
    MovementSpeed m_currentSpeedTier = MovementSpeed::Run;

    // Tuning constants, in engine units (see Engine/World/CollisionHull.h).
    static constexpr float kRunSpeed = 250.0f;
    static constexpr float kWalkSpeed = 120.0f;
    static constexpr float kCrouchSpeed = 80.0f;
    static constexpr float kGroundAcceleration = 10.0f;
    static constexpr float kAirAcceleration = 2.0f;
    static constexpr float kGroundFriction = 6.0f;
    static constexpr float kGravity = 800.0f;
    static constexpr float kJumpVelocity = 270.0f;
    static constexpr float kStandingEyeHeightAboveFeet = 64.0f;
    static constexpr float kCrouchingEyeHeightAboveFeet = 30.0f;
    static constexpr float kCrouchTransitionSpeed = 8.0f; // crouchLerp units per second
    static constexpr float kGroundCheckDistance = 4.0f;
    static constexpr float kStepHeight = 18.0f;
    static constexpr float kNoclipSpeed = 600.0f;
};

} // namespace Game

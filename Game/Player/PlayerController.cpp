#include "PlayerController.h"
#include "../../Engine/Physics/CollisionQuery.h"
#include "../../Engine/World/CollisionHull.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace Game {

namespace {
float HorizontalDistanceSquared(const glm::vec3& a, const glm::vec3& b) {
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;
    return dx * dx + dz * dz;
}
} // namespace

void PlayerController::Init(const glm::vec3& spawnPosition, float spawnYawDegrees, const Engine::CollisionWorld* collisionWorld) {
    m_collisionWorld = collisionWorld;
    Respawn(spawnPosition, spawnYawDegrees);
}

void PlayerController::Respawn(const glm::vec3& spawnPosition, float spawnYawDegrees) {
    const float standingHalfHeight = Engine::CollisionHull::Standing().halfExtents.y;
    m_position = spawnPosition + glm::vec3(0.0f, standingHalfHeight, 0.0f);
    m_velocity = glm::vec3(0.0f);
    m_grounded = false;
    m_crouching = false;
    m_crouchLerp = 0.0f;
    m_currentSpeedTier = MovementSpeed::Run;

    m_camera.Init(GetEyePosition(), spawnYawDegrees, 0.0f);
}

void PlayerController::ApplyMouseLook(float mouseDeltaX, float mouseDeltaY) {
    m_camera.ProcessMouseLook(mouseDeltaX, mouseDeltaY);
}

void PlayerController::ApplyRecoil(float pitchDeltaDegrees, float yawDeltaDegrees) {
    m_camera.ApplyExternalRotation(yawDeltaDegrees, pitchDeltaDegrees);
}

float PlayerController::GetHorizontalSpeed() const {
    return std::sqrt(m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z);
}

void PlayerController::TurnToward(const glm::vec3& targetPosition, float maxDegreesPerSecond, float deltaTime) {
    const glm::vec3 toTarget = targetPosition - GetEyePosition();
    const float horizontalDistance = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);

    const float desiredYaw = glm::degrees(std::atan2(toTarget.z, toTarget.x));
    const float desiredPitch = glm::degrees(std::atan2(toTarget.y, horizontalDistance));

    // Shortest angular distance in [-180, 180], so turning past the wrap
    // point (e.g. from 179 to -179 degrees) doesn't spin the long way around.
    auto shortestDelta = [](float from, float to) {
        float delta = std::fmod(to - from + 180.0f, 360.0f);
        if (delta < 0.0f) delta += 360.0f;
        return delta - 180.0f;
    };

    const float maxStep = maxDegreesPerSecond * deltaTime;
    const float yawDelta = std::clamp(shortestDelta(m_camera.GetYaw(), desiredYaw), -maxStep, maxStep);
    const float pitchDelta = std::clamp(shortestDelta(m_camera.GetPitch(), desiredPitch), -maxStep, maxStep);

    m_camera.ApplyExternalRotation(yawDelta, pitchDelta);
}

float PlayerController::GetCurrentHalfHeight() const {
    const float standing = Engine::CollisionHull::Standing().halfExtents.y;
    const float crouching = Engine::CollisionHull::Crouching().halfExtents.y;
    return standing + (crouching - standing) * m_crouchLerp;
}

void PlayerController::SetPositionDirect(const glm::vec3& eyePosition) {
    const float standingHalfHeight = Engine::CollisionHull::Standing().halfExtents.y;
    const float feetY = eyePosition.y - kStandingEyeHeightAboveFeet;
    m_position = glm::vec3(eyePosition.x, feetY + standingHalfHeight, eyePosition.z);
    m_velocity = glm::vec3(0.0f);
    m_camera.SetPosition(eyePosition);
}

glm::vec3 PlayerController::GetFeetPosition() const {
    return glm::vec3(m_position.x, m_position.y - GetCurrentHalfHeight(), m_position.z);
}

glm::vec3 PlayerController::GetEyePosition() const {
    const float halfHeight = GetCurrentHalfHeight();
    const float eyeAboveFeet = kStandingEyeHeightAboveFeet + (kCrouchingEyeHeightAboveFeet - kStandingEyeHeightAboveFeet) * m_crouchLerp;
    const float feetY = m_position.y - halfHeight;
    return glm::vec3(m_position.x, feetY + eyeAboveFeet, m_position.z);
}

void PlayerController::UpdateCrouch(bool crouchHeld, float deltaTime) {
    // NOTE: this does not yet check for a ceiling overhead before letting
    // the player stand back up (so standing up under something low could
    // embed the hull in it for a frame). Not an issue on the current test
    // map; a proper "can I stand here" trace is a follow-up before any map
    // gets low crawlspaces.
    const float target = crouchHeld ? 1.0f : 0.0f;
    const float previousHalfHeight = GetCurrentHalfHeight();

    if (m_crouchLerp < target) {
        m_crouchLerp = std::min(target, m_crouchLerp + kCrouchTransitionSpeed * deltaTime);
    } else if (m_crouchLerp > target) {
        m_crouchLerp = std::max(target, m_crouchLerp - kCrouchTransitionSpeed * deltaTime);
    }
    m_crouching = crouchHeld;

    // Keep the feet planted while the hull's height changes — only the top
    // of the hull should move as the player crouches/stands, not the point
    // touching the floor.
    const float newHalfHeight = GetCurrentHalfHeight();
    m_position.y += (newHalfHeight - previousHalfHeight);
}

void PlayerController::Accelerate(const glm::vec3& wishDir, float wishSpeed, float acceleration, float deltaTime) {
    const float currentSpeed = glm::dot(m_velocity, wishDir);
    const float addSpeed = wishSpeed - currentSpeed;
    if (addSpeed <= 0.0f) return;

    float accelSpeed = acceleration * deltaTime * wishSpeed;
    accelSpeed = std::min(accelSpeed, addSpeed);

    m_velocity += wishDir * accelSpeed;
}

void PlayerController::ApplyFriction(float deltaTime) {
    const float speed = std::sqrt(m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z);
    if (speed < 1.0f) {
        m_velocity.x = 0.0f;
        m_velocity.z = 0.0f;
        return;
    }

    const float drop = speed * kGroundFriction * deltaTime;
    const float newSpeed = std::max(speed - drop, 0.0f);
    const float scale = newSpeed / speed;
    m_velocity.x *= scale;
    m_velocity.z *= scale;
}

void PlayerController::CheckGrounded() {
    const std::vector<Engine::Brush>& activeBrushes = m_crouchLerp > 0.5f
        ? m_collisionWorld->GetCrouchingHullBrushes()
        : m_collisionWorld->GetStandingHullBrushes();

    const Engine::TraceResult ground = Engine::CheckGroundContact(m_position, kGroundCheckDistance, activeBrushes);
    m_grounded = ground.hit;
}

void PlayerController::MoveWithStepUp(float deltaTime, const std::vector<Engine::Brush>& brushes, bool allowStepUp) {
    const glm::vec3 startPosition = m_position;

    glm::vec3 flatPosition = m_position;
    glm::vec3 flatVelocity = m_velocity;
    Engine::SlideMove(flatPosition, flatVelocity, deltaTime, brushes);

    if (!allowStepUp) {
        m_position = flatPosition;
        m_velocity = flatVelocity;
        return;
    }

    const float flatDistanceSq = HorizontalDistanceSquared(startPosition, flatPosition);
    const float wishedDistanceSq = (m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z) * deltaTime * deltaTime;

    // If the flat move already made nearly as much horizontal progress as
    // requested, nothing blocked it — skip the extra step-up trace work.
    if (wishedDistanceSq < 1.0f || flatDistanceSq >= wishedDistanceSq * 0.95f) {
        m_position = flatPosition;
        m_velocity = flatVelocity;
        return;
    }

    // Lift up by the step height (capped by whatever's actually overhead —
    // e.g. a low ceiling), redo the same move at that height, then settle
    // back down onto whatever's underneath.
    const Engine::TraceResult upTrace = Engine::TraceMove(startPosition, startPosition + glm::vec3(0.0f, kStepHeight, 0.0f), brushes);
    glm::vec3 steppedPosition = upTrace.endPosition;
    glm::vec3 steppedVelocity = m_velocity;
    Engine::SlideMove(steppedPosition, steppedVelocity, deltaTime, brushes);

    const Engine::TraceResult downTrace = Engine::TraceMove(
        steppedPosition, steppedPosition - glm::vec3(0.0f, kStepHeight + kGroundCheckDistance, 0.0f), brushes);
    if (!downTrace.hit) {
        // Nothing to land on nearby (e.g. this would step off a ledge) —
        // the ordinary flat move already handles that case correctly.
        m_position = flatPosition;
        m_velocity = flatVelocity;
        return;
    }
    steppedPosition = downTrace.endPosition;

    const float steppedDistanceSq = HorizontalDistanceSquared(startPosition, steppedPosition);
    if (steppedDistanceSq > flatDistanceSq) {
        m_position = steppedPosition;
        m_velocity = steppedVelocity;
    } else {
        m_position = flatPosition;
        m_velocity = flatVelocity;
    }
}

void PlayerController::Update(float deltaTime, float wishForward, float wishRight, bool jumpHeld, bool crouchHeld, bool walkHeld) {
    if (m_noclip) {
        UpdateNoclip(deltaTime, wishForward, wishRight, jumpHeld, crouchHeld);
        return;
    }

    UpdateCrouch(crouchHeld, deltaTime);

    // Movement direction comes from camera yaw only, flattened to the
    // horizontal plane — looking up/down shouldn't tilt movement speed.
    const float yawRad = glm::radians(m_camera.GetYaw());
    const glm::vec3 forward(std::cos(yawRad), 0.0f, std::sin(yawRad));
    const glm::vec3 right(-forward.z, 0.0f, forward.x);

    glm::vec3 wishDir = forward * wishForward + right * wishRight;
    const float wishDirLength = glm::length(wishDir);
    if (wishDirLength > 0.0001f) {
        wishDir /= wishDirLength;
    }

    float wishSpeed;
    if (m_crouching) {
        m_currentSpeedTier = MovementSpeed::Crouch;
        wishSpeed = kCrouchSpeed;
    } else if (walkHeld) {
        m_currentSpeedTier = MovementSpeed::Walk;
        wishSpeed = kWalkSpeed;
    } else {
        m_currentSpeedTier = MovementSpeed::Run;
        wishSpeed = kRunSpeed;
    }

    CheckGrounded();

    if (m_grounded) {
        ApplyFriction(deltaTime);
        Accelerate(wishDir, wishSpeed, kGroundAcceleration, deltaTime);
        // Held (not just pressed) so holding Space auto-hops on landing —
        // the classic Quake-lineage bunnyhop behavior, an emergent skill
        // mechanic rather than something we need to script separately.
        if (jumpHeld) {
            m_velocity.y = kJumpVelocity;
            m_grounded = false;
        }
    } else {
        Accelerate(wishDir, wishSpeed, kAirAcceleration, deltaTime);
        m_velocity.y -= kGravity * deltaTime;
    }

    const std::vector<Engine::Brush>& activeBrushes = m_crouchLerp > 0.5f
        ? m_collisionWorld->GetCrouchingHullBrushes()
        : m_collisionWorld->GetStandingHullBrushes();

    // m_grounded already reflects "still grounded after this frame's jump
    // check" — false on the frame a jump is triggered, which correctly
    // disables step-up while leaving the ground (matches genre convention:
    // no stair-climbing assistance mid-air).
    MoveWithStepUp(deltaTime, activeBrushes, m_grounded);

    m_camera.SetPosition(GetEyePosition());
}

void PlayerController::UpdateNoclip(float deltaTime, float wishForward, float wishRight, bool jumpHeld, bool crouchHeld) {
    // Full 3D camera-relative movement (forward includes pitch, unlike
    // normal movement's yaw-only flattening) plus world-space up/down on
    // jump/crouch — the standard noclip control scheme.
    glm::vec3 move = m_camera.GetForward() * wishForward + m_camera.GetRight() * wishRight;
    if (jumpHeld) move += glm::vec3(0.0f, 1.0f, 0.0f);
    if (crouchHeld) move -= glm::vec3(0.0f, 1.0f, 0.0f);

    const float moveLength = glm::length(move);
    if (moveLength > 0.0001f) {
        move /= moveLength;
        m_position += move * kNoclipSpeed * deltaTime;
    }
    m_velocity = glm::vec3(0.0f);
    m_grounded = false;
    m_camera.SetPosition(GetEyePosition());
}

} // namespace Game

#pragma once

#include <glm/glm.hpp>

namespace Engine {

// A free-fly camera used to view the Milestone 1 test scene.
//
// This is intentionally NOT the game's player controller — it has no
// gravity, no collision, and can fly. Milestone 2 replaces how the camera's
// position/orientation get driven (a real PlayerController with movement,
// gravity and collision) but reuses this class for the view/projection math,
// since "where is the eye and which way is it looking" stays the same
// problem regardless of how position was computed.
class Camera {
public:
    void Init(glm::vec3 position, float yawDegrees = -90.0f, float pitchDegrees = 0.0f);

    // dx/dy are raw mouse-delta pixels; sensitivity is applied here.
    void ProcessMouseLook(float dx, float dy, float sensitivity = 0.12f);

    // Rotates the camera by an exact angle in degrees, bypassing mouse
    // sensitivity — used for weapon recoil view-punch, not player input.
    void ApplyExternalRotation(float yawDeltaDegrees, float pitchDeltaDegrees);

    // Sets orientation directly rather than by delta — for applying a
    // value computed elsewhere (a networked remote player's reported view
    // angles) rather than accumulating local input.
    void SetYawPitch(float yawDegrees, float pitchDegrees);

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;

    const glm::vec3& GetPosition() const { return m_position; }
    void SetPosition(const glm::vec3& pos) { m_position = pos; }

    const glm::vec3& GetForward() const { return m_forward; }
    const glm::vec3& GetRight() const { return m_right; }
    const glm::vec3& GetUp() const { return m_up; }

    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }
    float GetFovDegrees() const { return m_fovDegrees; }
    void SetFovDegrees(float fov) { m_fovDegrees = fov; }

private:
    void RecalculateVectors();

    glm::vec3 m_position{0.0f};
    glm::vec3 m_forward{0.0f, 0.0f, -1.0f};
    glm::vec3 m_right{1.0f, 0.0f, 0.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};

    float m_yaw = -90.0f;   // degrees; -90 faces -Z
    float m_pitch = 0.0f;   // degrees; clamped to avoid gimbal flip
    float m_fovDegrees = 90.0f;
    // In engine units (see Engine/World/CollisionHull.h — a player is ~32
    // units wide), not meters.
    float m_nearPlane = 1.0f;
    float m_farPlane = 8192.0f;
};

} // namespace Engine

#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Engine {

void Camera::Init(glm::vec3 position, float yawDegrees, float pitchDegrees) {
    m_position = position;
    m_yaw = yawDegrees;
    m_pitch = pitchDegrees;
    RecalculateVectors();
}

void Camera::ProcessMouseLook(float dx, float dy, float sensitivity) {
    m_yaw += dx * sensitivity;
    // Screen-space Y grows downward, so subtract to make "mouse up" look up.
    m_pitch -= dy * sensitivity;
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    RecalculateVectors();
}

void Camera::ApplyExternalRotation(float yawDeltaDegrees, float pitchDeltaDegrees) {
    m_yaw += yawDeltaDegrees;
    m_pitch = std::clamp(m_pitch + pitchDeltaDegrees, -89.0f, 89.0f);
    RecalculateVectors();
}

void Camera::SetYawPitch(float yawDegrees, float pitchDegrees) {
    m_yaw = yawDegrees;
    m_pitch = std::clamp(pitchDegrees, -89.0f, 89.0f);
    RecalculateVectors();
}

void Camera::RecalculateVectors() {
    const float yawRad = glm::radians(m_yaw);
    const float pitchRad = glm::radians(m_pitch);

    glm::vec3 forward;
    forward.x = std::cos(yawRad) * std::cos(pitchRad);
    forward.y = std::sin(pitchRad);
    forward.z = std::sin(yawRad) * std::cos(pitchRad);
    m_forward = glm::normalize(forward);

    static const glm::vec3 kWorldUp(0.0f, 1.0f, 0.0f);
    m_right = glm::normalize(glm::cross(m_forward, kWorldUp));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_forward, m_up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(m_fovDegrees), aspectRatio, m_nearPlane, m_farPlane);
}

} // namespace Engine

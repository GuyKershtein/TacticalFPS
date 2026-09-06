#version 330 core

in vec3 vNormal;
in vec3 vColor;
out vec4 FragColor;

// A single directional "sun" light plus a flat ambient term — the smallest
// lighting model that makes brush geometry read as three-dimensional
// (Milestone 4). Point lights/shadows are later Phase 16 graphics work.
uniform vec3 uLightDirection; // world-space, points FROM the light TOWARD the scene, normalized
uniform vec3 uLightColor;
uniform float uAmbientStrength;

void main() {
    vec3 normal = normalize(vNormal);
    float diffuseFactor = max(dot(normal, -uLightDirection), 0.0);

    vec3 ambient = uAmbientStrength * uLightColor;
    vec3 diffuse = diffuseFactor * uLightColor;

    vec3 result = (ambient + diffuse) * vColor;
    FragColor = vec4(result, 1.0);
}

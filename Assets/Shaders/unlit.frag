#version 330 core

in vec3 vColor;
out vec4 FragColor;

// Multiplied against the mesh's own baked vertex color, so callers that
// don't care (debug lines) can just leave these at their defaults (set
// explicitly every draw — GLSL uniforms are NOT zero-initialized safely
// across every driver, so every user of this shader must set them).
uniform vec3 uTint;
uniform float uAlpha;

void main() {
    FragColor = vec4(vColor * uTint, uAlpha);
}

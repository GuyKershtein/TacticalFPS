#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uAtlas;
uniform vec3 uColor;
uniform float uAlpha;

void main() {
    float coverage = texture(uAtlas, vUV).r;
    FragColor = vec4(uColor, coverage * uAlpha);
}

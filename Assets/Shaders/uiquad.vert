#version 330 core
layout(location = 0) in vec2 aPos; // unit quad, corners at (0,0)-(1,1)

uniform mat4 uProjection;
uniform vec2 uOffset;
uniform vec2 uSize;

void main() {
    vec2 pixelPos = aPos * uSize + uOffset;
    gl_Position = uProjection * vec4(pixelPos, 0.0, 1.0);
}

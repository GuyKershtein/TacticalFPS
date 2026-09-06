#version 330 core

// A full-screen quad in normalized device coordinates directly — no
// view/projection needed since this always covers the whole screen
// regardless of camera (used for the flashbang whiteout).
layout(location = 0) in vec2 aNdcPosition;

void main() {
    gl_Position = vec4(aNdcPosition, 0.0, 1.0);
}

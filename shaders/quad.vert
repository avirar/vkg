#version 450

// Per-point vertex from particle buffer
layout(location = 0) in vec2 inScreenPos;
layout(location = 1) in float inBrightness;

layout(location = 0) out float fragBrightness;

void main() {
    gl_Position = vec4(inScreenPos, 0.0, 1.0);
    gl_PointSize = 6.0;
    fragBrightness = inBrightness;
}

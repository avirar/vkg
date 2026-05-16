#version 450

layout(location = 0) in vec2 inScreenPos;
layout(location = 1) in float inBrightness;

layout(push_constant) uniform ParticlePush {
    float viewportHeight;
    float aspectY;
    float pointSizeMult;
    float _pad;
} pc;

layout(location = 0) out float fragBrightness;

void main() {
    gl_Position = vec4(inScreenPos, 0.0, 1.0);
    gl_PointSize = pc.viewportHeight * pc.aspectY * 0.02 * pc.pointSizeMult;
    fragBrightness = inBrightness;
}

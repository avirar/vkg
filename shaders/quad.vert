#version 450

layout(location = 0) in vec2 inScreenPos;
layout(location = 1) in float inBrightness;
layout(location = 2) in float inHue;

layout(push_constant) uniform ParticlePush {
    float viewportHeight;
    float aspectY;
    float pointSizeMult;
    uint mode;
    float hyperIntensity;
    float loR, loG, loB;
    float hiR, hiG, hiB;
} pc;

layout(location = 0) out float fragBrightness;
layout(location = 1) out float fragHue;

void main() {
    gl_Position = vec4(inScreenPos, 0.0, 1.0);
    gl_PointSize = pc.viewportHeight * pc.aspectY * 0.02 * pc.pointSizeMult * inBrightness;
    fragBrightness = inBrightness;
    fragHue = inHue;
}

#version 450

layout(location = 0) in vec2 inScreenPos;
layout(location = 1) in float inBrightness;
layout(location = 2) in float inVelHue;
layout(location = 3) in float inDistHue;

layout(push_constant) uniform ParticlePush {
    float viewportHeight;
    float aspectY;
    float pointSizeMult;
    uint velMode;
    uint distMode;
    float velLoR, velLoG, velLoB;
    float velHiR, velHiG, velHiB;
    float distLoR, distLoG, distLoB;
    float distHiR, distHiG, distHiB;
} pc;

layout(location = 0) out float fragBrightness;
layout(location = 1) out float fragVelHue;
layout(location = 2) out float fragDistHue;

void main() {
    gl_Position = vec4(inScreenPos, 0.0, 1.0);
    gl_PointSize = pc.viewportHeight * pc.aspectY * 0.02 * pc.pointSizeMult * inBrightness;
    fragBrightness = inBrightness;
    fragVelHue = inVelHue;
    fragDistHue = inDistHue;
}

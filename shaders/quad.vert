#version 450

layout(location = 0) in vec2 inScreenPos;
layout(location = 1) in float inBrightness;

layout(push_constant) uniform ParticlePush {
    float viewportHeight;
    float aspectY;
} pc;

layout(location = 0) out float fragBrightness;

void main() {
    gl_Position = vec4(inScreenPos, 0.0, 1.0);
    // Match original quad size: 0.01 * aspectY NDC per half-quad = 0.02 * aspectY NDC full
    // Convert NDC to pixels: NDC_height * viewportHeight_pixels
    gl_PointSize = pc.viewportHeight * pc.aspectY * 0.02;
    fragBrightness = inBrightness;
}

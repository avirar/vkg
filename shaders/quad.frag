#version 450

layout(location = 0) in float fragBrightness;
layout(location = 1) in float fragVelHue;
layout(location = 2) in float fragDistHue;
layout(location = 0) out vec4 outColor;

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
    float blendAlphaScale;
    float colorCap;
} pc;

void main() {
    vec2 center = gl_PointCoord - vec2(0.5);
    float d = dot(center, center) * 8.1632653;
    if (d >= 1.0) discard;

    float falloff = 1.0 - d;
    falloff = falloff * falloff;

    // Step 1: velocity effect
    vec3 color;
    if (pc.velMode == 2u) {
        float boost = 1.0 + fragVelHue;
        color = vec3(pc.velLoR, pc.velLoG, pc.velLoB) * boost;
    } else if (pc.velMode == 1u) {
        color = mix(vec3(pc.velLoR, pc.velLoG, pc.velLoB),
                    vec3(pc.velHiR, pc.velHiG, pc.velHiB), fragVelHue);
    } else {
        color = vec3(pc.velLoR, pc.velLoG, pc.velLoB);
    }

    // Step 2: distance effect (applied on top)
    if (pc.distMode == 2u) {
        color *= (1.0 + fragDistHue);
    } else if (pc.distMode == 1u) {
        color = mix(color, vec3(pc.distHiR, pc.distHiG, pc.distHiB), fragDistHue);
    }

    float vis = falloff * fragBrightness;
    vec3 cclamped = min(color, pc.colorCap);
    float alpha = vis * pc.blendAlphaScale;
    outColor = vec4(cclamped * vis, alpha);
}

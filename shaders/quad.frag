#version 450

layout(location = 0) in float fragBrightness;
layout(location = 1) in float fragHue;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform ParticlePush {
    float viewportHeight;
    float aspectY;
    float pointSizeMult;
    uint mode;
    float hyperIntensity;
    float loR, loG, loB;
    float hiR, hiG, hiB;
} pc;

void main() {
    vec2 center = gl_PointCoord - vec2(0.5);
    float d = dot(center, center) * 8.1632653;
    if (d >= 1.0) discard;

    float falloff = 1.0 - d;
    falloff = falloff * falloff;

    vec3 color;
    if (pc.mode == 2u) {
        float boost = 1.0 + fragHue * pc.hyperIntensity;
        color = vec3(pc.loR, pc.loG, pc.loB) * boost;
    } else if (pc.mode == 1u) {
        color = mix(vec3(pc.loR, pc.loG, pc.loB),
                    vec3(pc.hiR, pc.hiG, pc.hiB), fragHue);
    } else {
        color = vec3(pc.loR, pc.loG, pc.loB);
    }

    float vis = falloff * fragBrightness;
    outColor = vec4(color * vis, vis);
}

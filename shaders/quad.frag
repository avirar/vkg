#version 450

layout(location = 0) in float fragBrightness;
layout(location = 1) in float fragHue;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

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
    vec2 center = gl_PointCoord - 0.5;
    float dist = length(center);
    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);

    vec4 texColor = texture(texSampler, gl_PointCoord);

    vec3 color;
    if (pc.mode == 1u) {
        // color mode: blend lo→hi based on velocity hue
        color = mix(vec3(pc.loR, pc.loG, pc.loB),
                    vec3(pc.hiR, pc.hiG, pc.hiB), fragHue);
    } else if (pc.mode == 2u) {
        // brightness mode: base color + velocity brightness boost
        float boost = 1.0 + fragHue * pc.hyperIntensity;
        color = vec3(pc.loR, pc.loG, pc.loB) * boost;
    } else {
        // off: static base color
        color = vec3(pc.loR, pc.loG, pc.loB);
    }

    outColor = vec4(color * fragBrightness * texColor.r,
                    alpha * fragBrightness * texColor.r);
}

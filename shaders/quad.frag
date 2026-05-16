#version 450

layout(location = 0) in float fragBrightness;
layout(location = 1) in float fragHue;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform ParticlePush {
    float viewportHeight;
    float aspectY;
    float pointSizeMult;
    uint hypercolor;
    float hyperIntensity;
    float loR, loG, loB;
    float hiR, hiG, hiB;
    float staticR, staticG, staticB;
} pc;

void main() {
    vec2 center = gl_PointCoord - 0.5;
    float dist = length(center);
    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);

    vec4 texColor = texture(texSampler, gl_PointCoord);

    vec3 color;
    if (pc.hypercolor != 0u) {
        float hue = clamp(fragHue * pc.hyperIntensity / 8.0, 0.0, 1.0);
        color = mix(vec3(pc.loR, pc.loG, pc.loB),
                    vec3(pc.hiR, pc.hiG, pc.hiB), hue);
    } else {
        color = vec3(pc.staticR, pc.staticG, pc.staticB);
    }

    outColor = vec4(color * fragBrightness * texColor.r,
                    alpha * fragBrightness * texColor.r);
}

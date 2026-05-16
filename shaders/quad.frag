#version 450

layout(location = 0) in float fragBrightness;
layout(location = 1) in float fragHue;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

void main() {
    vec2 center = gl_PointCoord - 0.5;
    float dist = length(center);
    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);

    vec4 texColor = texture(texSampler, gl_PointCoord);

    // Low velocity = orange (1.0, 0.19, 0.065), high velocity = blue-white
    vec3 loColor = vec3(1.0, 0.19, 0.065);
    vec3 hiColor = vec3(0.6, 0.8, 1.0);
    vec3 color = mix(loColor, hiColor, fragHue);

    outColor = vec4(color * fragBrightness * texColor.r,
                    alpha * fragBrightness * texColor.r);
}

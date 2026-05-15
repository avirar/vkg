#version 450

layout(location = 0) in float fragBrightness;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

void main() {
    // Soft circle from point sprite
    vec2 center = gl_PointCoord - 0.5;
    float dist = length(center);
    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);

    vec4 texColor = texture(texSampler, gl_PointCoord);

    // Orange particle color (original: R=1.0, G=0.19, B=0.065)
    vec3 color = vec3(1.0, 0.19, 0.065);
    outColor = vec4(color * fragBrightness * texColor.r,
                    alpha * fragBrightness * texColor.r);
}

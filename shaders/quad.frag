#version 450

layout(location = 0) in float fragBrightness;
layout(location = 0) out vec4 outColor;

void main() {
    // Soft circle from point sprite
    vec2 center = gl_PointCoord - 0.5;
    float dist = length(center);
    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);

    // Orange-ish particle color (matches original: R=1.0, G=0.19, B=0.065)
    vec3 color = vec3(1.0, 0.19, 0.065);
    outColor = vec4(color * fragBrightness, alpha * fragBrightness);
}

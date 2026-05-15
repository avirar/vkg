#version 450

layout(location = 0) in float fragAlpha;
layout(location = 0) out vec4 outColor;

void main() {
    // Radial falloff for soft glow
    vec2 center = gl_PointCoord - 0.5;
    float dist = length(center);
    float glow = 1.0 - smoothstep(0.0, 0.5, dist);
    
    // Blue-ish sun color (matches original: R=0.07, G=0.30, B=1.0)
    vec3 color = vec3(0.07, 0.30, 1.0);
    outColor = vec4(color * glow, glow * fragAlpha);
}

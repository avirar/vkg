#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in float fragAlpha;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);

    // Radial falloff from quad center (fragTexCoord goes from 0 to 1)
    vec2 center = fragTexCoord - 0.5;
    float dist = length(center);
    float glow = 1.0 - smoothstep(0.0, 0.5, dist);

    // Blue-ish sun color (original: R=0.07, G=0.30, B=1.0)
    vec3 color = vec3(0.07, 0.30, 1.0);
    outColor = vec4(color * texColor.r * glow, texColor.r * glow * fragAlpha);
}

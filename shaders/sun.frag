#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in float fragAlpha;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);

    float l = texColor.r;

    vec3 color = vec3(0.07, 0.30, 1.0);
    outColor = vec4(color * l, l * fragAlpha);
}

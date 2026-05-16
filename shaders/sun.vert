#version 450

layout(location = 0) in vec2 inPosition;

layout(push_constant) uniform SunPush {
    vec2 center;
    float aspectX;
    float sunPulse;
    float layerScales[7];
    float layerAlphas[7];
} pc;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out float fragAlpha;

void main() {
    int i = gl_InstanceIndex;
    float scale = pc.layerScales[i];
    float alpha = pc.layerAlphas[i] * pc.sunPulse;
    gl_Position = vec4(pc.center.x + inPosition.x * scale * pc.aspectX,
                       pc.center.y + inPosition.y * scale,
                       0.0, 1.0);
    fragTexCoord = (inPosition + 1.0) * 0.5;
    fragAlpha = alpha;
}

#version 450

layout(location = 0) in vec2 inPosition;

layout(push_constant) uniform SunPush {
    vec2 center;
    float scaleX;
    float scaleY;
    float alpha;
    float _pad;
} pc;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out float fragAlpha;

void main() {
    gl_Position = vec4(pc.center.x + inPosition.x * pc.scaleX,
                       pc.center.y + inPosition.y * pc.scaleY,
                       0.0, 1.0);
    fragTexCoord = (inPosition + 1.0) * 0.5;
    fragAlpha = pc.alpha;
}

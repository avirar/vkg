#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;

layout(push_constant) uniform OsdPush {
    vec2 invScreenSize;
} pc;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = vec4(inPos.x * pc.invScreenSize.x * 2.0 - 1.0,
                       1.0 - inPos.y * pc.invScreenSize.y * 2.0,
                       inPos.z, 1.0);
    fragColor = inColor;
}

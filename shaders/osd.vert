#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;

layout(push_constant) uniform OsdPush {
    vec2 invScreenSize;
    float scale;
} pc;

layout(location = 0) out vec4 fragColor;

void main() {
    float sx = pc.invScreenSize.x * 2.0 * pc.scale;
    float sy = pc.invScreenSize.y * 2.0 * pc.scale;
    gl_Position = vec4(inPos.x * sx - 1.0,
                       inPos.y * sy - 1.0,
                       inPos.z, 1.0);
    fragColor = inColor;
}

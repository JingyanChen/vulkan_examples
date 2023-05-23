#version 450
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

layout(location = 0) out vec3 fragColor;
void main() {
    gl_Position =  ubo.proj * ubo.view * ubo.model * vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = vec3(positions[gl_VertexIndex],0.0);
}
#version 450
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

vec3 positions[3] = vec3[](
    vec3(0.0, -0.5,0.0),
    vec3(0.5, 0.5,0.0),
    vec3(-0.5, 0.5,0.0)
);

layout(location = 0) out vec3 fragColor;
void main() {
    gl_Position =  ubo.view * ubo.model * vec4(positions[gl_VertexIndex], 1.0);
}
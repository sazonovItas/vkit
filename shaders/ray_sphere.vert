#version 450

layout(location = 0) out vec3 outLocalPos;
layout(location = 1) out vec3 outWorldPos;

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
    vec3 position;
} camera;

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint materialIndex;
} pcs;

void main() {
    vec3 cubeVertices[36] = vec3[](
            vec3(-0.5, -0.5, -0.5), vec3(0.5, -0.5, -0.5), vec3(0.5, 0.5, -0.5), vec3(0.5, 0.5, -0.5), vec3(-0.5, 0.5, -0.5), vec3(-0.5, -0.5, -0.5),
            vec3(-0.5, -0.5, 0.5), vec3(0.5, -0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(-0.5, 0.5, 0.5), vec3(-0.5, -0.5, 0.5),
            vec3(-0.5, 0.5, 0.5), vec3(-0.5, 0.5, -0.5), vec3(-0.5, -0.5, -0.5), vec3(-0.5, -0.5, -0.5), vec3(-0.5, -0.5, 0.5), vec3(-0.5, 0.5, 0.5),
            vec3(0.5, 0.5, 0.5), vec3(0.5, 0.5, -0.5), vec3(0.5, -0.5, -0.5), vec3(0.5, -0.5, -0.5), vec3(0.5, -0.5, 0.5), vec3(0.5, 0.5, 0.5),
            vec3(-0.5, -0.5, -0.5), vec3(0.5, -0.5, -0.5), vec3(0.5, -0.5, 0.5), vec3(0.5, -0.5, 0.5), vec3(-0.5, -0.5, 0.5), vec3(-0.5, -0.5, -0.5),
            vec3(-0.5, 0.5, -0.5), vec3(0.5, 0.5, -0.5), vec3(0.5, 0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(-0.5, 0.5, 0.5), vec3(-0.5, 0.5, -0.5)
        );

    vec3 localPos = cubeVertices[gl_VertexIndex];
    vec4 worldPos = pcs.model * vec4(localPos, 1.0);

    outLocalPos = localPos;
    outWorldPos = worldPos.xyz;

    gl_Position = camera.proj * camera.view * worldPos;
}

#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
    vec3 position;
} camera;

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint primIndex;
    uint skinOffset;
} pcs;

const vec3 LIGHT_DIR = -vec3(0.0, 0.0, -1.0);

void main() {
    vec3 normal = normalize(inNormal);

    if (length(normal) < 0.1) {
        normal = vec3(0.0, 1.0, 0.0);
    }

    float diff = max(dot(normal, LIGHT_DIR), 0.0);

    vec3 albedo = vec3(0.2, 0.5, 0.8);
    vec3 ambient = albedo * 0.1;

    vec3 finalColor = ambient + (albedo * diff);

    finalColor = vec3(inUV, 0.0); // Visualize UVs
    // finalColor = normal * 0.5 + 0.5; // Visualize World Normals
    // finalColor = normalize(inTangent) * 0.5 + 0.5; // Visualize Tangents

    outFragColor = vec4(finalColor, 1.0);
}

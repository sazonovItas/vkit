#version 450

layout(location = 0) in vec3 inLocalPos;
layout(location = 1) in vec3 inWorldPos;

layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
    vec3 position;
} camera;

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint materialIdx;
} pcs;

const vec3 LIGHT_DIR = -vec3(0.0, 0.0, -1.0);

void main() {
    vec3 ro = camera.position;
    vec3 rd = normalize(inWorldPos - camera.position);

    vec3 center = vec3(pcs.model[3]);

    float scale = length(vec3(pcs.model[0]));

    float radius = scale * 0.5;

    vec3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - (radius * radius);
    float h = b * b - c;

    if (h < 0.0) {
        discard;
    }

    float t = -b - sqrt(h);

    if (t < 0.0) {
        discard;
    }

    vec3 hitPos = ro + rd * t;
    vec3 normal = normalize(hitPos - center);

    float diff = max(dot(normal, LIGHT_DIR), 0.0);

    vec3 albedo = vec3(0.2, 0.5, 0.8);
    vec3 ambient = albedo * 0.1;

    vec3 finalColor = ambient + (albedo * diff);

    vec4 clipPos = camera.proj * camera.view * vec4(hitPos, 1.0);
    gl_FragDepth = clipPos.z / clipPos.w;

    outFragColor = vec4(finalColor, 1.0);
}

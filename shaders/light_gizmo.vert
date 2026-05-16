#version 450

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
    vec3 position;
} camera;

struct Light {
    vec3 position; float range;
    vec3 direction; float intensity;
    vec3 color; int type;
    float innerAngle; float outerAngle;
    int castsShadows; float _pad;
};

layout(std430, set = 0, binding = 3) readonly buffer LightsBlock {
    uint  count;
    uint  _pad0; uint _pad1; uint _pad2;
    Light lights[];
} lightsBlock;

layout(push_constant) uniform PushConstants {
    float size;
} pcs;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outColor;

void main() {
    const vec2 corners[6] = vec2[](
        vec2(-1.0, -1.0), vec2( 1.0, -1.0), vec2( 1.0,  1.0),
        vec2(-1.0, -1.0), vec2( 1.0,  1.0), vec2(-1.0,  1.0)
    );
    vec2 corner = corners[gl_VertexIndex];

    Light li = lightsBlock.lights[gl_InstanceIndex];
    outUV    = corner;
    outColor = li.color * li.intensity;

    vec4 clip = camera.proj * camera.view * vec4(li.position, 1.0);
    // Offset in NDC, corrected for aspect ratio
    float aspect = camera.proj[1][1] / camera.proj[0][0];
    vec2 ndcOffset = corner * pcs.size;
    ndcOffset.x /= aspect;
    gl_Position = vec4(clip.xy + ndcOffset * clip.w, clip.z, clip.w);
}

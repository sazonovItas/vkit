#version 450 core

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
    vec3 position;
} camera;

layout(location = 0) out vec3 outViewDir;

void main() {
    vec2 positions[6] = vec2[](
            vec2(-1.0, -1.0),
            vec2(1.0, -1.0),
            vec2(-1.0, 1.0),
            vec2(-1.0, 1.0),
            vec2(1.0, -1.0),
            vec2(1.0, 1.0)
        );

    vec2 clipPos = positions[gl_VertexIndex];

    mat4 viewNoTranslation = mat4(mat3(camera.view));

    mat4 invProj = inverse(camera.proj);
    mat4 invView = inverse(viewNoTranslation);

    vec4 target = invProj * vec4(clipPos.x, clipPos.y, 1.0, 1.0);
    outViewDir = (invView * vec4(normalize(target.xyz / target.w), 0.0)).xyz;

    gl_Position = vec4(clipPos, 1.0, 1.0);
}

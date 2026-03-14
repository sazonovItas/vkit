#version 450 core

layout(set = 0, binding = 0) uniform UBO {
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) out vec3 outViewDir;

void main() {
    vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    vec2 clipPos = uv * 2.0 - 1.0;

    mat4 viewNoTranslation = mat4(mat3(ubo.view));
    
    mat4 invProj = inverse(ubo.projection);
    mat4 invView = inverse(viewNoTranslation);

    vec4 target = invProj * vec4(clipPos.x, clipPos.y, 1.0, 1.0);
    outViewDir = (invView * vec4(normalize(target.xyz / target.w), 0.0)).xyz;

    gl_Position = vec4(clipPos, 1.0, 1.0);
}

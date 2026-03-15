#version 450 core

layout (set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 cameraPosition;
} ubo;

layout(location = 0) out vec3 outViewDir;

void main() {
    vec2 positions[6] = vec2[](
        vec2(-1.0, -1.0), // Bottom-Left
        vec2( 1.0, -1.0), // Bottom-Right
        vec2(-1.0,  1.0), // Top-Left
        
        vec2(-1.0,  1.0), // Top-Left
        vec2( 1.0, -1.0), // Bottom-Right
        vec2( 1.0,  1.0)  // Top-Right
    );

    vec2 clipPos = positions[gl_VertexIndex];

    mat4 viewNoTranslation = mat4(mat3(ubo.view));
    
    mat4 invProj = inverse(ubo.projection);
    mat4 invView = inverse(viewNoTranslation);

    vec4 target = invProj * vec4(clipPos.x, clipPos.y, 1.0, 1.0);
    outViewDir = (invView * vec4(normalize(target.xyz / target.w), 0.0)).xyz;

    gl_Position = vec4(clipPos, 1.0, 1.0);
}

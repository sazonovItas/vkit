#ifndef BINDLESS_GLSL
#define BINDLESS_GLSL

#extension GL_EXT_nonuniform_qualifier : enable

#define LINEAR_SAMPLER_ID 0
#define NEAREST_SAMPLER_ID 1

layout(set = 2, binding = 0) uniform texture2D textures[];
layout(set = 2, binding = 1) uniform textureCube textureCubes[];
layout(set = 2, binding = 2) uniform sampler samplers[];

vec4 sampleTexture2DLinear(uint texID, vec2 uv) {
    return texture(sampler2D(nonuniformEXT(textures[texID]), samplers[LINEAR_SAMPLER_ID]), uv);
}

vec4 sampleTexture2DNearest(uint texID, vec2 uv) {
    return texture(sampler2D(nonuniformEXT(textures[texID]), samplers[NEAREST_SAMPLER_ID]), uv);
}

vec4 sampleTextureCubeLinear(uint texID, vec3 p) {
    return texture(samplerCube(nonuniformEXT(textureCubes[texID]), samplers[LINEAR_SAMPLER_ID]), p);
}

vec4 sampleTextureCubeNearest(uint texID, vec3 p) {
    return texture(samplerCube(nonuniformEXT(textureCubes[texID]), samplers[NEAREST_SAMPLER_ID]), p);
}

#endif // BINDLESS_GLSL

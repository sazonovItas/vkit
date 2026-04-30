#ifndef BINDLESS_GLSL
#define BINDLESS_GLSL

#extension GL_EXT_nonuniform_qualifier : enable

#define LINEAR_SAMPLER_ID 0
#define NEAREST_SAMPLER_ID 1

layout(set = 1, binding = 0) uniform sampler samplers[];

layout(set = 1, binding = 1) uniform texture2D textures[];
layout(set = 1, binding = 1) uniform texture2DArray textureArrays[];

vec4 sampleTexture2DLinear(uint texID, vec2 uv) {
    return texture(sampler2D(nonuniformEXT(textures[texID]), samplers[LINEAR_SAMPLER_ID]), uv);
}

vec4 sampleTexture2DNearest(uint texID, vec2 uv) {
    return texture(sampler2D(nonuniformEXT(textures[texID]), samplers[NEAREST_SAMPLER_ID]), uv);
}

vec4 sampleTexture2DArrayNearest(uint texID, vec3 uvw) {
    return texture(sampler2DArray(nonuniformEXT(textureArrays[texID]), samplers[NEAREST_SAMPLER_ID]), uvw);
}

vec4 sampleTexture2DArrayLinear(uint texID, vec3 uvw) {
    return texture(sampler2DArray(nonuniformEXT(textureArrays[texID]), samplers[LINEAR_SAMPLER_ID]), uvw);
}

#endif // BINDLESS_GLSL

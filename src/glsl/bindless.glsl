#ifndef BINDLESS_GLSL
#define BINDLESS_GLSL

#extension GL_EXT_nonuniform_qualifier : enable

#define LINEAR_SAMPLER_ID 0
#define NEAREST_SAMPLER_ID 1

#ifndef BINDLESS_SET_IDX
#define BINDLESS_SET_IDX 2
#endif

layout(set = BINDLESS_SET_IDX, binding = 0) uniform sampler samplers[];
layout(set = BINDLESS_SET_IDX, binding = 1) uniform texture2D textures[];
layout(set = BINDLESS_SET_IDX, binding = 2, rgba32f) uniform image2D storageImages[];


vec4 sampleTexture2DLinear(uint texID, vec2 uv) {
    return texture(sampler2D(nonuniformEXT(textures[texID]), samplers[LINEAR_SAMPLER_ID]), uv);
}

vec4 sampleTexture2DNearest(uint texID, vec2 uv) {
    return texture(sampler2D(nonuniformEXT(textures[texID]), samplers[NEAREST_SAMPLER_ID]), uv);
}

vec4 sampleTexture2DLod(uint texID, vec2 uv, float lod) {
    return textureLod(sampler2D(nonuniformEXT(textures[texID]), samplers[LINEAR_SAMPLER_ID]), uv, lod);
}

void writeStorageImage(uint imageID, ivec2 coord, vec4 data) {
    imageStore(storageImages[nonuniformEXT(imageID)], coord, data);
}

vec4 readStorageImage(uint imageID, ivec2 coord) {
    return imageLoad(storageImages[nonuniformEXT(imageID)], coord);
}

#endif // BINDLESS_GLSL

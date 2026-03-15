#version 450 core

#define BINDLESS_SET_IDX 1
#include "bindless.glsl"

#include "skybox_pcs.glsl"

layout(location = 0) in vec3 inViewDir;

layout (set = 0, binding = 1) uniform UBOParams {
    float exposure;
    float gamma;
    float iblIntensity;

    int diffuseEnvMapIdx;
    int specularEnvMapIdx;
    float maxSpecularLod;

    int lightCount;
} uboParams;

layout(location = 0) out vec4 outColor;

const vec2 INV_ATAN = vec2(0.15915494309, 0.31830988618);

vec2 directionToSphericalEnvmap(vec3 dir) {
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= INV_ATAN;
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    if (pcs.envMapIdx == -1) {
        outColor = pcs.envBaseColor;
        return;
    }

    vec3 viewDir = normalize(inViewDir);
    vec2 uv = directionToSphericalEnvmap(viewDir);
    
    vec3 envColor = sampleTexture2DLinear(pcs.envMapIdx, uv).rgb;

    envColor *= uboParams.exposure;

    envColor = ACESFilm(envColor);

    outColor = vec4(envColor, 1.0);
}

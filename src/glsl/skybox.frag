#version 450 core

#include "bindless.glsl"

layout(location = 0) in vec3 inViewDir;

layout (set = 0, binding = 1) uniform UBOParams {
    float exposure;
    float gamma;

    int envMapIdx;
    int pad_;

    vec4 envColor;
} uboParams;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265358979323846;

vec2 directionToSphericalEnvmap(vec3 dir) {
    float s = 1.0 - mod(1.0 / (2.0 * PI) * atan(dir.y, dir.x), 1.0);
    float t = 1.0 / PI * acos(-dir.z);
    return vec2(s, t);
}

void main() {
    if (uboParams.envMapIdx == -1) {
        outColor = uboParams.envColor;
        return;
    }

    vec3 viewDir = normalize(inViewDir);
    vec2 uv = directionToSphericalEnvmap(viewDir);
    
    vec3 envColor = sampleTexture2DLinear(uboParams.envMapIdx, uv).rgb;

    envColor *= uboParams.exposure;
    envColor = pow(envColor, vec3(1.0 / uboParams.gamma));

    outColor = vec4(envColor, 1.0);
}

#ifndef LIGHTS_GLSL
#define LIGHTS_GLSL

#include "light_types.glsl"

// Smooth inverse-square falloff clamped to [0,1] by range.
float lightAttenuation(float dist, float range) {
    if (range <= 0.0) return 1.0;
    float t = clamp(dist / range, 0.0, 1.0);
    return (1.0 - t * t) * (1.0 - t * t);
}

// Computes L (unit vector toward light), lightColor (pre-attenuated), and
// shadowFactor for light[i]. Accesses globals: lightsBlock, shadowMap,
// sceneParams (for shadowViewProj / shadowBias / shadowsEnabled).
void computeLightContrib(uint i, vec3 worldPos,
                         out vec3 L, out vec3 lightColor, out float shadowFactor) {
    Light li = lightsBlock.lights[i];

    float atten = 1.0;

    if (li.type == LIGHT_DIRECTIONAL) {
        L = normalize(-li.direction);
    } else if (li.type == LIGHT_POINT) {
        vec3 d = li.position - worldPos;
        float dist = length(d);
        L = d / max(dist, 0.0001);
        atten = lightAttenuation(dist, li.range);
    } else { // LIGHT_SPOT
        vec3 d = li.position - worldPos;
        float dist = length(d);
        L = d / max(dist, 0.0001);
        atten = lightAttenuation(dist, li.range);
        float cosTheta = dot(-L, normalize(li.direction));
        float cosInner = cos(li.innerAngle);
        float cosOuter = cos(li.outerAngle);
        float spot = clamp((cosTheta - cosOuter) / max(cosInner - cosOuter, 0.0001), 0.0, 1.0);
        atten *= spot * spot;
    }

    lightColor = li.color * li.intensity * atten;

    shadowFactor = 1.0;
    if (li.castsShadows != 0 && sceneParams.shadowsEnabled != 0) {
        vec4 lsPos = sceneParams.shadowViewProj * vec4(worldPos, 1.0);
        shadowFactor = computeShadow(shadowMap, lsPos, sceneParams.shadowBias);
    }
}

#endif // LIGHTS_GLSL

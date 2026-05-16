#ifndef SHADOW_GLSL
#define SHADOW_GLSL

// PCF 3x3 shadow factor. Returns 1.0 when fully lit, 0.0 when fully shadowed.
float computeShadow(sampler2DShadow shadowMap, vec4 lightSpacePos, float bias) {
    // Behind perspective camera (w <= 0) or outside frustum → fully lit.
    if (lightSpacePos.w <= 0.0) return 1.0;
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    if (projCoords.z > 1.0 || projCoords.z < 0.0 ||
        abs(projCoords.x) > 1.0 || abs(projCoords.y) > 1.0) {
        return 1.0;
    }

    vec2 uv = projCoords.xy * 0.5 + 0.5;
    float depth = projCoords.z - bias;

    vec2 texelSize = vec2(1.0) / vec2(textureSize(shadowMap, 0));
    float shadow = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            shadow += texture(shadowMap, vec3(uv + vec2(x, y) * texelSize, depth));
        }
    }
    return shadow / 9.0;
}

#endif // SHADOW_GLSL

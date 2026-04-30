#ifndef IBL_MATH_GLSL
#define IBL_MATH_GLSL

const float PI = 3.14159265358979323846;
const vec2 INV_ATAN = vec2(0.15915494309, 0.31830988618);

vec2 directionToSphericalEnvmap(vec3 dir) {
    vec2 uv = vec2(atan(dir.z, dir.x), asin(clamp(dir.y, -1.0, 1.0)));
    uv *= INV_ATAN;
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

vec3 sphericalEnvmapToDirection(vec2 uv) {
    uv.y = 1.0 - uv.y;
    uv -= 0.5;
    uv /= INV_ATAN;
    float y = sin(uv.y);
    float cosTheta = cos(uv.y);
    float x = cosTheta * cos(uv.x);
    float z = cosTheta * sin(uv.x);
    return normalize(vec3(x, y, z));
}

float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

#endif // IBL_MATH_GLSL

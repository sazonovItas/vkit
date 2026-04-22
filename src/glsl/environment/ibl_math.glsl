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

#endif // IBL_MATH_GLSL

#ifndef RAY_SPHERE_GLSL
#define RAY_SPHERE_GLSL

const float PI_CONST = 3.14159265359;

struct SphereHit {
    vec3 worldPos;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 uv;
};

SphereHit calculateSphereHit(vec3 inLocalPos, vec3 camWorldPos, mat4 model, mat4 view, mat4 proj, uint depthWrite) {
    mat4 invModel = inverse(model);
    vec3 camLocalPos = (invModel * vec4(camWorldPos, 1.0)).xyz;

    vec3 rayOrigin = camLocalPos;
    vec3 rayDir = normalize(inLocalPos - camLocalPos);

    float radius = 0.5;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(rayDir, rayOrigin);
    float c = dot(rayOrigin, rayOrigin) - (radius * radius);

    float discriminant = b * b - 4.0 * a * c;

    if (discriminant < 0.0) {
        discard;
    }

    float t = (-b - sqrt(discriminant)) / (2.0 * a);
    if (t < 0.0) {
        t = (-b + sqrt(discriminant)) / (2.0 * a);
    }

    vec3 hitLocal = rayOrigin + rayDir * t;
    vec4 hitWorld4 = model * vec4(hitLocal, 1.0);

    SphereHit hit;
    hit.worldPos = hitWorld4.xyz;

    vec3 localNormal = normalize(hitLocal);
    mat3 normalMatrix = mat3(model);
    hit.normal = normalize(normalMatrix * localNormal);

    float u = 0.5 + atan(localNormal.z, localNormal.x) / (2.0 * PI_CONST);
    float v = 0.5 - asin(localNormal.y) / PI_CONST;
    hit.uv = vec2(u, v);

    vec3 localTangent = normalize(vec3(-localNormal.z, 0.0, localNormal.x));
    if (abs(localNormal.y) > 0.999) {
        localTangent = vec3(1.0, 0.0, 0.0);
    }
    hit.tangent = normalize(normalMatrix * localTangent);
    hit.bitangent = cross(hit.normal, hit.tangent);

    vec4 clipSpacePos = proj * view * hitWorld4;
    if (depthWrite == 1) {
      gl_FragDepth = clipSpacePos.z / clipSpacePos.w;
    }

    return hit;
}

#endif // RAY_SPHERE_GLSL

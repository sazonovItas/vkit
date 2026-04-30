#ifndef DIFFUSE_SPECULAR_GLSL
#define DIFFUSE_SPECULAR_GLSL

struct DiffuseSpecularParams {
    vec4 diffuseFactor;
    vec3 specularFactor;
    float glossinessFactor;
};

struct DiffuseSpecularTextures {
    int diffuseTexIdx;
    int specularGlossinessTexIdx;
    int normalTexIdx;
    uint padding0;
};

struct DiffuseSpecularData {
    DiffuseSpecularParams params;
    DiffuseSpecularTextures textures;
};

#endif // DIFFUSE_SPECULAR_GLSL

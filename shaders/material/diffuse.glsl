#ifndef DIFFUSE_GLSL
#define DIFFUSE_GLSL

struct DiffuseParams {
    vec4 diffuseFactor;
};

struct DiffuseTextures {
    int diffuseTexIdx;
    int normalTexIdx;
    uint padding0;
    uint padding1;
};

struct DiffuseData {
    DiffuseParams params;
    DiffuseTextures textures;
};

#endif // DIFFUSE_GLSL

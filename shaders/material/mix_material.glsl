#ifndef MIX_MATERIAL_GLSL
#define MIX_MATERIAL_GLSL

struct MixMaterialParams {
    uint  materialIndexA;
    uint  materialIndexB;
    float factor;
    int   factorTexIdx;
    float threshold;
    float edge;
    int   opacityTexIdx;
    float alphaCutoff;
};

struct MixMaterialData {
    MixMaterialParams params;
};

#endif // MIX_MATERIAL_GLSL

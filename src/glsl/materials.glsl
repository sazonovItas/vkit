#ifndef MATERIALS_GLSL
#define MATERIALS_GLSL

#extension GL_EXT_buffer_reference : require

struct Material {
    vec4 base_color;
    vec4 metallic_roughness_emissive;
    uint diffuse_tex;
    uint normal_tex;
    uint metallic_roughness_tex;
    uint emissive_tex;
};

layout (buffer_reference, std430) readonly buffer MaterialsBuffer {
    Material materials[];
} materialsBuffer;

#endif // MATERIALS_GLSL

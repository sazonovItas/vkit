#ifndef LIGHT_TYPES_GLSL
#define LIGHT_TYPES_GLSL

#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT       1
#define LIGHT_SPOT        2

struct Light {
    vec3  position;
    float range;
    vec3  direction;
    float intensity;
    vec3  color;
    int   type;
    float innerAngle;
    float outerAngle;
    int   castsShadows;
    float _pad;
};

#endif // LIGHT_TYPES_GLSL

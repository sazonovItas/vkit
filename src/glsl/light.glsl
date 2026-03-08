#ifndef LIGHT_GLSL
#define LIGHT_GLSL

#define TYPE_DIRECTIONAL_LIGHT 0
#define TYPE_POINT_LIGHT 1
#define TYPE_SPOT_LIGHT 2

struct Light
{
  vec3 position;
  uint type;

  vec3 direction;
  float range;

  vec3 color;
  float intensity;

  vec2 scaleOffset;
  uint shadowMapID;

  float unused;
};

#endif // LIGHT_GLSL

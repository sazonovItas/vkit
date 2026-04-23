#ifndef ENVIRONMENT_LAYOUTS
#define ENVIRONMENT_LAYOUTS

#ifndef ENVIRONMENT_LAYOUTS_SET
#define ENVIRONMENT_LAYOUTS_SET 1
#endif

layout(set = ENVIRONMENT_LAYOUTS_SET, binding = 0) uniform sampler2D      irradianceMap;
layout(set = ENVIRONMENT_LAYOUTS_SET, binding = 1) uniform sampler2DArray prefilterMapArray;
layout(set = ENVIRONMENT_LAYOUTS_SET, binding = 2) uniform Environment { 
  vec3 ambientColor; 
  float intensity; 
  uint hasIrradiance; 
  uint hasPrefilter; 
  uint prefilterLayers; 
} env;

#endif // ENVIRONMENT_LAYOUTS

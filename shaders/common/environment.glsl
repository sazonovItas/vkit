#ifndef ENVIRONMENT_GLSL
#define ENVIRONMENT_GLSL

#define ENV_FEATURE_USE_DIFFUSE  (1u << 0)
#define ENV_FEATURE_USE_SPECULAR (1u << 1)

struct EnvironmentParams {
  int irradianceTexIdx;
  int prefilterTexIdx;
  int brdfLutTexIdx;
  int prefilterNumLayers; 

  float intensity;
  uint  features;
  uint  padding0;
  uint  padding1;
};

#endif // ENVIRONMENT_GLSL

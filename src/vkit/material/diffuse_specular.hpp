#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <string_view>

#include "vkit/material/material.hpp"

namespace vkit::material {

class DiffuseSpecular : public Material {
 public:
  struct Params {
    glm::vec4 diffuseFactor{1.0F, 1.0F, 1.0F, 1.0F};
    alignas(16) glm::vec3 specularFactor{1.0F, 1.0F, 1.0F};
    float glossinessFactor{1.0F};
  };

  struct Textures {
    std::int32_t diffuseTexIdx{-1};
    std::int32_t specularGlossinessTexIdx{-1};
    std::int32_t normalTexIdx{-1};
    std::uint32_t padding0{0};
  };

  struct Data {
    Params params;
    Textures textures;
  };

  explicit DiffuseSpecular(std::string_view name) : Material(name) {}

  Data data;

  [[nodiscard]] auto getType() const -> Type override {
    return Type::kDiffuseSpecular;
  }
};

static_assert(sizeof(DiffuseSpecular::Params) == 32,
              "DiffuseSpecular::Data size must be exactly 32 bytes to match "
              "GLSL std430 alignment.");

static_assert(sizeof(DiffuseSpecular::Textures) == 16,
              "DiffuseSpecular::Textures size must be exactly 16 bytes to "
              "match GLSL std430 alignment.");

static_assert(sizeof(DiffuseSpecular::Data) == 48,
              "DiffuseSpecular::GPUData size must be exactly 48 bytes to match "
              "GLSL std430 alignment.");

};  // namespace vkit::material

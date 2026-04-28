#pragma once

#include <cstdint>
#include <string_view>

#include "vkit/material/material.hpp"

namespace vkit::material {

class Diffuse : public Material {
 public:
  struct Params {
    glm::vec4 diffuseFactor{1.0F, 1.0F, 1.0F, 1.0F};
  };

  struct Textures {
    std::int32_t diffuseTexIdx{-1};
    std::int32_t normalTexIdx{-1};
    std::uint32_t padding0{0};
    std::uint32_t padding1{0};
  };

  struct Data {
    Params params;
    Textures textures;
  };

  explicit Diffuse(std::string_view name) : Material(name) {}

  Data data;

  [[nodiscard]] auto getType() const -> Type override { return Type::kDiffuse; }
};

static_assert(sizeof(Diffuse::Params) == 16,
              "Diffuse::Params size must be exactly 16 bytes to match GLSL "
              "std430 alignment.");

static_assert(sizeof(Diffuse::Textures) == 16,
              "Diffuse::Textures size must be exactly 16 bytes to match GLSL "
              "std430 alignment.");

static_assert(sizeof(Diffuse::Data) == 32,
              "Diffuse::Data size must be exactly 32 bytes to match GLSL "
              "std430 alignment.");

};  // namespace vkit::material

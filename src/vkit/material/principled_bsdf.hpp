#pragma once

#include <cstdint>
#include <string_view>

#include "vkit/material/material.hpp"

namespace vkit::material {

class PrincipledBSDF : public Material {
 public:
  enum class Feature : std::uint32_t {
    kNone = 0,
    kEmissive = (1U << 0),
    kTransmission = (1U << 1),
    kClearcoat = (1U << 2),
    kSheen = (1U << 3),
    kAnisotropy = (1U << 4),
    kIridescence = (1U << 5)
  };

  struct Params {
    glm::vec4 baseColorFactor{1.0F, 1.0F, 1.0F, 1.0F};
    alignas(16) glm::vec3 emissiveFactor{0.0F, 0.0F, 0.0F};
    float metallicFactor{1.0F};

    float roughnessFactor{1.0F};
    float occlusionStrength{1.0F};
    float ior{1.5F};
    std::uint32_t featureMask{0};

    alignas(16) glm::vec3 specularColorFactor{1.0F, 1.0F, 1.0F};
    float specularFactor{1.0F};

    float transmissionFactor{0.0F};
    float thicknessFactor{0.0F};
    float attenuationDistance{0.0F};
    float padding1{0.0F};

    alignas(16) glm::vec3 attenuationColor{1.0F, 1.0F, 1.0F};
    float padding2{0.0F};

    alignas(16) glm::vec3 sheenColorFactor{0.0F, 0.0F, 0.0F};
    float sheenRoughnessFactor{0.0F};

    float clearcoatFactor{0.0F};
    float clearcoatRoughnessFactor{0.0F};
    float anisotropyStrength{0.0F};
    float iridescenceFactor{0.0F};

    glm::vec2 anisotropyRotation{1.0F, 0.0F};
    float iridescenceIor{1.5F};
    float iridescenceThicknessMin{100.0F};
    float iridescenceThicknessMax{400.0F};

    float padding3{0.0F};
    float padding4{0.0F};
    float padding5{0.0F};
  };

  struct Textures {
    std::int32_t baseColorTexIdx{-1};
    std::int32_t normalTexIdx{-1};
    std::int32_t metallicRoughnessTexIdx{-1};
    std::int32_t emissiveTexIdx{-1};

    std::int32_t clearcoatTexIdx{-1};
    std::int32_t clearcoatNormalTexIdx{-1};
    std::int32_t occlusionTexIdx{-1};
    std::int32_t specularTexIdx{-1};

    std::int32_t specularColorTexIdx{-1};
    std::int32_t thicknessTexIdx{-1};
    std::int32_t sheenColorTexIdx{-1};
    std::int32_t sheenRoughnessTexIdx{-1};

    std::int32_t anisotropyTexIdx{-1};
    std::int32_t iridescenceTexIdx{-1};
    std::int32_t iridescenceThicknessTexIdx{-1};

    std::int32_t padding0{0};
  };

  struct Data {
    Params params;
    Textures textures;
  };

  explicit PrincipledBSDF(std::string_view name) : Material(name) {}

  Data data;

  [[nodiscard]] auto getType() const -> Type override {
    return Type::kPrincipledBSDF;
  }

  void enableFeature(Feature feature) {
    data.params.featureMask |= static_cast<std::uint32_t>(feature);
  }

  void disableFeature(Feature feature) {
    data.params.featureMask &= ~static_cast<std::uint32_t>(feature);
  }

  [[nodiscard]] bool hasFeature(Feature feature) const {
    return (data.params.featureMask & static_cast<std::uint32_t>(feature)) != 0;
  }
};

inline auto operator|(PrincipledBSDF::Feature lhs, PrincipledBSDF::Feature rhs)
    -> PrincipledBSDF::Feature {
  return static_cast<PrincipledBSDF::Feature>(static_cast<std::uint32_t>(lhs) |
                                              static_cast<std::uint32_t>(rhs));
}

inline auto operator&(PrincipledBSDF::Feature lhs, PrincipledBSDF::Feature rhs)
    -> PrincipledBSDF::Feature {
  return static_cast<PrincipledBSDF::Feature>(static_cast<std::uint32_t>(lhs) &
                                              static_cast<std::uint32_t>(rhs));
}

inline auto operator|=(PrincipledBSDF::Feature& lhs,
                       PrincipledBSDF::Feature rhs)
    -> PrincipledBSDF::Feature& {
  lhs = lhs | rhs;
  return lhs;
}

static_assert(sizeof(PrincipledBSDF::Params) == 160,
              "PrincipledBSDF::Data size must be exactly 160 bytes to match "
              "GLSL std430 alignment.");

static_assert(sizeof(PrincipledBSDF::Textures) == 64,
              "PrincipledBSDF::Textures size must be exactly 64 bytes to match "
              "GLSL std430 alignment.");

static_assert(sizeof(PrincipledBSDF::Data) == 224,
              "PrincipledBSDF::GPUData size must be exactly 224 bytes to match "
              "GLSL std430 alignment.");

};  // namespace vkit::material

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include "vkit/material/material.hpp"
#include "vkit/texture/texture.hpp"

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

  [[nodiscard]] auto getType() const -> Type override {
    return Type::kPrincipledBSDF;
  }

  Params params;

  void setBaseColorTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    baseColorTexture_ = std::move(tex);
  }
  void setNormalTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    normalTexture_ = std::move(tex);
  }
  void setMetallicRoughnessTexture(
      std::shared_ptr<vkit::texture::Texture> tex) {
    metallicRoughnessTexture_ = std::move(tex);
  }
  void setEmissiveTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    emissiveTexture_ = std::move(tex);
  }
  void setClearcoatTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    clearcoatTexture_ = std::move(tex);
  }
  void setClearcoatNormalTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    clearcoatNormalTexture_ = std::move(tex);
  }
  void setOcclusionTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    occlusionTexture_ = std::move(tex);
  }
  void setSpecularTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    specularTexture_ = std::move(tex);
  }
  void setSpecularColorTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    specularColorTexture_ = std::move(tex);
  }
  void setThicknessTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    thicknessTexture_ = std::move(tex);
  }
  void setSheenColorTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    sheenColorTexture_ = std::move(tex);
  }
  void setSheenRoughnessTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    sheenRoughnessTexture_ = std::move(tex);
  }
  void setAnisotropyTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    anisotropyTexture_ = std::move(tex);
  }
  void setIridescenceTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    iridescenceTexture_ = std::move(tex);
  }
  void setIridescenceThicknessTexture(
      std::shared_ptr<vkit::texture::Texture> tex) {
    iridescenceThicknessTexture_ = std::move(tex);
  }

  [[nodiscard]] auto getBaseColorTexture() const { return baseColorTexture_; }
  [[nodiscard]] auto getNormalTexture() const { return normalTexture_; }
  [[nodiscard]] auto getMetallicRoughnessTexture() const {
    return metallicRoughnessTexture_;
  }
  [[nodiscard]] auto getEmissiveTexture() const { return emissiveTexture_; }
  [[nodiscard]] auto getClearcoatTexture() const { return clearcoatTexture_; }
  [[nodiscard]] auto getClearcoatNormalTexture() const {
    return clearcoatNormalTexture_;
  }
  [[nodiscard]] auto getOcclusionTexture() const { return occlusionTexture_; }
  [[nodiscard]] auto getSpecularTexture() const { return specularTexture_; }
  [[nodiscard]] auto getSpecularColorTexture() const {
    return specularColorTexture_;
  }
  [[nodiscard]] auto getThicknessTexture() const { return thicknessTexture_; }
  [[nodiscard]] auto getSheenColorTexture() const { return sheenColorTexture_; }
  [[nodiscard]] auto getSheenRoughnessTexture() const {
    return sheenRoughnessTexture_;
  }
  [[nodiscard]] auto getAnisotropyTexture() const { return anisotropyTexture_; }
  [[nodiscard]] auto getIridescenceTexture() const {
    return iridescenceTexture_;
  }
  [[nodiscard]] auto getIridescenceThicknessTexture() const {
    return iridescenceThicknessTexture_;
  }

  [[nodiscard]] auto getData() const -> Data {
    Data d;
    d.params = params;

    auto resolve =
        [](const std::shared_ptr<vkit::texture::Texture>& tex) -> std::int32_t {
      return tex ? static_cast<std::int32_t>(tex->getBindlessId().value_or(-1))
                 : -1;
    };

    d.textures.baseColorTexIdx = resolve(baseColorTexture_);
    d.textures.normalTexIdx = resolve(normalTexture_);
    d.textures.metallicRoughnessTexIdx = resolve(metallicRoughnessTexture_);
    d.textures.emissiveTexIdx = resolve(emissiveTexture_);
    d.textures.clearcoatTexIdx = resolve(clearcoatTexture_);
    d.textures.clearcoatNormalTexIdx = resolve(clearcoatNormalTexture_);
    d.textures.occlusionTexIdx = resolve(occlusionTexture_);
    d.textures.specularTexIdx = resolve(specularTexture_);
    d.textures.specularColorTexIdx = resolve(specularColorTexture_);
    d.textures.thicknessTexIdx = resolve(thicknessTexture_);
    d.textures.sheenColorTexIdx = resolve(sheenColorTexture_);
    d.textures.sheenRoughnessTexIdx = resolve(sheenRoughnessTexture_);
    d.textures.anisotropyTexIdx = resolve(anisotropyTexture_);
    d.textures.iridescenceTexIdx = resolve(iridescenceTexture_);
    d.textures.iridescenceThicknessTexIdx =
        resolve(iridescenceThicknessTexture_);
    d.textures.padding0 = 0;

    return d;
  }

  void enableFeature(Feature feature) {
    params.featureMask |= static_cast<std::uint32_t>(feature);
  }

  void disableFeature(Feature feature) {
    params.featureMask &= ~static_cast<std::uint32_t>(feature);
  }

  [[nodiscard]] bool hasFeature(Feature feature) const {
    return (params.featureMask & static_cast<std::uint32_t>(feature)) != 0;
  }

 private:
  std::shared_ptr<vkit::texture::Texture> baseColorTexture_;
  std::shared_ptr<vkit::texture::Texture> normalTexture_;
  std::shared_ptr<vkit::texture::Texture> metallicRoughnessTexture_;
  std::shared_ptr<vkit::texture::Texture> emissiveTexture_;
  std::shared_ptr<vkit::texture::Texture> clearcoatTexture_;
  std::shared_ptr<vkit::texture::Texture> clearcoatNormalTexture_;
  std::shared_ptr<vkit::texture::Texture> occlusionTexture_;
  std::shared_ptr<vkit::texture::Texture> specularTexture_;
  std::shared_ptr<vkit::texture::Texture> specularColorTexture_;
  std::shared_ptr<vkit::texture::Texture> thicknessTexture_;
  std::shared_ptr<vkit::texture::Texture> sheenColorTexture_;
  std::shared_ptr<vkit::texture::Texture> sheenRoughnessTexture_;
  std::shared_ptr<vkit::texture::Texture> anisotropyTexture_;
  std::shared_ptr<vkit::texture::Texture> iridescenceTexture_;
  std::shared_ptr<vkit::texture::Texture> iridescenceThicknessTexture_;
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
              "PrincipledBSDF::Params size must be exactly 160 bytes to match "
              "GLSL std430 alignment.");

static_assert(sizeof(PrincipledBSDF::Textures) == 64,
              "PrincipledBSDF::Textures size must be exactly 64 bytes to match "
              "GLSL std430 alignment.");

static_assert(sizeof(PrincipledBSDF::Data) == 224,
              "PrincipledBSDF::Data size must be exactly 224 bytes to match "
              "GLSL std430 alignment.");

};  // namespace vkit::material

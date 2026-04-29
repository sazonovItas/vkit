#pragma once

#include <memory>
#include <optional>

#include "vkit/material/material.hpp"
#include "vkit/texture/texture.hpp"

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

  Params params;

  [[nodiscard]] auto getType() const -> Type override {
    return Type::kDiffuseSpecular;
  }

  void setDiffuseTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    diffuseTexture_ = std::move(tex);
  }
  void setSpecularGlossinessTexture(
      std::shared_ptr<vkit::texture::Texture> tex) {
    specularGlossinessTexture_ = std::move(tex);
  }
  void setNormalTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    normalTexture_ = std::move(tex);
  }

  [[nodiscard]] auto getDiffuseTexture() const { return diffuseTexture_; }
  [[nodiscard]] auto getSpecularGlossinessTexture() const {
    return specularGlossinessTexture_;
  }
  [[nodiscard]] auto getNormalTexture() const { return normalTexture_; }

  [[nodiscard]] auto getData() const -> Data {
    Data d;
    d.params = params;

    auto resolve =
        [](const std::shared_ptr<vkit::texture::Texture>& tex) -> std::int32_t {
      return tex ? static_cast<std::int32_t>(tex->getBindlessId().value_or(-1))
                 : -1;
    };

    d.textures.diffuseTexIdx = resolve(diffuseTexture_);
    d.textures.specularGlossinessTexIdx = resolve(specularGlossinessTexture_);
    d.textures.normalTexIdx = resolve(normalTexture_);
    d.textures.padding0 = 0;

    return d;
  }

 private:
  std::shared_ptr<vkit::texture::Texture> diffuseTexture_;
  std::shared_ptr<vkit::texture::Texture> specularGlossinessTexture_;
  std::shared_ptr<vkit::texture::Texture> normalTexture_;
};

static_assert(sizeof(DiffuseSpecular::Params) == 32,
              "DiffuseSpecular::Params size must be exactly 32 bytes to match "
              "GLSL std430 alignment.");

static_assert(sizeof(DiffuseSpecular::Textures) == 16,
              "DiffuseSpecular::Textures size must be exactly 16 bytes to "
              "match GLSL std430 alignment.");

static_assert(sizeof(DiffuseSpecular::Data) == 48,
              "DiffuseSpecular::Data size must be exactly 48 bytes to match "
              "GLSL std430 alignment.");

};  // namespace vkit::material

#pragma once

#include <memory>
#include <optional>

#include "vkit/material/material.hpp"
#include "vkit/texture/texture.hpp"

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

  [[nodiscard]] auto getType() const -> Type override { return Type::kDiffuse; }

  Params params;

  void setDiffuseTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    diffuseTexture_ = std::move(tex);
  }
  void setNormalTexture(std::shared_ptr<vkit::texture::Texture> tex) {
    normalTexture_ = std::move(tex);
  }

  [[nodiscard]] auto getDiffuseTexture() const
      -> std::shared_ptr<vkit::texture::Texture> {
    return diffuseTexture_;
  }
  [[nodiscard]] auto getNormalTexture() const
      -> std::shared_ptr<vkit::texture::Texture> {
    return normalTexture_;
  }

  [[nodiscard]] auto getData() const -> Data {
    Data d;
    d.params = params;

    d.textures.diffuseTexIdx =
        diffuseTexture_ ? static_cast<int32_t>(
                              diffuseTexture_->getBindlessId().value_or(-1))
                        : -1;

    d.textures.normalTexIdx =
        normalTexture_
            ? static_cast<int32_t>(normalTexture_->getBindlessId().value_or(-1))
            : -1;

    return d;
  }

 private:
  std::shared_ptr<texture::Texture> diffuseTexture_;
  std::shared_ptr<texture::Texture> normalTexture_;
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

#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "vkit/material/material.hpp"
#include "vkit/texture/texture.hpp"

namespace vkit::material {

class MixMaterial : public Material {
 public:
  struct Params {
    std::uint32_t materialIndexA{0};
    std::uint32_t materialIndexB{0};
    float factor{0.5F};
    std::int32_t factorTexIdx{-1};
    float threshold{0.5F};
    float edge{1.0F};
    std::int32_t opacityTexIdx{-1};
    float alphaCutoff{0.01F};
  };
  static_assert(sizeof(Params) == 32);

  struct Data {
    Params params;
  };
  static_assert(sizeof(Data) == 32);

  explicit MixMaterial(std::string_view name) : Material(name) {}

  [[nodiscard]] auto getType() const -> Type override { return Type::kMix; }

  Params params;

  void setFactorTexture(const std::shared_ptr<vkit::texture::Texture>& tex) {
    factorTexture_ = tex;
    setDirty(true);
  }
  void setOpacityTexture(const std::shared_ptr<vkit::texture::Texture>& tex) {
    opacityTexture_ = tex;
    setDirty(true);
  }

  [[nodiscard]] auto getData() const -> Data {
    Data d;
    d.params = params;
    d.params.factorTexIdx = factorTexture_
        ? static_cast<std::int32_t>(factorTexture_->getBindlessId().value_or(-1))
        : -1;
    d.params.opacityTexIdx = opacityTexture_
        ? static_cast<std::int32_t>(opacityTexture_->getBindlessId().value_or(-1))
        : -1;
    return d;
  }

  void setHasTransmission(bool v) { hasTransmission_ = v; }
  [[nodiscard]] bool hasTransmission() const { return hasTransmission_; }
  [[nodiscard]] bool hasOpacityMap() const { return opacityTexture_ != nullptr; }

 private:
  std::shared_ptr<vkit::texture::Texture> factorTexture_;
  std::shared_ptr<vkit::texture::Texture> opacityTexture_;
  bool hasTransmission_{false};
};

}  // namespace vkit::material

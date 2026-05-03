#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include "vkit/item/item.hpp"
#include "vkit/item/storage_item.hpp"
#include "vkit/texture/texture.hpp"

namespace vkit::env {

struct EnvironmentParams {
  int irradianceTexIdx;
  int prefilterTexIdx;
  int brdfLutTexIdx;
  int prefilterNumLayers;

  float intensity;
  std::uint32_t features;
  std::uint32_t padding0;
  std::uint32_t padding1;
};

enum class EnvironmentFeature : std::uint32_t {
  kNone = 0,
  kUseDiffuse = (1U << 0),
  kUseSpecular = (1U << 1),
  kAll = kUseDiffuse | kUseSpecular
};

class Environment : public Item<Environment>, public StorageItem {
 public:
  explicit Environment(
      std::string_view name,
      const std::shared_ptr<texture::Texture>& brdfLutTexture,
      const std::shared_ptr<texture::Texture>& irradianceTexture,
      const std::shared_ptr<texture::Texture>& prefilterTexture)
      : Item<Environment>{name},
        brdfLutTexture{brdfLutTexture},
        irradianceTexture{irradianceTexture},
        prefilterTexture{prefilterTexture},
        intensity{1.0F},
        features{static_cast<std::uint32_t>(EnvironmentFeature::kAll)} {}

  std::shared_ptr<texture::Texture> brdfLutTexture;
  std::shared_ptr<texture::Texture> irradianceTexture;
  std::shared_ptr<texture::Texture> prefilterTexture;

  float intensity{};
  float blurLevel{};
  std::uint32_t features{};

  [[nodiscard]] auto getData() const -> EnvironmentParams {
    return EnvironmentParams{
        .irradianceTexIdx =
            static_cast<int>(irradianceTexture->getBindlessId().value_or(-1)),
        .prefilterTexIdx =
            static_cast<int>(prefilterTexture->getBindlessId().value_or(-1)),
        .brdfLutTexIdx =
            static_cast<int>(brdfLutTexture->getBindlessId().value_or(-1)),
        .prefilterNumLayers =
            prefilterTexture->getGraphicsTexture()->getArrayLayerCount(),
        .intensity = intensity,
        .features = features,
        .padding0 = 0,
        .padding1 = 0,
    };
  }

  [[nodiscard]] auto getStorageId() const -> std::optional<std::uint32_t> {
    return storageId_;
  }
  void setStorageId(std::optional<std::uint32_t> id) { storageId_ = id; }

 private:
  std::optional<std::uint32_t> storageId_;
};

};  // namespace vkit::env

#pragma once

#include <glm/glm.hpp>
#include <string_view>

#include "vkit/graphics/texture.hpp"
#include "vkit/scene/item.hpp"

namespace vkit::scene {

enum class AlphaMode { kOpaque, kMask, kBlend };

class Material : public Item {
  using TextureBinding = graphics::TextureBinding;

 public:
  explicit Material(std::string_view name) : Item(name) {}

  bool doubleSided{false};
  AlphaMode alphaMode{AlphaMode::kOpaque};
  float alphaCutoff{0.5F};

  glm::vec4 baseColorFactor{1.0F, 1.0F, 1.0F, 1.0F};
  glm::vec3 emissiveFactor{0.0F, 0.0F, 0.0F};
  float metallicFactor{1.0F};
  float roughnessFactor{1.0F};

  float ior{1.5F};
  float clearcoatFactor{0.0F};
  float clearcoatRoughnessFactor{0.0F};
  float transmissionFactor{0.0F};

  std::optional<TextureBinding> baseColorMap;
  std::optional<TextureBinding> normalMap;
  std::optional<TextureBinding> metallicRoughnessMap;
  std::optional<TextureBinding> emissiveMap;
  std::optional<TextureBinding> clearcoatMap;
  std::optional<TextureBinding> clearcoatNormalMap;
};

};  // namespace vkit::scene

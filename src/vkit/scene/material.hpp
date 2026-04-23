#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <string_view>

#include "vkit/graphics/texture.hpp"
#include "vkit/scene/item.hpp"

namespace vkit::scene {

enum class AlphaMode { kOpaque, kMask, kBlend };

class BsdfMaterial : public Item {
  using TextureBinding = graphics::TextureBinding;

 public:
  explicit BsdfMaterial(std::string_view name) : Item(name) {}

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

  float occlusionStrength{1.0F};
  float specularFactor{1.0F};
  glm::vec3 specularColorFactor{1.0F, 1.0F, 1.0F};

  float thicknessFactor{0.0F};
  glm::vec3 attenuationColor{1.0F, 1.0F, 1.0F};
  float attenuationDistance{0.0F};

  glm::vec3 sheenColorFactor{0.0F, 0.0F, 0.0F};
  float sheenRoughnessFactor{0.0F};

  float anisotropyStrength{0.0F};
  glm::vec2 anisotropyRotation{1.0F, 0.0F};

  float iridescenceFactor{0.0F};
  float iridescenceIor{1.3F};
  float iridescenceThicknessMin{100.0F};
  float iridescenceThicknessMax{400.0F};

  // --- Texture Bindings ---
  std::optional<TextureBinding> baseColorMap;
  std::optional<TextureBinding> normalMap;
  std::optional<TextureBinding> metallicRoughnessMap;
  std::optional<TextureBinding> emissiveMap;

  std::optional<TextureBinding> clearcoatMap;
  std::optional<TextureBinding> clearcoatNormalMap;

  std::optional<TextureBinding> occlusionMap;
  std::optional<TextureBinding> specularMap;
  std::optional<TextureBinding> specularColorMap;

  std::optional<TextureBinding> thicknessMap;

  std::optional<TextureBinding> sheenColorMap;
  std::optional<TextureBinding> sheenRoughnessMap;

  std::optional<TextureBinding> anisotropyMap;

  std::optional<TextureBinding> iridescenceMap;
  std::optional<TextureBinding> iridescenceThicknessMap;
};

}  // namespace vkit::scene

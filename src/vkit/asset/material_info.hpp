#pragma once

#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <limits>
#include <optional>
#include <string>

namespace vkit::asset {

struct GltfTextureRef {
  std::filesystem::path path;
};

struct GltfMaterialInfo {
  std::string name;

  // Alpha: 0=Opaque, 1=Mask, 2=Blend (matches material::AlphaMode values)
  std::uint32_t alphaMode{0};
  float alphaCutoff{0.5f};
  bool doubleSided{false};

  // ── Base PBR ─────────────────────────────────────────────────────────────
  std::optional<GltfTextureRef> baseColorTexture;
  std::optional<GltfTextureRef> normalTexture;
  std::optional<GltfTextureRef> metallicRoughnessTexture;
  std::optional<GltfTextureRef> emissiveTexture;
  std::optional<GltfTextureRef> occlusionTexture;

  glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
  float metallicFactor{0.0f};
  float roughnessFactor{0.5f};
  glm::vec3 emissiveFactor{0.0f, 0.0f, 0.0f};  // already multiplied by emissiveStrength
  float occlusionStrength{1.0f};

  // ── KHR_materials_ior ────────────────────────────────────────────────────
  float ior{1.5f};

  // ── KHR_materials_specular ───────────────────────────────────────────────
  std::optional<GltfTextureRef> specularTexture;
  std::optional<GltfTextureRef> specularColorTexture;
  float specularFactor{1.0f};
  glm::vec3 specularColorFactor{1.0f, 1.0f, 1.0f};

  // ── KHR_materials_transmission ───────────────────────────────────────────
  float transmissionFactor{0.0f};

  // ── KHR_materials_volume ─────────────────────────────────────────────────
  std::optional<GltfTextureRef> thicknessTexture;
  float thicknessFactor{0.0f};
  float attenuationDistance{std::numeric_limits<float>::infinity()};
  glm::vec3 attenuationColor{1.0f, 1.0f, 1.0f};

  // ── KHR_materials_clearcoat ──────────────────────────────────────────────
  std::optional<GltfTextureRef> clearcoatTexture;
  std::optional<GltfTextureRef> clearcoatRoughnessTexture;
  std::optional<GltfTextureRef> clearcoatNormalTexture;
  float clearcoatFactor{0.0f};
  float clearcoatRoughnessFactor{0.0f};

  // ── KHR_materials_sheen ──────────────────────────────────────────────────
  std::optional<GltfTextureRef> sheenColorTexture;
  std::optional<GltfTextureRef> sheenRoughnessTexture;
  glm::vec3 sheenColorFactor{0.0f, 0.0f, 0.0f};
  float sheenRoughnessFactor{0.0f};

  // ── KHR_materials_anisotropy ─────────────────────────────────────────────
  std::optional<GltfTextureRef> anisotropyTexture;
  float anisotropyStrength{0.0f};
  glm::vec2 anisotropyRotation{1.0f, 0.0f};  // stored as (cos, sin) of the angle

  // ── KHR_materials_iridescence ────────────────────────────────────────────
  std::optional<GltfTextureRef> iridescenceTexture;
  std::optional<GltfTextureRef> iridescenceThicknessTexture;
  float iridescenceFactor{0.0f};
  float iridescenceIor{1.3f};
  float iridescenceThicknessMin{100.0f};
  float iridescenceThicknessMax{400.0f};
};

};  // namespace vkit::asset

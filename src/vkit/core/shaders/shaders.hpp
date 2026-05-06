#pragma once

#include "vkit/asset/util.hpp"

namespace vkit::shaders {

[[nodiscard]] inline auto shaderPath(std::string_view uri)
    -> std::filesystem::path {
  return asset::assetPath(uri);
}

static constexpr std::string_view kImguiVertShaderPath = "shaders/imgui.vert";
static constexpr std::string_view kImguiFragShaderPath = "shaders/imgui.frag";

static constexpr std::string_view kRaySphereVertShaderPath =
    "shaders/ray_sphere.vert";
static constexpr std::string_view kRaySphereDebugFragShaderPath =
    "shaders/ray_sphere_debug.frag";
static constexpr std::string_view kRaySphereMaterialFragShaderPath =
    "shaders/ray_sphere_material.frag";

static constexpr std::string_view kPrimitiveVertShaderPath =
    "shaders/primitive.vert";
static constexpr std::string_view kPrimitiveDebugFragShaderPath =
    "shaders/primitive_debug.frag";
static constexpr std::string_view kPrimitiveMaterialFragShaderPath =
    "shaders/primitive_material.frag";

static constexpr std::string_view kSkyboxVertShaderPath = "shaders/skybox.vert";
static constexpr std::string_view kSkyboxFragShaderPath = "shaders/skybox.frag";

static constexpr std::string_view kIblBrdfLutShaderPath =
    "shaders/ibl/brdf_lut.comp";
static constexpr std::string_view kIblDiffuseShaderPath =
    "shaders/ibl/diffuse_ibl.comp";
static constexpr std::string_view kIblSpecularShaderPath =
    "shaders/ibl/specular_ibl.comp";

static constexpr std::string_view kProceduralNoiceShaderPath =
    "shaders/procedural/noise.comp";
static constexpr std::string_view kProceduralPatternShaderPath =
    "shaders/procedural/pattern.comp";
static constexpr std::string_view kProceduralFractalShaderPath =
    "shaders/procedural/fractal.comp";

static constexpr std::string_view kOperatorsSobelShaderPath =
    "shaders/operators/sobel.comp";
static constexpr std::string_view kOperatorsHeightMapShaderPath =
    "shaders/operators/heightmap.comp";
static constexpr std::string_view kOperatorsNormalMapShaderPath =
    "shaders/operators/normalmap.comp";
static constexpr std::string_view kOperatorsTintShaderPath =
    "shaders/operators/tint.comp";
static constexpr std::string_view kOperatorsMixShaderPath =
    "shaders/operators/mix.comp";

};  // namespace vkit::shaders

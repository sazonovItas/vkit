#pragma once

namespace vkit::asset {

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
static constexpr std::string_view kPrimitiveDebugVertShaderPath =
    "shaders/primitive_debug.vert";
static constexpr std::string_view kPrimitiveDebugFragShaderPath =
    "shaders/primitive_debug.frag";
static constexpr std::string_view kPrimitiveMaterialFragShaderPath =
    "shaders/primitive_material.frag";

static constexpr std::string_view kProceduralNoiceShaderPath =
    "shaders/procedural/noise.comp";

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

};  // namespace vkit::asset

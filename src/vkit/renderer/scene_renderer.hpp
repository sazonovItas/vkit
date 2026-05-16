#pragma once

#include <memory>
#include <vector>

#include "vkit/environment/environment.hpp"
#include "vkit/graphics/descriptor_buffer.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/graphics/image.hpp"
#include "vkit/renderer/descriptor_set_layout/bindless.hpp"
#include "vkit/renderer/descriptor_set_layout/light.hpp"
#include "vkit/renderer/descriptor_set_layout/material.hpp"
#include "vkit/renderer/descriptor_set_layout/primitive.hpp"
#include "vkit/renderer/descriptor_set_layout/scene.hpp"
#include "vkit/renderer/pipeline_layout/light_gizmo.hpp"
#include "vkit/renderer/pipeline_layout/primitive_material.hpp"
#include "vkit/renderer/pipeline_layout/ray_sphere_material.hpp"
#include "vkit/renderer/pipeline_layout/shadow.hpp"
#include "vkit/renderer/pipeline_layout/skybox.hpp"
#include "vkit/renderer/types.hpp"

namespace vkit::renderer {

static constexpr std::uint32_t kShadowMapSize = 2048;

class SceneRenderer {
 public:
  SceneRenderer(graphics::GfxDevice& device,
                const dsl::BindlessTextureSetLayout& bindlessLayout,
                std::uint32_t framesInFlight);

  void updateUniforms(std::uint32_t frameIndex,
                      const types::CameraUBO& sceneCam,
                      const types::CameraUBO& matCam,
                      const env::EnvironmentParams& envParams,
                      const types::SceneParamsUBO& sceneParams,
                      const types::LightsSSBO& sceneLights,
                      const types::LightsSSBO& matLights,
                      const types::ShadowLightUBO& shadowLight);

  [[nodiscard]] auto getMaterialSetLayout() const
      -> const dsl::MaterialSetLayout& {
    return *materialSetLayout_;
  }
  [[nodiscard]] auto getPrimitiveSetLayout() const
      -> const dsl::PrimitiveSetLayout& {
    return *primitiveSetLayout_;
  }

  [[nodiscard]] auto getSceneDescriptorSet(std::uint32_t frameIndex) const
      -> vk::DescriptorSet {
    return frames_[frameIndex].sceneDescriptorSet;
  }
  [[nodiscard]] auto getMaterialDescriptorSet(std::uint32_t frameIndex) const
      -> vk::DescriptorSet {
    return frames_[frameIndex].materialDescriptorSet;
  }
  [[nodiscard]] auto getLightDescriptorSet(std::uint32_t frameIndex) const
      -> vk::DescriptorSet {
    return frames_[frameIndex].lightDescriptorSet;
  }

  [[nodiscard]] auto getShadowImage() const -> vk::Image {
    return static_cast<vk::Image>(shadowImage_);
  }
  [[nodiscard]] auto getShadowImageView() const -> vk::ImageView {
    return *shadowView_;
  }
  [[nodiscard]] auto getShadowExtent() const -> vk::Extent2D {
    return {kShadowMapSize, kShadowMapSize};
  }

  vk::Pipeline opaquePipeline;
  vk::Pipeline transparentPipeline;
  vk::Pipeline transparentBackFacePipeline;
  vk::Pipeline opaqueRaySpherePipeline;
  vk::Pipeline transparentRaySpherePipeline;
  vk::Pipeline transparentRaySphereBackFacePipeline;
  vk::Pipeline skyboxPipeline;
  vk::Pipeline shadowPipeline;
  vk::Pipeline lightGizmoPipeline;
  vk::Pipeline outlinePipeline;

  pl::PrimitiveMaterialPipelineLayout* primitiveLayout{nullptr};
  pl::RaySphereMaterialPipelineLayout* raySphereLayout{nullptr};
  pl::SkyboxPipelineLayout* skyboxLayout{nullptr};
  pl::ShadowPipelineLayout* shadowLayout{nullptr};
  pl::LightGizmoPipelineLayout* lightGizmoLayout{nullptr};

 private:
  graphics::GfxDevice& device_;

  std::unique_ptr<dsl::SceneSetLayout> sceneSetLayout_;
  std::unique_ptr<dsl::PrimitiveSetLayout> primitiveSetLayout_;
  std::unique_ptr<dsl::MaterialSetLayout> materialSetLayout_;
  std::unique_ptr<dsl::LightSetLayout> lightSetLayout_;

  std::unique_ptr<pl::PrimitiveMaterialPipelineLayout> uPrimLayout_;
  std::unique_ptr<pl::RaySphereMaterialPipelineLayout> uRaySphereLayout_;
  std::unique_ptr<pl::SkyboxPipelineLayout> uSkyboxLayout_;
  std::unique_ptr<pl::ShadowPipelineLayout> uShadowLayout_;
  std::unique_ptr<pl::LightGizmoPipelineLayout> uLightGizmoLayout_;

  vk::UniquePipeline uOpaquePipeline_;
  vk::UniquePipeline uTransparentPipeline_;
  vk::UniquePipeline uTransparentBackFacePipeline_;
  vk::UniquePipeline uRaySphereOpaquePipeline_;
  vk::UniquePipeline uRaySphereTransparentPipeline_;
  vk::UniquePipeline uRaySphereTransparentBackFacePipeline_;
  vk::UniquePipeline uSkyboxPipeline_;
  vk::UniquePipeline uShadowPipeline_;
  vk::UniquePipeline uLightGizmoPipeline_;
  vk::UniquePipeline uOutlinePipeline_;

  graphics::AllocatedImage shadowImage_;
  vk::UniqueImageView shadowView_;
  vk::UniqueSampler shadowSampler_;

  vk::UniqueDescriptorPool descriptorPool_;

  struct Frame {
    std::unique_ptr<graphics::DescriptorBuffer> sceneCameraBuffer;
    std::unique_ptr<graphics::DescriptorBuffer> materialCameraBuffer;
    std::unique_ptr<graphics::DescriptorBuffer> environmentBuffer;
    std::unique_ptr<graphics::DescriptorBuffer> sceneParamsBuffer;
    std::unique_ptr<graphics::DescriptorBuffer> sceneLightsBuffer;
    std::unique_ptr<graphics::DescriptorBuffer> matLightsBuffer;
    std::unique_ptr<graphics::DescriptorBuffer> shadowLightBuffer;
    vk::DescriptorSet sceneDescriptorSet;
    vk::DescriptorSet materialDescriptorSet;
    vk::DescriptorSet lightDescriptorSet;
  };
  std::vector<Frame> frames_;
};

};  // namespace vkit::renderer

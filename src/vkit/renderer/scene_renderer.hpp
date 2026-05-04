#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "vkit/environment/environment.hpp"
#include "vkit/graphics/descriptor_buffer.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/renderer/descriptor_set_layout/bindless.hpp"
#include "vkit/renderer/descriptor_set_layout/material.hpp"
#include "vkit/renderer/descriptor_set_layout/primitive.hpp"
#include "vkit/renderer/descriptor_set_layout/scene.hpp"
#include "vkit/renderer/pipeline_layout/primitive_material.hpp"
#include "vkit/renderer/pipeline_layout/ray_sphere_debug.hpp"
#include "vkit/renderer/pipeline_layout/skybox.hpp"
#include "vkit/renderer/types.hpp"

namespace vkit::renderer {

class SceneRenderer {
 public:
  SceneRenderer(graphics::GfxDevice& device,
                const dsl::BindlessTextureSetLayout& bindlessLayout,
                std::uint32_t framesInFlight);

  void updateUniforms(std::uint32_t frameIndex,
                      const types::CameraUBO& sceneCam,
                      const types::CameraUBO& matCam,
                      const env::EnvironmentParams& envParams);

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

  vk::Pipeline opaquePipeline;
  vk::Pipeline transparentPipeline;
  vk::Pipeline raySpherePipeline;
  vk::Pipeline skyboxPipeline;

  pl::PrimitiveMaterialPipelineLayout* primitiveLayout{nullptr};
  pl::RaySphereDebugPipelineLayout* raySphereLayout{nullptr};
  pl::SkyboxPipelineLayout* skyboxLayout{nullptr};

 private:
  graphics::GfxDevice& device_;

  std::unique_ptr<dsl::SceneSetLayout> sceneSetLayout_;
  std::unique_ptr<dsl::PrimitiveSetLayout> primitiveSetLayout_;
  std::unique_ptr<dsl::MaterialSetLayout> materialSetLayout_;

  std::unique_ptr<pl::PrimitiveMaterialPipelineLayout> uPrimLayout_;
  std::unique_ptr<pl::RaySphereDebugPipelineLayout> uRaySphereLayout_;
  std::unique_ptr<pl::SkyboxPipelineLayout> uSkyboxLayout_;

  vk::UniquePipeline uOpaquePipeline_;
  vk::UniquePipeline uTransparentPipeline_;
  vk::UniquePipeline uRaySpherePipeline_;
  vk::UniquePipeline uSkyboxPipeline_;

  vk::UniqueDescriptorPool descriptorPool_;

  struct Frame {
    std::unique_ptr<graphics::DescriptorBuffer> sceneCameraBuffer;
    std::unique_ptr<graphics::DescriptorBuffer> materialCameraBuffer;
    std::unique_ptr<graphics::DescriptorBuffer> environmentBuffer;
    vk::DescriptorSet sceneDescriptorSet;
    vk::DescriptorSet materialDescriptorSet;
  };
  std::vector<Frame> frames_;
};

};  // namespace vkit::renderer

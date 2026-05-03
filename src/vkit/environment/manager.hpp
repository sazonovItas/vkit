#pragma once

#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

#include "vkit/compute/async_compute.hpp"
#include "vkit/compute/descriptor_set_layout/ibl.hpp"
#include "vkit/compute/pipeline_layout/ibl.hpp"
#include "vkit/environment/environment.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/item/storage.hpp"
#include "vkit/texture/manager.hpp"

namespace vkit::env {

class EnvironmentManager {
 public:
  EnvironmentManager(graphics::GfxDevice& device,
                     std::shared_ptr<texture::TextureManager> textureManager,
                     std::shared_ptr<compute::AsyncCompute> asyncCompute);

  void initializeGlobalBrdfLut();

  [[nodiscard]] auto promptAndLoadEnvironment() -> std::optional<std::uint32_t>;
  [[nodiscard]] auto loadEnvironment(const std::filesystem::path& filepath)
      -> std::optional<std::uint32_t>;

  [[nodiscard]] auto generateEnvironment(
      std::string_view name,
      const std::shared_ptr<graphics::Texture>& sourceEnvMap) -> std::uint32_t;

  void removeEnvironment(std::uint32_t id);
  [[nodiscard]] auto getEnvironment(std::uint32_t id) const
      -> std::shared_ptr<Environment>;
  [[nodiscard]] auto getEnvironments() const
      -> std::vector<std::shared_ptr<Environment>>;

 private:
  graphics::GfxDevice& device_;
  std::shared_ptr<texture::TextureManager> textureManager_;
  std::shared_ptr<compute::AsyncCompute> asyncCompute_;

  vkit::Storage<Environment> storage_;

  vk::UniqueDescriptorPool computeDescriptorPool_;

  vk::UniqueSampler defaultSampler_;

  std::unique_ptr<compute::dsl::BrdfLutSetLayout> brdfLutSetLayout_;
  std::unique_ptr<compute::dsl::IblComputeSetLayout> iblComputeSetLayout_;

  std::unique_ptr<compute::pl::BrdfLutPipelineLayout> brdfLutPipelineLayout_;
  std::unique_ptr<compute::pl::DiffusePipelineLayout> diffusePipelineLayout_;
  std::unique_ptr<compute::pl::SpecularPipelineLayout> specularPipelineLayout_;

  vk::UniquePipeline brdfLutPipeline_;
  vk::UniquePipeline irradiancePipeline_;
  vk::UniquePipeline prefilterPipeline_;

  std::shared_ptr<texture::Texture> globalBrdfLut_;

  void initComputePipelines();
};

};  // namespace vkit::env

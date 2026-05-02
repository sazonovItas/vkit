#pragma once

#include <imgui.h>

#include <optional>
#include <vector>
#include <vk_mem_alloc.hpp>

#include "vkit/graphics/descriptor_set_layout.hpp"
#include "vkit/graphics/image.hpp"
#include "vkit/graphics/mapped_buffer.hpp"
#include "vkit/graphics/pipeline/graphics.hpp"
#include "vkit/graphics/pipeline_layout.hpp"
#include "vkit/graphics/util.hpp"

namespace vkit::imgui {

class ImguiRenderer {
 public:
  ImguiRenderer(vk::Device device, vma::Allocator allocator,
                vk::Format colorFormat, vk::SampleCountFlagBits samples,
                std::uint32_t maxFramesInFlight = 3);
  ~ImguiRenderer();

  void uploadFont(const graphics::util::RecordAndSubmitInfo& submitInfo);

  [[nodiscard]] auto registerTexture(vk::ImageView imageView,
                                     vk::Sampler sampler = nullptr)
      -> ImTextureID;
  [[nodiscard]] auto updateOrRegisterTexture(ImTextureID existingId,
                                             vk::ImageView imageView,
                                             vk::Sampler sampler = nullptr)
      -> ImTextureID;
  void unregisterTexture(ImTextureID textureId);

  void render(std::uint32_t frameIndex, vk::CommandBuffer cb,
              ImDrawData* drawData);

  void processGC();

 private:
  std::uint32_t maxFramesInFlight_;

  vk::Device device_;
  vma::Allocator allocator_;

  std::optional<graphics::DescriptorSetLayout> descriptorSetLayout_;
  vk::UniqueDescriptorPool descriptorPool_;

  std::optional<graphics::PipelineLayout> pipelineLayout_;
  std::optional<graphics::pipeline::GraphicsPipeline> pipeline_;

  struct FrameData {
    std::optional<graphics::MappedBuffer> vertexBuffer;
    std::optional<graphics::MappedBuffer> indexBuffer;
  };

  std::vector<FrameData> frames_;

  std::optional<graphics::AllocatedImage> fontImage_;
  vk::UniqueImageView fontView_;

  vk::UniqueSampler linearSampler_;
  vk::UniqueSampler nearestSampler_;
  vk::UniqueSampler fontSampler_;

  struct GCTask {
    vk::DescriptorSet set;
    int framesRemaining;
  };

  std::vector<GCTask> gcQueue_;
};

}  // namespace vkit::imgui

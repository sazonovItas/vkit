#pragma once

#include <imgui.h>

#include <optional>
#include <vector>
#include <vk_mem_alloc.hpp>

#include "vkit/graphics/descriptor_set_layout/dsl.hpp"
#include "vkit/graphics/image.hpp"
#include "vkit/graphics/mapped_buffer.hpp"
#include "vkit/graphics/pipeline/graphics.hpp"
#include "vkit/graphics/pipeline_layout/pl.hpp"

namespace vkit::imgui {

class ImguiRenderer {
 public:
  ImguiRenderer(vk::Device device, vma::Allocator allocator,
                vk::Format colorFormat, vk::SampleCountFlagBits samples,
                std::uint32_t framesInFlight = 2);
  ~ImguiRenderer();

  void uploadFont(vk::CommandBuffer cb);

  [[nodiscard]] auto registerTexture(vk::ImageView imageView,
                                     vk::Sampler sampler) -> ImTextureID;
  void unregisterTexture(ImTextureID textureId);

  void renderDrawData(vk::CommandBuffer cb, ImDrawData* drawData,
                      std::size_t frameIndex);

 private:
  vk::Device device_;
  vma::Allocator allocator_;

  std::optional<graphics::dsl::DescriptorSetLayout> descriptorSetLayout_;
  vk::UniqueDescriptorPool descriptorPool_;

  std::optional<graphics::pl::PipelineLayout> pipelineLayout_;
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
};

}  // namespace vkit::imgui

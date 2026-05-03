#include "vkit/imgui/imgui_renderer.hpp"

#include <cstring>

#include "vkit/asset/shaders.hpp"
#include "vkit/asset/util.hpp"
#include "vkit/graphics/mapped_buffer.hpp"
#include "vkit/graphics/shader_module.hpp"
#include "vulkan/vulkan.hpp"

namespace vkit::imgui {

ImguiRenderer::ImguiRenderer(vk::Device device, vma::Allocator allocator,
                             vk::Format colorFormat,
                             vk::SampleCountFlagBits samples,
                             const std::uint32_t maxFramesInFlight)
    : maxFramesInFlight_{maxFramesInFlight},
      device_{device},
      allocator_{allocator},
      frames_{maxFramesInFlight} {
  const auto binding = vk::DescriptorSetLayoutBinding{
      0,
      vk::DescriptorType::eCombinedImageSampler,
      1,
      vk::ShaderStageFlagBits::eFragment,
  };
  descriptorSetLayout_.emplace(device_,
                               vk::DescriptorSetLayoutCreateInfo{{}, binding});

  const auto pool_size = vk::DescriptorPoolSize{
      vk::DescriptorType::eCombinedImageSampler,
      1024,
  };
  const auto pool_ci = vk::DescriptorPoolCreateInfo{
      vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      1024,
      pool_size,
  };
  descriptorPool_ = device_.createDescriptorPoolUnique(pool_ci);

  const auto pc_range = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eVertex,
      0,
      sizeof(glm::vec2) * 2,
  };

  const auto pipeline_layout_ci = vk::PipelineLayoutCreateInfo{
      {},
      descriptorSetLayout_->get(),
      pc_range,
  };
  pipelineLayout_.emplace(device_, pipeline_layout_ci);

  const auto linear_sampler_ci =
      vk::SamplerCreateInfo{}
          .setMagFilter(vk::Filter::eLinear)
          .setMinFilter(vk::Filter::eLinear)
          .setMipmapMode(vk::SamplerMipmapMode::eLinear)
          .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
          .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
          .setAddressModeW(vk::SamplerAddressMode::eClampToEdge);
  linearSampler_ = device_.createSamplerUnique(linear_sampler_ci);

  const auto nearest_sampler_ci =
      vk::SamplerCreateInfo{}
          .setMagFilter(vk::Filter::eNearest)
          .setMinFilter(vk::Filter::eNearest)
          .setMipmapMode(vk::SamplerMipmapMode::eNearest)
          .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
          .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
          .setAddressModeW(vk::SamplerAddressMode::eClampToEdge);
  nearestSampler_ = device_.createSamplerUnique(nearest_sampler_ci);

  auto vert_module = graphics::SpirVShaderModule{
      device_,
      asset::assetPath(asset::kImguiVertShaderPath),
  };
  auto frag_module = graphics::SpirVShaderModule{
      device_,
      asset::assetPath(asset::kImguiFragShaderPath),
  };

  auto builder =
      graphics::pipeline::GraphicsPipelineBuilder{pipelineLayout_->get()};

  const auto binding_desc = vk::VertexInputBindingDescription{
      0, sizeof(ImDrawVert), vk::VertexInputRate::eVertex};

  const auto attr_desc = std::array{
      vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32Sfloat,
                                          offsetof(ImDrawVert, pos)},
      vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32Sfloat,
                                          offsetof(ImDrawVert, uv)},
      vk::VertexInputAttributeDescription{2, 0, vk::Format::eR8G8B8A8Unorm,
                                          offsetof(ImDrawVert, col)}};

  builder
      .addShaderStage(
          vert_module.stageCreateInfo(vk::ShaderStageFlagBits::eVertex))
      .addShaderStage(
          frag_module.stageCreateInfo(vk::ShaderStageFlagBits::eFragment))
      .setVertexInput(binding_desc, attr_desc)
      .setRenderingFormats({colorFormat}, vk::Format::eUndefined)
      .setMultisampling(samples)
      .setDepthState(vk::False, vk::False)
      .setCullMode(vk::CullModeFlagBits::eNone)
      .setColorBlendAttachment(0, graphics::pipeline::blend::kAlpha);

  pipeline_.emplace(builder.build(device_));
}

ImguiRenderer::~ImguiRenderer() { device_.waitIdle(); }

auto ImguiRenderer::registerTexture(vk::ImageView imageView,
                                    vk::Sampler sampler) -> ImTextureID {
  if (sampler == nullptr) {
    sampler = *this->linearSampler_;
  }

  const auto alloc_info = vk::DescriptorSetAllocateInfo{
      *descriptorPool_,
      1,
      &descriptorSetLayout_->get(),
  };
  const auto descriptor_set = device_.allocateDescriptorSets(alloc_info)[0];

  const auto image_info = vk::DescriptorImageInfo{
      sampler,
      imageView,
      vk::ImageLayout::eShaderReadOnlyOptimal,
  };
  const auto write = vk::WriteDescriptorSet{
      descriptor_set, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler,
      &image_info,
  };
  device_.updateDescriptorSets(write, nullptr);

  const auto* raw_set = static_cast<VkDescriptorSet>(descriptor_set);
  return reinterpret_cast<ImTextureID>(raw_set);
}

auto ImguiRenderer::updateOrRegisterTexture(ImTextureID existingId,
                                            vk::ImageView imageView,
                                            vk::Sampler sampler)
    -> ImTextureID {
  if (sampler == nullptr) {
    sampler = *this->linearSampler_;
  }

  if (existingId == 0) {
    return registerTexture(imageView, sampler);
  }

  auto* const raw_set = reinterpret_cast<VkDescriptorSet>(existingId);
  const auto descriptor_set = vk::DescriptorSet{raw_set};

  const auto image_info = vk::DescriptorImageInfo{
      sampler,
      imageView,
      vk::ImageLayout::eShaderReadOnlyOptimal,
  };

  const auto write = vk::WriteDescriptorSet{
      descriptor_set, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler,
      &image_info,
  };

  device_.updateDescriptorSets(write, nullptr);

  return existingId;
}

void ImguiRenderer::unregisterTexture(ImTextureID textureId) {
  if (textureId) {
    auto* const raw_set = reinterpret_cast<VkDescriptorSet>(textureId);
    auto set = vk::DescriptorSet{raw_set};
    std::ignore = device_.freeDescriptorSets(descriptorPool_.get(), 1, &set);
  }
}

void ImguiRenderer::uploadFont(
    const graphics::util::RecordAndSubmitInfo& submitInfo) {
  auto& io = ImGui::GetIO();
  auto* pixels = static_cast<unsigned char*>(nullptr);
  auto width = 0;
  auto height = 0;

  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  const auto upload_size =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;

  const auto staging_ci = vk::BufferCreateInfo{
      {},
      upload_size,
      vk::BufferUsageFlagBits::eTransferSrc,
  };

  auto staging_buffer = graphics::MappedBuffer{
      allocator_, staging_ci, graphics::allocation::kHostWrite};
  std::memcpy(staging_buffer.data, pixels, upload_size);

  auto image_ci = vk::ImageCreateInfo{};
  image_ci.setImageType(vk::ImageType::e2D)
      .setFormat(vk::Format::eR8G8B8A8Unorm)
      .setExtent(
          {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1})
      .setMipLevels(1)
      .setArrayLayers(1)
      .setSamples(vk::SampleCountFlagBits::e1)
      .setUsage(vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst);

  fontImage_.emplace(allocator_, image_ci, graphics::allocation::kDeviceLocal);
  fontView_ = device_.createImageViewUnique(fontImage_->getViewCreateInfo());

  graphics::util::recordAndSubmit(submitInfo, [&](vk::CommandBuffer cb) {
    auto transfer_barrier = vk::ImageMemoryBarrier2{};
    transfer_barrier.setImage(static_cast<vk::Image>(*fontImage_))
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
        .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite)
        .setSubresourceRange(fontImage_->subresourceRange());
    cb.pipelineBarrier2(
        vk::DependencyInfo{}.setImageMemoryBarriers(transfer_barrier));

    const auto copy_region = vk::BufferImageCopy{
        0,         0,
        0,         {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        {0, 0, 0}, image_ci.extent};
    cb.copyBufferToImage(static_cast<vk::Buffer>(staging_buffer),
                         static_cast<vk::Image>(*fontImage_),
                         vk::ImageLayout::eTransferDstOptimal, copy_region);

    auto shader_barrier = transfer_barrier;
    shader_barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderRead);
    cb.pipelineBarrier2(
        vk::DependencyInfo{}.setImageMemoryBarriers(shader_barrier));
  });

  io.Fonts->SetTexID(registerTexture(*fontView_, *linearSampler_));
}

void ImguiRenderer::render(std::uint32_t frameIndex, vk::CommandBuffer cb,
                           ImDrawData* drawData) {
  assert(frameIndex < maxFramesInFlight_ &&
         "ImguiRenderer: frameIndex out of bounds!");

  if (!drawData || drawData->TotalVtxCount == 0) {
    return;
  }

  auto& frame = frames_[frameIndex];
  const auto req_vtx_size =
      static_cast<std::size_t>(drawData->TotalVtxCount) * sizeof(ImDrawVert);
  const auto req_idx_size =
      static_cast<std::size_t>(drawData->TotalIdxCount) * sizeof(ImDrawIdx);

  if (!frame.vertexBuffer || frame.vertexBuffer->size < req_vtx_size) {
    const auto ci = vk::BufferCreateInfo{
        {}, req_vtx_size + 5000, vk::BufferUsageFlagBits::eVertexBuffer};
    frame.vertexBuffer.emplace(allocator_, ci,
                               graphics::allocation::kHostWrite);
  }
  if (!frame.indexBuffer || frame.indexBuffer->size < req_idx_size) {
    const auto ci = vk::BufferCreateInfo{
        {}, req_idx_size + 5000, vk::BufferUsageFlagBits::eIndexBuffer};
    frame.indexBuffer.emplace(allocator_, ci, graphics::allocation::kHostWrite);
  }

  auto* vtx_dst = static_cast<ImDrawVert*>(frame.vertexBuffer->data);
  auto* idx_dst = static_cast<ImDrawIdx*>(frame.indexBuffer->data);

  for (auto n = 0; n < drawData->CmdListsCount; n++) {
    const auto* cmd_list = drawData->CmdLists[n];
    std::memcpy(vtx_dst, cmd_list->VtxBuffer.Data,
                cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
    std::memcpy(idx_dst, cmd_list->IdxBuffer.Data,
                cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
    vtx_dst += cmd_list->VtxBuffer.Size;
    idx_dst += cmd_list->IdxBuffer.Size;
  }

  cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_->get());
  cb.bindVertexBuffers(0, {static_cast<vk::Buffer>(*frame.vertexBuffer)}, {0});
  cb.bindIndexBuffer(
      static_cast<vk::Buffer>(*frame.indexBuffer), 0,
      sizeof(ImDrawIdx) == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);

  const auto viewport = vk::Viewport{
      0.0F, 0.0F, drawData->DisplaySize.x, drawData->DisplaySize.y, 0.0F, 1.0F,
  };
  cb.setViewport(0, viewport);

  const auto scale =
      glm::vec2{2.0F / drawData->DisplaySize.x, 2.0F / drawData->DisplaySize.y};
  const auto translate = glm::vec2{-1.0F - (drawData->DisplayPos.x * scale.x),
                                   -1.0F - (drawData->DisplayPos.y * scale.y)};
  struct PC {
    glm::vec2 s, t;
  };
  const auto pc = PC{.s = scale, .t = translate};
  cb.pushConstants(pipelineLayout_->get(), vk::ShaderStageFlagBits::eVertex, 0,
                   sizeof(PC), &pc);

  const auto clip_off = drawData->DisplayPos;
  const auto clip_scale = drawData->FramebufferScale;

  auto global_vtx_offset = std::uint32_t{0};
  auto global_idx_offset = std::uint32_t{0};

  for (auto n = 0; n < drawData->CmdListsCount; n++) {
    const auto* cmd_list = drawData->CmdLists[n];

    for (auto cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
      const auto* pcmd = &cmd_list->CmdBuffer[cmd_i];

      if (pcmd->UserCallback) {
        pcmd->UserCallback(cmd_list, pcmd);
      } else {
        auto scissor = vk::Rect2D{};
        scissor.offset.x =
            std::max(static_cast<int32_t>((pcmd->ClipRect.x - clip_off.x) *
                                          clip_scale.x),
                     0);
        scissor.offset.y =
            std::max(static_cast<int32_t>((pcmd->ClipRect.y - clip_off.y) *
                                          clip_scale.y),
                     0);
        scissor.extent.width = static_cast<uint32_t>(
            (pcmd->ClipRect.z - pcmd->ClipRect.x) * clip_scale.x);
        scissor.extent.height = static_cast<uint32_t>(
            (pcmd->ClipRect.w - pcmd->ClipRect.y) * clip_scale.y);
        cb.setScissor(0, scissor);

        auto* const raw_set =
            reinterpret_cast<VkDescriptorSet>(pcmd->GetTexID());
        const auto texture_set = vk::DescriptorSet{raw_set};

        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                              pipelineLayout_->get(), 0, {texture_set}, {});

        cb.drawIndexed(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset,
                       pcmd->VtxOffset + global_vtx_offset, 0);
      }
    }

    global_vtx_offset += cmd_list->VtxBuffer.Size;
    global_idx_offset += cmd_list->IdxBuffer.Size;
  }
}

};  // namespace vkit::imgui

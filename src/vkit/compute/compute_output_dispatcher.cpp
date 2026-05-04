#include "vkit/compute/compute_output_dispatcher.hpp"

#include <algorithm>

#include "vkit/compute/command/image.hpp"
#include "vkit/graphics/command.hpp"
#include "vkit/graphics/shader_module.hpp"
#include "vkit/graphics/texture.hpp"

namespace vkit::compute {

namespace {

auto makeDsl(vk::Device device,
             std::span<const vk::DescriptorSetLayoutBinding> bindings)
    -> vk::UniqueDescriptorSetLayout {
  return device.createDescriptorSetLayoutUnique(
      vk::DescriptorSetLayoutCreateInfo{{}, bindings});
}

auto makeOutputTex(const graphics::GfxDevice& device, uint32_t w, uint32_t h,
                   vk::Format fmt, vk::Sampler sampler, bool mipmaps,
                   std::string_view name) -> std::shared_ptr<texture::Texture> {
  vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage |
                              vk::ImageUsageFlagBits::eSampled |
                              vk::ImageUsageFlagBits::eTransferSrc;
  if (mipmaps) usage |= vk::ImageUsageFlagBits::eTransferDst;

  graphics::TextureCreateInfo ci{
      .type = graphics::TextureType::k2D,
      .pixelFormat = fmt,
      .usage = usage,
      .useMipmaps = mipmaps,
      .width = static_cast<int>(w),
      .height = static_cast<int>(h),
  };
  auto gfx =
      std::make_shared<graphics::Texture>(device.get(), device.allocator, ci);
  gfx->setSampler(sampler);
  return std::make_shared<texture::Texture>(name, gfx);
}

void recordMipmapGeneration(vk::CommandBuffer cb, vk::Image img,
                            uint32_t levelCount, uint32_t w, uint32_t h) {
  auto barrier =
      [&](vk::ImageLayout from, vk::ImageLayout to, vk::AccessFlags2 srcAccess,
          vk::AccessFlags2 dstAccess, vk::PipelineStageFlags2 srcStage,
          vk::PipelineStageFlags2 dstStage, vk::ImageSubresourceRange range) {
        vk::ImageMemoryBarrier2 b{};
        b.setImage(img)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setOldLayout(from)
            .setNewLayout(to)
            .setSrcStageMask(srcStage)
            .setSrcAccessMask(srcAccess)
            .setDstStageMask(dstStage)
            .setDstAccessMask(dstAccess)
            .setSubresourceRange(range);
        cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(b));
      };

  constexpr vk::ImageSubresourceRange kMip0{vk::ImageAspectFlagBits::eColor, 0,
                                            1, 0, 1};

  if (levelCount <= 1) {
    barrier(vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::PipelineStageFlagBits2::eAllCommands, kMip0);
    return;
  }

  barrier(vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal,
          vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eTransferRead,
          vk::PipelineStageFlagBits2::eComputeShader,
          vk::PipelineStageFlagBits2::eTransfer, kMip0);

  auto mip_w = static_cast<int32_t>(w);
  auto mip_h = static_cast<int32_t>(h);

  for (uint32_t i = 1; i < levelCount; ++i) {
    int32_t next_w = std::max(1, mip_w / 2);
    int32_t next_h = std::max(1, mip_h / 2);
    vk::ImageSubresourceRange dst_range{vk::ImageAspectFlagBits::eColor, i, 1,
                                        0, 1};

    barrier(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
            vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eTransferWrite,
            vk::PipelineStageFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eTransfer, dst_range);

    vk::ImageBlit2 region{};
    region.setSrcSubresource({vk::ImageAspectFlagBits::eColor, i - 1, 0, 1})
        .setSrcOffsets({vk::Offset3D{0, 0, 0}, vk::Offset3D{mip_w, mip_h, 1}})
        .setDstSubresource({vk::ImageAspectFlagBits::eColor, i, 0, 1})
        .setDstOffsets(
            {vk::Offset3D{0, 0, 0}, vk::Offset3D{next_w, next_h, 1}});
    cb.blitImage2(vk::BlitImageInfo2{}
                      .setSrcImage(img)
                      .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
                      .setDstImage(img)
                      .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
                      .setRegions(region)
                      .setFilter(vk::Filter::eLinear));

    barrier(vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::AccessFlagBits2::eTransferWrite,
            vk::AccessFlagBits2::eTransferRead,
            vk::PipelineStageFlagBits2::eTransfer,
            vk::PipelineStageFlagBits2::eTransfer, dst_range);

    mip_w = next_w;
    mip_h = next_h;
  }

  vk::ImageSubresourceRange all_mips{vk::ImageAspectFlagBits::eColor, 0,
                                     levelCount, 0, 1};
  barrier(vk::ImageLayout::eTransferSrcOptimal,
          vk::ImageLayout::eShaderReadOnlyOptimal,
          vk::AccessFlagBits2::eTransferRead, vk::AccessFlagBits2::eNone,
          vk::PipelineStageFlagBits2::eTransfer,
          vk::PipelineStageFlagBits2::eAllCommands, all_mips);
}

};  // namespace

ComputeOutputDispatcher::ComputeOutputDispatcher(
    const graphics::GfxDevice& device, core::events::ComputeOutputBus& jobBus,
    core::events::ComputeOutputResultBus& resultBus,
    std::shared_ptr<AsyncCompute> asyncCompute)
    : device_{device},
      resultBus_{resultBus},
      asyncCompute_{std::move(asyncCompute)},
      sub_{jobBus.subscribe(
          [this](core::events::ComputeOutputJob& job) { onRequest(job); })} {
  constexpr auto kComp = vk::ShaderStageFlagBits::eCompute;
  constexpr auto kStore = vk::DescriptorType::eStorageImage;
  constexpr auto kSamp = vk::DescriptorType::eCombinedImageSampler;

  {
    std::array<vk::DescriptorSetLayoutBinding, 2> b{
        vk::DescriptorSetLayoutBinding{0, kStore, 1, kComp},
        vk::DescriptorSetLayoutBinding{1, kStore, 1, kComp},
    };
    generatorDsl_ = makeDsl(device.get(), b);
  }
  {
    std::array<vk::DescriptorSetLayoutBinding, 3> b{
        vk::DescriptorSetLayoutBinding{0, kSamp, 1, kComp},
        vk::DescriptorSetLayoutBinding{1, kStore, 1, kComp},
        vk::DescriptorSetLayoutBinding{2, kStore, 1, kComp},
    };
    singleInputDsl_ = makeDsl(device.get(), b);
  }
  {
    std::array<vk::DescriptorSetLayoutBinding, 5> b{
        vk::DescriptorSetLayoutBinding{0, kSamp, 1, kComp},
        vk::DescriptorSetLayoutBinding{1, kSamp, 1, kComp},
        vk::DescriptorSetLayoutBinding{2, kSamp, 1, kComp},
        vk::DescriptorSetLayoutBinding{3, kStore, 1, kComp},
        vk::DescriptorSetLayoutBinding{4, kStore, 1, kComp},
    };
    dualInputDsl_ = makeDsl(device.get(), b);
  }

  auto make_sampler = [&](vk::SamplerAddressMode mode) {
    return device.get().createSamplerUnique(vk::SamplerCreateInfo{
        {},
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear,
        mode,
        mode,
        mode,
    });
  };
  samplerRepeat_ = make_sampler(vk::SamplerAddressMode::eRepeat);
  samplerClamp_ = make_sampler(vk::SamplerAddressMode::eClampToEdge);
}

void ComputeOutputDispatcher::registerPipeline(
    const std::string& name, vk::Device device,
    const std::filesystem::path& spv, core::events::ComputeBindingLayout layout,
    uint32_t pushConstantsSize) {
  vk::DescriptorSetLayout dsl = getDsl(layout);

  vk::PushConstantRange pc_range{vk::ShaderStageFlagBits::eCompute, 0,
                                 pushConstantsSize};
  auto pl = device.createPipelineLayoutUnique(
      vk::PipelineLayoutCreateInfo{{}, dsl, pc_range});

  graphics::SpirVShaderModule shader(device, spv);
  auto stage = shader.stageCreateInfo(vk::ShaderStageFlagBits::eCompute);
  auto [result, pipe] = device.createComputePipelineUnique(
      nullptr, vk::ComputePipelineCreateInfo{{}, stage, *pl});

  pipelines_.emplace(name, PipelineEntry{
                               .pipeline = std::move(pipe),
                               .pipelineLayout = std::move(pl),
                               .bindingLayout = layout,
                           });
}

auto ComputeOutputDispatcher::getPipeline(const std::string& name) const
    -> core::events::ComputeHandles {
  auto it = pipelines_.find(name);
  if (it == pipelines_.end()) return {};
  const auto& e = it->second;
  return {
      .pipeline = *e.pipeline,
      .pipelineLayout = *e.pipelineLayout,
      .dsl = getDsl(e.bindingLayout),
      .bindingLayout = e.bindingLayout,
  };
}

auto ComputeOutputDispatcher::getDsl(core::events::ComputeBindingLayout l) const
    -> vk::DescriptorSetLayout {
  switch (l) {
    case core::events::ComputeBindingLayout::kGenerator:
      return *generatorDsl_;
    case core::events::ComputeBindingLayout::kSingleInput:
      return *singleInputDsl_;
    case core::events::ComputeBindingLayout::kDualInput:
      return *dualInputDsl_;
  }
  return *generatorDsl_;
}

void ComputeOutputDispatcher::onRequest(core::events::ComputeOutputJob& job) {
  using BL = core::events::ComputeBindingLayout;
  const auto& h = job.handles;

  auto out_sampler =
      (h.bindingLayout == BL::kGenerator) ? *samplerRepeat_ : *samplerClamp_;

  auto tex_f32 = makeOutputTex(device_, job.width, job.height,
                               vk::Format::eR32G32B32A32Sfloat, out_sampler,
                               false, "compute_f32");
  auto tex_unorm =
      makeOutputTex(device_, job.width, job.height, vk::Format::eR8G8B8A8Unorm,
                    out_sampler, true, "compute_unorm");

  auto& gfx_f32 = *tex_f32->getGraphicsTexture();
  auto& gfx_unorm = *tex_unorm->getGraphicsTexture();

  vk::Image img_f32 = gfx_f32.getImage().image;
  vk::Image img_unorm = gfx_unorm.getImage().image;

  auto view_f32 = device_.get().createImageViewUnique(
      gfx_f32.getImage().getViewCreateInfo());

  vk::ImageSubresourceRange mip0{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
  auto view_unorm_storage = device_.get().createImageViewUnique(
      gfx_unorm.getImage().getViewCreateInfo(mip0));

  std::vector<vk::DescriptorPoolSize> pool_sizes;
  uint32_t sampler_count = 0;
  if (h.bindingLayout == BL::kSingleInput) sampler_count = 1;
  if (h.bindingLayout == BL::kDualInput) sampler_count = 3;
  if (sampler_count > 0)
    pool_sizes.emplace_back(vk::DescriptorType::eCombinedImageSampler,
                            sampler_count);
  pool_sizes.emplace_back(vk::DescriptorType::eStorageImage, 2);

  auto desc_pool =
      device_.get().createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
          vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, pool_sizes});

  auto set_arr = std::array<vk::DescriptorSetLayout, 1>{h.dsl};
  auto ds = device_.get()
                .allocateDescriptorSets(vk::DescriptorSetAllocateInfo{}
                                            .setDescriptorPool(*desc_pool)
                                            .setSetLayouts(set_arr))
                .front();

  std::vector<vk::DescriptorImageInfo> img_infos;
  std::vector<vk::WriteDescriptorSet> writes;
  img_infos.reserve(8);
  writes.reserve(8);

  auto push_sampler = [&](uint32_t binding, vk::ImageView view,
                          vk::Sampler sampler) {
    img_infos.emplace_back(sampler, view,
                           vk::ImageLayout::eShaderReadOnlyOptimal);
    writes.emplace_back(ds, binding, 0, 1,
                        vk::DescriptorType::eCombinedImageSampler,
                        &img_infos.back());
  };
  auto push_storage = [&](uint32_t binding, vk::ImageView view) {
    img_infos.emplace_back(nullptr, view, vk::ImageLayout::eGeneral);
    writes.emplace_back(ds, binding, 0, 1, vk::DescriptorType::eStorageImage,
                        &img_infos.back());
  };

  auto input_sampler =
      [&](std::shared_ptr<texture::Texture>& tex) -> vk::Sampler {
    if (!tex) return *samplerClamp_;
    auto s = tex->getGraphicsTexture()->getSampler();
    return s ? s : *samplerClamp_;
  };
  auto input_view =
      [&](std::shared_ptr<texture::Texture>& tex) -> vk::ImageView {
    return tex->getGraphicsTexture()->getImageView();
  };

  switch (h.bindingLayout) {
    case BL::kGenerator:
      push_storage(0, *view_f32);
      push_storage(1, *view_unorm_storage);
      break;
    case BL::kSingleInput:
      push_sampler(0, input_view(job.inputA), input_sampler(job.inputA));
      push_storage(1, *view_f32);
      push_storage(2, *view_unorm_storage);
      break;
    case BL::kDualInput: {
      auto& fac = job.inputC ? job.inputC : job.inputA;
      push_sampler(0, input_view(job.inputA), input_sampler(job.inputA));
      push_sampler(1, input_view(job.inputB), input_sampler(job.inputB));
      push_sampler(2, input_view(fac), input_sampler(fac));
      push_storage(3, *view_f32);
      push_storage(4, *view_unorm_storage);
      break;
    }
  }
  device_.get().updateDescriptorSets(writes, {});

  auto task = std::make_shared<ComputeTask>();

  task->add<cmd::ComputeImageBarrierCommand>(
      img_f32, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
      vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eShaderWrite,
      vk::PipelineStageFlagBits2::eTopOfPipe,
      vk::PipelineStageFlagBits2::eComputeShader);
  task->add<cmd::ComputeImageBarrierCommand>(
      img_unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
      vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eShaderWrite,
      vk::PipelineStageFlagBits2::eTopOfPipe,
      vk::PipelineStageFlagBits2::eComputeShader, 0, 1);

  task->add<graphics::LambdaCommand>([pipeline = h.pipeline,
                                      layout = h.pipelineLayout, ds,
                                      push = job.pushData, w = job.width,
                                      h = job.height](vk::CommandBuffer cb) {
    cb.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);
    cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout, 0, ds, {});
    if (!push.empty())
      cb.pushConstants(layout, vk::ShaderStageFlagBits::eCompute, 0,
                       static_cast<uint32_t>(push.size()), push.data());
    cb.dispatch((w + 15) / 16, (h + 15) / 16, 1);
  });

  task->add<cmd::ComputeImageBarrierCommand>(
      img_f32, vk::ImageLayout::eGeneral,
      vk::ImageLayout::eShaderReadOnlyOptimal,
      vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eNone,
      vk::PipelineStageFlagBits2::eComputeShader,
      vk::PipelineStageFlagBits2::eAllCommands);

  {
    auto levels = static_cast<uint32_t>(gfx_unorm.getLevelCount());
    task->add<graphics::LambdaCommand>([img_unorm, levels, w = job.width,
                                        h = job.height](vk::CommandBuffer cb) {
      recordMipmapGeneration(cb, img_unorm, levels, w, h);
    });
  }

  auto fence = device_.get().createFenceUnique({});
  auto result = asyncCompute_->submit(*task, *fence);

  inFlightJobs_.push_back(InFlightComputeOutputJob{
      .requestId = job.requestId,
      .outputF32 = tex_f32,
      .outputUnorm = tex_unorm,
      .descriptorPool = std::move(desc_pool),
      .viewF32 = std::move(view_f32),
      .viewUnormStorage = std::move(view_unorm_storage),
      .fence = std::move(fence),
      .computeResult = std::move(result),
      .task = task,
  });
}

void ComputeOutputDispatcher::update() {
  for (auto it = inFlightJobs_.begin(); it != inFlightJobs_.end();) {
    if (device_.get().getFenceStatus(*it->fence) == vk::Result::eSuccess) {
      resultBus_.sendMessage(core::events::ComputeOutputResult{
          .requestId = it->requestId,
          .imageF32 = it->outputF32,
          .imageUnorm = it->outputUnorm,
      });
      it = inFlightJobs_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace vkit::compute

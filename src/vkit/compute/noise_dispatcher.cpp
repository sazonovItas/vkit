#include "vkit/compute/noise_dispatcher.hpp"

#include "vkit/asset/shaders.hpp"
#include "vkit/asset/util.hpp"
#include "vkit/graphics/command.hpp"
#include "vkit/graphics/texture.hpp"
#include "vulkan/vulkan.hpp"

namespace vkit::compute {

namespace {

auto makeNoiseTexture(const graphics::GfxDevice& device, uint32_t w, uint32_t h,
                      vk::Format fmt, vk::Sampler sampler,
                      std::string_view name)
    -> std::shared_ptr<texture::Texture> {
  graphics::TextureCreateInfo ci{
      .type = graphics::TextureType::k2D,
      .pixelFormat = fmt,
      .usage = vk::ImageUsageFlagBits::eStorage |
               vk::ImageUsageFlagBits::eSampled |
               vk::ImageUsageFlagBits::eTransferSrc,
      .useMipmaps = false,
      .width = static_cast<int>(w),
      .height = static_cast<int>(h),
  };
  auto gfx =
      std::make_shared<graphics::Texture>(device.get(), device.allocator, ci);
  gfx->setSampler(sampler);
  return std::make_shared<texture::Texture>(name, gfx);
}

void recordLayoutTransition(vk::CommandBuffer cb, vk::Image img,
                            vk::ImageLayout from, vk::ImageLayout to,
                            vk::PipelineStageFlags srcStage,
                            vk::PipelineStageFlags dstStage,
                            vk::AccessFlags srcAccess,
                            vk::AccessFlags dstAccess) {
  vk::ImageMemoryBarrier barrier{
      srcAccess,
      dstAccess,
      from,
      to,
      vk::QueueFamilyIgnored,
      vk::QueueFamilyIgnored,
      img,
      vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
  };
  cb.pipelineBarrier(srcStage, dstStage, {}, {}, {}, barrier);
}

};  // namespace

NoiseDispatcher::NoiseDispatcher(
    const graphics::GfxDevice& device,
    std::shared_ptr<texture::TextureManager> storage,
    core::events::NoiseJobBus& jobBus, core::events::NoiseResultBus& resultBus,
    std::shared_ptr<AsyncCompute> asyncCompute)
    : device_{device},
      storage_{std::move(storage)},
      resultBus_{resultBus},
      asyncCompute_{std::move(asyncCompute)},
      pipeline_{device.get(),
                asset::assetPath(asset::kProceduralNoiceShaderPath)},
      sub_{jobBus.subscribe(
          [this](core::events::NoiseJobRequest& req) { onRequest(req); })} {
  sampler_ = device.get().createSamplerUnique(vk::SamplerCreateInfo{
      {},
      vk::Filter::eLinear,
      vk::Filter::eLinear,
      vk::SamplerMipmapMode::eLinear,
      vk::SamplerAddressMode::eRepeat,
      vk::SamplerAddressMode::eRepeat,
      vk::SamplerAddressMode::eRepeat,
  });
}

void NoiseDispatcher::onRequest(core::events::NoiseJobRequest& req) {
  const auto& p = req.params;

  auto tex_f32 =
      makeNoiseTexture(device_, p.width, p.height,
                       vk::Format::eR32G32B32A32Sfloat, *sampler_, "noise_f32");
  auto tex_unorm =
      makeNoiseTexture(device_, p.width, p.height, vk::Format::eR8G8B8A8Unorm,
                       *sampler_, "noise_color");

  auto& gfx_f32 = *tex_f32->getGraphicsTexture();
  auto& gfx_unorm = *tex_unorm->getGraphicsTexture();

  vk::Image img_f32 = gfx_f32.getImage().image;
  vk::Image img_unorm = gfx_unorm.getImage().image;

  auto view_f32 = device_.get().createImageViewUnique(
      gfx_f32.getImage().getViewCreateInfo());
  auto view_unorm = device_.get().createImageViewUnique(
      gfx_unorm.getImage().getViewCreateInfo());

  std::array<vk::DescriptorPoolSize, 1> pool_sizes{
      vk::DescriptorPoolSize{
          vk::DescriptorType::eStorageImage,
          2,
      },
  };
  auto desc_pool =
      device_.get().createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
          vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
          2,
          pool_sizes,
      });

  auto set_layouts = pipeline_.descriptorSetLayout();
  auto alloc_info = vk::DescriptorSetAllocateInfo{}
                        .setDescriptorPool(*desc_pool)
                        .setSetLayouts(set_layouts);
  auto ds = device_.get().allocateDescriptorSets(alloc_info).front();

  std::array<vk::DescriptorImageInfo, 2> image_infos{
      vk::DescriptorImageInfo{
          nullptr,
          *view_f32,
          vk::ImageLayout::eGeneral,
      },
      vk::DescriptorImageInfo{
          nullptr,
          *view_unorm,
          vk::ImageLayout::eGeneral,
      },
  };

  std::array<vk::WriteDescriptorSet, 2> writes{
      vk::WriteDescriptorSet{
          ds,
          0,
          0,
          vk::DescriptorType::eStorageImage,
          image_infos[0],
      },
      vk::WriteDescriptorSet{
          ds,
          1,
          0,
          vk::DescriptorType::eStorageImage,
          image_infos[1],
      },
  };
  device_.get().updateDescriptorSets(writes, {});

  auto task = std::make_shared<ComputeTask>();
  task->add<graphics::LambdaCommand>(
      [this, img_f32, img_unorm, ds, params = p](vk::CommandBuffer cb) {
        recordLayoutTransition(cb, img_f32, vk::ImageLayout::eUndefined,
                               vk::ImageLayout::eGeneral,
                               vk::PipelineStageFlagBits::eTopOfPipe,
                               vk::PipelineStageFlagBits::eComputeShader, {},
                               vk::AccessFlagBits::eShaderWrite);
        recordLayoutTransition(cb, img_unorm, vk::ImageLayout::eUndefined,
                               vk::ImageLayout::eGeneral,
                               vk::PipelineStageFlagBits::eTopOfPipe,
                               vk::PipelineStageFlagBits::eComputeShader, {},
                               vk::AccessFlagBits::eShaderWrite);

        pipeline_.recordDispatch(cb, ds, params);

        recordLayoutTransition(cb, img_f32, vk::ImageLayout::eGeneral,
                               vk::ImageLayout::eShaderReadOnlyOptimal,
                               vk::PipelineStageFlagBits::eComputeShader,
                               vk::PipelineStageFlagBits::eComputeShader,
                               vk::AccessFlagBits::eShaderWrite,
                               vk::AccessFlagBits::eShaderRead);
        recordLayoutTransition(cb, img_unorm, vk::ImageLayout::eGeneral,
                               vk::ImageLayout::eShaderReadOnlyOptimal,
                               vk::PipelineStageFlagBits::eComputeShader,
                               vk::PipelineStageFlagBits::eComputeShader,
                               vk::AccessFlagBits::eShaderWrite,
                               vk::AccessFlagBits::eShaderRead);
      });

  auto fence = device_.get().createFenceUnique({});
  auto result = asyncCompute_->submit(*task, *fence);

  inFlightJobs_.push_back(InFlightNoiseJob{
      .requestId = req.requestId,
      .outputF32 = tex_f32,
      .outputUnorm = tex_unorm,
      .descriptorPool = std::move(desc_pool),
      .viewF32 = std::move(view_f32),
      .viewUnorm = std::move(view_unorm),
      .fence = std::move(fence),
      .computeResult = std::move(result),
      .task = task,
  });
}

void NoiseDispatcher::update() {
  std::vector<core::events::NoiseJobResult> completed_results;

  for (auto it = inFlightJobs_.begin(); it != inFlightJobs_.end();) {
    if (device_.get().getFenceStatus(*it->fence) == vk::Result::eSuccess) {
      completed_results.push_back(core::events::NoiseJobResult{
          .requestId = it->requestId,
          .imageF32 = it->outputF32,
          .imageUnorm = it->outputUnorm,
      });
      it = inFlightJobs_.erase(it);
    } else {
      ++it;
    }
  }

  for (auto& res : completed_results) {
    resultBus_.sendMessage(res);
  }
}

};  // namespace vkit::compute

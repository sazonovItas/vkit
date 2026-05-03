#include "vkit/compute/noise_dispatcher.hpp"

#include "vkit/compute/command/image.hpp"
#include "vkit/compute/util.hpp"
#include "vkit/core/shaders/shaders.hpp"
#include "vkit/graphics/command.hpp"

namespace vkit::compute {

NoiseDispatcher::NoiseDispatcher(const graphics::GfxDevice& device,
                                 core::events::NoiseJobBus& jobBus,
                                 core::events::NoiseResultBus& resultBus,
                                 std::shared_ptr<AsyncCompute> asyncCompute)
    : device_{device},
      resultBus_{resultBus},
      asyncCompute_{std::move(asyncCompute)},
      sub_{jobBus.subscribe(
          [this](core::events::NoiseJobRequest& req) { onRequest(req); })} {
  setLayout_ = std::make_unique<dsl::NoiseSetLayout>(device.get());
  pipelineLayout_ = std::make_unique<pl::NoisePipelineLayout>(
      device.get(), std::forward_as_tuple(*setLayout_));

  pipeline_ = std::make_unique<NoisePipeline>(
      device.get(), *pipelineLayout_,
      shaders::shaderPath(shaders::kProceduralNoiceShaderPath));

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

  auto tex_f32 = util::makeOutputTexture(device_, p.width, p.height,
                                         vk::Format::eR32G32B32A32Sfloat,
                                         *sampler_, "noise_f32");
  auto tex_unorm = util::makeOutputTexture(device_, p.width, p.height,
                                           vk::Format::eR8G8B8A8Unorm,
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

  auto set_layouts = std::array<vk::DescriptorSetLayout, 1>{setLayout_->get()};

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
          dsl::NoiseSetLayout::kOutImageF32Binding,
          0,
          vk::DescriptorType::eStorageImage,
          image_infos[0],
      },
      vk::WriteDescriptorSet{
          ds,
          dsl::NoiseSetLayout::kOutImageUnormBinding,
          0,
          vk::DescriptorType::eStorageImage,
          image_infos[1],
      },
  };
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
      vk::PipelineStageFlagBits2::eComputeShader);

  task->add<graphics::LambdaCommand>(
      [this, ds, params = p](vk::CommandBuffer cb) {
        pipeline_->recordDispatch(cb, pipelineLayout_->get(), ds, params);
      });

  task->add<cmd::ComputeImageBarrierCommand>(
      img_f32, vk::ImageLayout::eGeneral,
      vk::ImageLayout::eShaderReadOnlyOptimal,
      vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eShaderRead,
      vk::PipelineStageFlagBits2::eComputeShader,
      vk::PipelineStageFlagBits2::eComputeShader);

  task->add<cmd::ComputeImageBarrierCommand>(
      img_unorm, vk::ImageLayout::eGeneral,
      vk::ImageLayout::eShaderReadOnlyOptimal,
      vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eShaderRead,
      vk::PipelineStageFlagBits2::eComputeShader,
      vk::PipelineStageFlagBits2::eComputeShader);

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

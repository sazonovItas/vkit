#include "vkit/compute/mix_dispatcher.hpp"

#include "vkit/asset/shaders.hpp"
#include "vkit/asset/util.hpp"
#include "vkit/compute/command/image.hpp"
#include "vkit/compute/util.hpp"
#include "vkit/graphics/command.hpp"
#include "vkit/graphics/shader_module.hpp"

namespace vkit::compute {

MixDispatcher::MixDispatcher(const graphics::GfxDevice& device,
                             core::events::MixJobBus& jobBus,
                             core::events::MixResultBus& resultBus,
                             std::shared_ptr<AsyncCompute> asyncCompute)
    : device_{device},
      resultBus_{resultBus},
      asyncCompute_{std::move(asyncCompute)},
      sub_{jobBus.subscribe(
          [this](core::events::MixJobRequest& req) { onRequest(req); })} {
  setLayout_ = std::make_unique<dsl::MixOperatorSetLayout>(device.get());
  pipelineLayout_ = std::make_unique<pl::MixPipelineLayout>(
      device.get(), std::forward_as_tuple(*setLayout_));

  graphics::SpirVShaderModule shader(
      device.get(), asset::assetPath(asset::kOperatorsMixShaderPath));
  auto stage = shader.stageCreateInfo(vk::ShaderStageFlagBits::eCompute);
  auto [result, pipe] = device.get().createComputePipelineUnique(
      nullptr,
      vk::ComputePipelineCreateInfo{{}, stage, pipelineLayout_->get()});
  pipeline_ = std::move(pipe);

  sampler_ = device.get().createSamplerUnique(vk::SamplerCreateInfo{
      {},
      vk::Filter::eLinear,
      vk::Filter::eLinear,
      vk::SamplerMipmapMode::eLinear,
      vk::SamplerAddressMode::eClampToEdge,
      vk::SamplerAddressMode::eClampToEdge,
      vk::SamplerAddressMode::eClampToEdge,
  });
}

void MixDispatcher::onRequest(core::events::MixJobRequest& req) {
  if (!req.inputA || !req.inputB || !req.inputA->getGraphicsTexture() ||
      !req.inputB->getGraphicsTexture()) {
    resultBus_.sendMessage(
        {.requestId = req.requestId, .error = "Missing Input A or B texture"});
    return;
  }

  const auto& p = req.params;
  auto gfx_a = req.inputA->getGraphicsTexture();
  auto gfx_b = req.inputB->getGraphicsTexture();

  auto gfx_fac = (req.inputFac && req.inputFac->getGraphicsTexture())
                     ? req.inputFac->getGraphicsTexture()
                     : gfx_a;

  auto tex_f32 = util::makeOutputTexture(device_, p.width, p.height,
                                         vk::Format::eR32G32B32A32Sfloat,
                                         *sampler_, "mix_f32");
  auto tex_unorm = util::makeOutputTexture(device_, p.width, p.height,
                                           vk::Format::eR8G8B8A8Unorm,
                                           *sampler_, "mix_unorm");

  vk::Image img_f32 = tex_f32->getGraphicsTexture()->getImage().image;
  vk::Image img_unorm = tex_unorm->getGraphicsTexture()->getImage().image;

  auto view_f32 = device_.get().createImageViewUnique(
      tex_f32->getGraphicsTexture()->getImage().getViewCreateInfo());
  auto view_unorm = device_.get().createImageViewUnique(
      tex_unorm->getGraphicsTexture()->getImage().getViewCreateInfo());

  std::array<vk::DescriptorPoolSize, 2> pool_sizes{
      vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 3},
      vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, 2},
  };
  auto desc_pool =
      device_.get().createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
          vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, pool_sizes});

  auto set_layouts = std::array<vk::DescriptorSetLayout, 1>{setLayout_->get()};
  auto ds = device_.get()
                .allocateDescriptorSets(
                    vk::DescriptorSetAllocateInfo{*desc_pool, set_layouts})
                .front();

  vk::Sampler s_a = gfx_a->getSampler() ? gfx_a->getSampler() : *sampler_;
  vk::Sampler s_b = gfx_b->getSampler() ? gfx_b->getSampler() : *sampler_;
  vk::Sampler s_f = gfx_fac->getSampler() ? gfx_fac->getSampler() : *sampler_;

  vk::DescriptorImageInfo info_a{s_a, gfx_a->getImageView(),
                                 vk::ImageLayout::eShaderReadOnlyOptimal};
  vk::DescriptorImageInfo info_b{s_b, gfx_b->getImageView(),
                                 vk::ImageLayout::eShaderReadOnlyOptimal};
  vk::DescriptorImageInfo info_fac{s_f, gfx_fac->getImageView(),
                                   vk::ImageLayout::eShaderReadOnlyOptimal};

  vk::DescriptorImageInfo f32_info{nullptr, *view_f32,
                                   vk::ImageLayout::eGeneral};
  vk::DescriptorImageInfo unorm_info{nullptr, *view_unorm,
                                     vk::ImageLayout::eGeneral};

  std::array<vk::WriteDescriptorSet, 5> writes{
      vk::WriteDescriptorSet{
          ds, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &info_a},
      vk::WriteDescriptorSet{
          ds, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &info_b},
      vk::WriteDescriptorSet{
          ds, 2, 0, 1, vk::DescriptorType::eCombinedImageSampler, &info_fac},
      vk::WriteDescriptorSet{ds, 3, 0, 1, vk::DescriptorType::eStorageImage,
                             &f32_info},
      vk::WriteDescriptorSet{ds, 4, 0, 1, vk::DescriptorType::eStorageImage,
                             &unorm_info},
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

  task->add<graphics::LambdaCommand>([this, ds,
                                      params = p](vk::CommandBuffer cb) {
    cb.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline_);
    cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                          pipelineLayout_->get(), 0, ds, {});
    cb.pushConstants(pipelineLayout_->get(), vk::ShaderStageFlagBits::eCompute,
                     0, sizeof(params), &params);
    cb.dispatch((params.width + 15) / 16, (params.height + 15) / 16, 1);
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

  inFlightJobs_.push_back(InFlightMixJob{
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

void MixDispatcher::update() {
  std::vector<core::events::MixJobResult> completed;
  for (auto it = inFlightJobs_.begin(); it != inFlightJobs_.end();) {
    if (device_.get().getFenceStatus(*it->fence) == vk::Result::eSuccess) {
      completed.push_back({
          .requestId = it->requestId,
          .imageF32 = it->outputF32,
          .imageUnorm = it->outputUnorm,
          .error = "",
      });
      it = inFlightJobs_.erase(it);
    } else {
      ++it;
    }
  }
  for (auto& res : completed) resultBus_.sendMessage(res);
}

};  // namespace vkit::compute

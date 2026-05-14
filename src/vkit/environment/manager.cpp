#include "vkit/environment/manager.hpp"

#include <limits>
#include <tuple>

#include "vkit/compute/command/ibl_commands.hpp"
#include "vkit/compute/command/image.hpp"
#include "vkit/core/shaders/shaders.hpp"
#include "vkit/graphics/pipeline/compute.hpp"
#include "vkit/graphics/shader_module.hpp"
#include "vkit/platform/file_dialog.hpp"
#include "vkit/texture/loader.hpp"

namespace vkit::env {

EnvironmentManager::EnvironmentManager(
    graphics::GfxDevice& device,
    std::shared_ptr<texture::TextureManager> textureManager,
    std::shared_ptr<compute::AsyncCompute> asyncCompute)
    : device_{device},
      textureManager_{std::move(textureManager)},
      asyncCompute_{std::move(asyncCompute)} {
  std::vector<vk::DescriptorPoolSize> pool_sizes = {
      {vk::DescriptorType::eStorageImage, 16},
      {vk::DescriptorType::eCombinedImageSampler, 16}};

  computeDescriptorPool_ =
      device_.get().createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
          vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 16,
          pool_sizes});

  brdfLutSetLayout_ =
      std::make_unique<compute::dsl::BrdfLutSetLayout>(device_.get());
  iblComputeSetLayout_ =
      std::make_unique<compute::dsl::IblComputeSetLayout>(device_.get());

  brdfLutPipelineLayout_ = std::make_unique<compute::pl::BrdfLutPipelineLayout>(
      device_.get(), std::forward_as_tuple(*brdfLutSetLayout_));

  diffusePipelineLayout_ = std::make_unique<compute::pl::DiffusePipelineLayout>(
      device_.get(), std::forward_as_tuple(*iblComputeSetLayout_));

  specularPipelineLayout_ =
      std::make_unique<compute::pl::SpecularPipelineLayout>(
          device_.get(), std::forward_as_tuple(*iblComputeSetLayout_));

  auto sampler_info =
      vk::SamplerCreateInfo{}
          .setMinLod(0.0F)
          .setMaxLod(12.0F)
          .setMagFilter(vk::Filter::eLinear)
          .setMinFilter(vk::Filter::eLinear)
          .setMipmapMode(vk::SamplerMipmapMode::eLinear)
          .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
          .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
          .setAddressModeW(vk::SamplerAddressMode::eClampToEdge);

  defaultSampler_ = device_.get().createSamplerUnique(sampler_info);

  initComputePipelines();
}

void EnvironmentManager::initComputePipelines() {
  auto device = device_.get();
  using graphics::pipeline::ComputePipelineBuilder;

  {
    graphics::SpirVShaderModule mod(
        device, shaders::shaderPath(shaders::kIblBrdfLutShaderPath));

    auto builder = ComputePipelineBuilder{brdfLutPipelineLayout_->get()};
    builder.setShaderStage(
        mod.stageCreateInfo(vk::ShaderStageFlagBits::eCompute));
    brdfLutPipeline_ = builder.build(device);
  }

  {
    graphics::SpirVShaderModule mod(
        device, shaders::shaderPath(shaders::kIblDiffuseShaderPath));

    auto builder = ComputePipelineBuilder{diffusePipelineLayout_->get()};
    builder.setShaderStage(
        mod.stageCreateInfo(vk::ShaderStageFlagBits::eCompute));
    irradiancePipeline_ = builder.build(device);
  }

  {
    graphics::SpirVShaderModule mod(
        device, shaders::shaderPath(shaders::kIblSpecularShaderPath));

    auto builder = ComputePipelineBuilder{specularPipelineLayout_->get()};
    builder.setShaderStage(
        mod.stageCreateInfo(vk::ShaderStageFlagBits::eCompute));
    prefilterPipeline_ = builder.build(device);
  }
}

void EnvironmentManager::initializeGlobalBrdfLut() {
  constexpr uint32_t kDim = 512;

  graphics::TextureCreateInfo create_info{};
  create_info.type = graphics::TextureType::k2D;
  create_info.pixelFormat = vk::Format::eR32G32Sfloat;
  create_info.width = kDim;
  create_info.height = kDim;
  create_info.usage =
      vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

  auto graphics_tex = std::make_shared<graphics::Texture>(
      device_.get(), device_.allocator, create_info);

  globalBrdfLut_ =
      std::make_shared<texture::Texture>("GlobalBRDF", graphics_tex);
  textureManager_->add(globalBrdfLut_);

  const auto set_layout = brdfLutSetLayout_->get();
  auto sets = device_.get().allocateDescriptorSets({
      *computeDescriptorPool_,
      1,
      &set_layout,
  });

  vk::DescriptorImageInfo info{nullptr, graphics_tex->getImageView(),
                               vk::ImageLayout::eGeneral};
  device_.get().updateDescriptorSets(
      {
          vk::WriteDescriptorSet{
              sets[0],
              0,
              0,
              1,
              vk::DescriptorType::eStorageImage,
              &info,
          },
      },
      {});

  compute::ComputeTask task;

  task.add<compute::cmd::ComputeImageBarrierCommand>(
      static_cast<vk::Image>(graphics_tex->getImage()),
      vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
      vk::AccessFlags2{}, vk::AccessFlagBits2::eShaderWrite,
      vk::PipelineStageFlagBits2::eTopOfPipe,
      vk::PipelineStageFlagBits2::eComputeShader, 0, 1, 0, 1);

  task.add<compute::cmd::DispatchBrdfLutCommand>(
      *brdfLutPipeline_, brdfLutPipelineLayout_->get(), sets[0], kDim, kDim);

  task.add<compute::cmd::ComputeImageBarrierCommand>(
      static_cast<vk::Image>(graphics_tex->getImage().image),
      vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
      vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eNone,
      vk::PipelineStageFlagBits2::eComputeShader,
      vk::PipelineStageFlagBits2::eBottomOfPipe, 0, 1, 0, 1);

  auto fence = device_.get().createFenceUnique({});
  const auto compute_result = asyncCompute_->submit(task, *fence);
  (void)device_.get().waitForFences(*fence, vk::True,
                                    std::numeric_limits<std::uint64_t>::max());
}

auto EnvironmentManager::promptAndLoadEnvironment()
    -> std::optional<std::uint32_t> {
  std::array<platform::FileFilter, 1> filters = {
      platform::FileFilter{.name = "HDR Environment images", .spec = "hdr"}};

  auto filepath = platform::openFileDialog(filters, "");
  if (filepath && !filepath->empty() && std::filesystem::exists(*filepath)) {
    return loadEnvironment(*filepath);
  }

  return std::nullopt;
}

auto EnvironmentManager::loadEnvironment(const std::filesystem::path& filepath)
    -> std::optional<std::uint32_t> {
  if (!std::filesystem::exists(filepath)) return std::nullopt;

  texture::LoadOptions options{};
  options.useMipmaps = true;
  options.isSrgb = false;
  options.isHdr = true;
  options.type = graphics::TextureType::k2D;

  auto loaded_env = texture::loadFromFile(device_.get(), device_.allocator,
                                          filepath.string(), options);

  if (!loaded_env.texture) {
    return std::nullopt;
  }

  const auto submit_info = graphics::util::RecordAndSubmitInfo{
      .device = device_.get(),
      .queue = device_.queues.graphicsPresent,
      .commandPool = device_.getGraphicsPresentCommandPool(),
  };

  graphics::util::recordAndSubmit(submit_info, [&](vk::CommandBuffer cb) {
    loaded_env.texture->recordUpload(cb, loaded_env.stagingBuffer);
    loaded_env.texture->recordMipmapGeneration(cb);
  });

  std::string env_name = filepath.stem().string();

  return generateEnvironment(env_name, loaded_env.texture);
}

auto EnvironmentManager::generateEnvironment(
    std::string_view name,
    const std::shared_ptr<graphics::Texture>& sourceEnvMap) -> std::uint32_t {
  const uint32_t diff_dim = 256;
  const uint32_t spec_dim = 1024;
  const uint32_t mips = 5;

  graphics::TextureCreateInfo irr_info{};
  irr_info.type = graphics::TextureType::k2D;
  irr_info.pixelFormat = vk::Format::eR32G32B32A32Sfloat;
  irr_info.width = diff_dim * 2;
  irr_info.height = diff_dim;
  irr_info.arrayLayerCount = 1;
  irr_info.levelCount = 1;
  irr_info.usage =
      vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

  auto irr_gfx = std::make_shared<graphics::Texture>(
      device_.get(), device_.allocator, irr_info);

  graphics::TextureCreateInfo pref_info{};
  pref_info.type = graphics::TextureType::k2DArray;
  pref_info.pixelFormat = vk::Format::eR32G32B32A32Sfloat;
  pref_info.width = spec_dim * 2;
  pref_info.height = spec_dim;
  pref_info.arrayLayerCount = mips;
  pref_info.levelCount = 1;
  pref_info.useMipmaps = false;
  pref_info.usage =
      vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

  auto pref_gfx = std::make_shared<graphics::Texture>(
      device_.get(), device_.allocator, pref_info);

  auto irr_tex = std::make_shared<texture::Texture>(
      std::string(name) + "_irradiance", irr_gfx);
  auto pref_tex = std::make_shared<texture::Texture>(
      std::string(name) + "_prefilter", pref_gfx);

  textureManager_->add(irr_tex);
  textureManager_->add(pref_tex);

  auto layouts =
      std::array{iblComputeSetLayout_->get(), iblComputeSetLayout_->get()};
  auto sets = device_.get().allocateDescriptorSets(
      {*computeDescriptorPool_, 2, layouts.data()});

  auto update_set = [&](vk::DescriptorSet set, vk::ImageView target) {
    vk::DescriptorImageInfo src{*defaultSampler_, sourceEnvMap->getImageView(),
                                vk::ImageLayout::eShaderReadOnlyOptimal};
    vk::DescriptorImageInfo dst{nullptr, target, vk::ImageLayout::eGeneral};

    std::array writes = {
        vk::WriteDescriptorSet{set, 0, 0, 1,
                               vk::DescriptorType::eCombinedImageSampler, &src},
        vk::WriteDescriptorSet{set, 1, 0, 1, vk::DescriptorType::eStorageImage,
                               &dst}};
    device_.get().updateDescriptorSets(writes, {});
  };

  update_set(sets[0], irr_gfx->getImageView());
  update_set(sets[1], pref_gfx->getImageView());

  compute::ComputeTask task;

  task.add<compute::cmd::ComputeImageBarrierCommand>(
      static_cast<vk::Image>(irr_gfx->getImage()), vk::ImageLayout::eUndefined,
      vk::ImageLayout::eGeneral, vk::AccessFlags2{},
      vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eTopOfPipe,
      vk::PipelineStageFlagBits2::eComputeShader, 0, 1, 0, 1);

  task.add<compute::cmd::ComputeImageBarrierCommand>(
      static_cast<vk::Image>(pref_gfx->getImage()), vk::ImageLayout::eUndefined,
      vk::ImageLayout::eGeneral, vk::AccessFlags2{},
      vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eTopOfPipe,
      vk::PipelineStageFlagBits2::eComputeShader, 0, 1, 0, mips);

  task.add<compute::cmd::DispatchIrradianceCommand>(
      *irradiancePipeline_, diffusePipelineLayout_->get(), sets[0],
      diff_dim * 2, diff_dim);

  task.add<compute::cmd::DispatchPrefilterCommand>(
      *prefilterPipeline_, specularPipelineLayout_->get(), sets[1],
      spec_dim * 2, spec_dim, mips);

  task.add<compute::cmd::ComputeImageBarrierCommand>(
      static_cast<vk::Image>(irr_gfx->getImage()), vk::ImageLayout::eGeneral,
      vk::ImageLayout::eShaderReadOnlyOptimal,
      vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eNone,
      vk::PipelineStageFlagBits2::eComputeShader,
      vk::PipelineStageFlagBits2::eBottomOfPipe, 0, 1, 0, 1);

  task.add<compute::cmd::ComputeImageBarrierCommand>(
      static_cast<vk::Image>(pref_gfx->getImage()), vk::ImageLayout::eGeneral,
      vk::ImageLayout::eShaderReadOnlyOptimal,
      vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eNone,
      vk::PipelineStageFlagBits2::eComputeShader,
      vk::PipelineStageFlagBits2::eBottomOfPipe, 0, 1, 0, mips);

  auto fence = device_.get().createFenceUnique({});
  const auto compute_result = asyncCompute_->submit(task, *fence);
  (void)device_.get().waitForFences(*fence, vk::True,
                                    std::numeric_limits<std::uint64_t>::max());

  auto env =
      std::make_shared<Environment>(name, globalBrdfLut_, irr_tex, pref_tex);
  return storage_.add(env);
}

void EnvironmentManager::removeEnvironment(std::uint32_t id) {
  if (auto env = storage_.get(id)) {
    if (env->irradianceTexture &&
        env->irradianceTexture->getStorageId().has_value()) {
      textureManager_->remove(env->irradianceTexture->getStorageId().value());
    }

    if (env->prefilterTexture &&
        env->prefilterTexture->getStorageId().has_value()) {
      textureManager_->remove(env->prefilterTexture->getStorageId().value());
    }

    storage_.remove(id);
  }
}

auto EnvironmentManager::getEnvironment(std::uint32_t id) const
    -> std::shared_ptr<Environment> {
  return storage_.get(id);
}

auto EnvironmentManager::getEnvironments() const
    -> std::vector<std::shared_ptr<Environment>> {
  return storage_.getItems();
}

}  // namespace vkit::env

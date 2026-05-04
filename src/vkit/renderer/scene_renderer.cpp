#include "vkit/renderer/scene_renderer.hpp"

#include "vkit/core/shaders/shaders.hpp"
#include "vkit/graphics/pipeline/graphics.hpp"
#include "vkit/graphics/shader_module.hpp"

namespace vkit::renderer {

SceneRenderer::SceneRenderer(
    graphics::GfxDevice& device,
    const dsl::BindlessTextureSetLayout& bindlessLayout,
    std::uint32_t framesInFlight)
    : device_{device} {
  vk::Device dev = device_.get();

  sceneSetLayout_ = std::make_unique<dsl::SceneSetLayout>(dev);
  primitiveSetLayout_ = std::make_unique<dsl::PrimitiveSetLayout>(dev);
  materialSetLayout_ = std::make_unique<dsl::MaterialSetLayout>(dev);

  uPrimLayout_ = std::make_unique<pl::PrimitiveMaterialPipelineLayout>(
      dev, std::forward_as_tuple(*sceneSetLayout_, bindlessLayout,
                                 *materialSetLayout_, *primitiveSetLayout_));
  primitiveLayout = uPrimLayout_.get();

  uRaySphereLayout_ = std::make_unique<pl::RaySphereMaterialPipelineLayout>(
      dev, std::forward_as_tuple(*sceneSetLayout_, bindlessLayout,
                                 *materialSetLayout_));
  raySphereLayout = uRaySphereLayout_.get();

  uSkyboxLayout_ = std::make_unique<pl::SkyboxPipelineLayout>(
      dev, std::forward_as_tuple(*sceneSetLayout_, bindlessLayout));
  skyboxLayout = uSkyboxLayout_.get();

  auto prim_vert = graphics::SpirVShaderModule{
      dev,
      shaders::shaderPath(shaders::kPrimitiveVertShaderPath),
  };
  auto prim_frag = graphics::SpirVShaderModule{
      dev,
      shaders::shaderPath(shaders::kPrimitiveMaterialFragShaderPath),
  };

  auto opaque_builder =
      graphics::pipeline::GraphicsPipelineBuilder{uPrimLayout_->get()};
  opaque_builder
      .addShaderStage(
          prim_vert.stageCreateInfo(vk::ShaderStageFlagBits::eVertex))
      .addShaderStage(
          prim_frag.stageCreateInfo(vk::ShaderStageFlagBits::eFragment))
      .setVertexInput({}, {})
      .setRenderingFormats({vk::Format::eR8G8B8A8Unorm}, vk::Format::eD32Sfloat)
      .setDepthState(vk::True, vk::True)
      .setCullMode(vk::CullModeFlagBits::eBack)
      .setMultisampling(vk::SampleCountFlagBits::e8, vk::True, 0.5F);
  uOpaquePipeline_ = opaque_builder.build(dev);
  opaquePipeline = *uOpaquePipeline_;

  auto trans_builder =
      graphics::pipeline::GraphicsPipelineBuilder{uPrimLayout_->get()};
  trans_builder
      .addShaderStage(
          prim_vert.stageCreateInfo(vk::ShaderStageFlagBits::eVertex))
      .addShaderStage(
          prim_frag.stageCreateInfo(vk::ShaderStageFlagBits::eFragment))
      .setVertexInput({}, {})
      .setRenderingFormats({vk::Format::eR8G8B8A8Unorm}, vk::Format::eD32Sfloat)
      .setDepthState(vk::True, vk::False)
      .setCullMode(vk::CullModeFlagBits::eNone)
      .setColorBlendAttachment(0, graphics::pipeline::blend::kAlpha)
      .setMultisampling(vk::SampleCountFlagBits::e8, vk::True, 0.5F);
  uTransparentPipeline_ = trans_builder.build(dev);
  transparentPipeline = *uTransparentPipeline_;

  auto ray_vert = graphics::SpirVShaderModule{
      dev,
      shaders::shaderPath(shaders::kRaySphereVertShaderPath),
  };
  auto ray_frag = graphics::SpirVShaderModule{
      dev,
      shaders::shaderPath(shaders::kRaySphereMaterialFragShaderPath),
  };
  auto opaque_ray_builder =
      graphics::pipeline::GraphicsPipelineBuilder{uRaySphereLayout_->get()};
  opaque_ray_builder
      .addShaderStage(
          ray_vert.stageCreateInfo(vk::ShaderStageFlagBits::eVertex))
      .addShaderStage(
          ray_frag.stageCreateInfo(vk::ShaderStageFlagBits::eFragment))
      .setVertexInput({}, {})
      .setRenderingFormats({vk::Format::eR8G8B8A8Unorm}, vk::Format::eD32Sfloat)
      .setDepthState(vk::True, vk::True)
      .setCullMode(vk::CullModeFlagBits::eNone)
      .setMultisampling(vk::SampleCountFlagBits::e8, vk::True, 0.5F);
  uRaySphereOpaquePipeline_ = opaque_ray_builder.build(dev);
  opaqueRaySpherePipeline = *uRaySphereOpaquePipeline_;

  auto trans_ray_builder =
      graphics::pipeline::GraphicsPipelineBuilder{uRaySphereLayout_->get()};
  trans_ray_builder
      .addShaderStage(
          ray_vert.stageCreateInfo(vk::ShaderStageFlagBits::eVertex))
      .addShaderStage(
          ray_frag.stageCreateInfo(vk::ShaderStageFlagBits::eFragment))
      .setVertexInput({}, {})
      .setRenderingFormats({vk::Format::eR8G8B8A8Unorm}, vk::Format::eD32Sfloat)
      .setDepthState(vk::True, vk::True)
      .setCullMode(vk::CullModeFlagBits::eNone)
      .setColorBlendAttachment(0, graphics::pipeline::blend::kAlpha)
      .setMultisampling(vk::SampleCountFlagBits::e8, vk::True, 0.5F);
  uRaySphereTransparentPipeline_ = trans_ray_builder.build(dev);
  transparentRaySpherePipeline = *uRaySphereTransparentPipeline_;

  auto sky_vert = graphics::SpirVShaderModule{
      dev,
      shaders::shaderPath(shaders::kSkyboxVertShaderPath),
  };
  auto sky_frag = graphics::SpirVShaderModule{
      dev,
      shaders::shaderPath(shaders::kSkyboxFragShaderPath),
  };
  auto sky_builder =
      graphics::pipeline::GraphicsPipelineBuilder{uSkyboxLayout_->get()};
  sky_builder
      .addShaderStage(
          sky_vert.stageCreateInfo(vk::ShaderStageFlagBits::eVertex))
      .addShaderStage(
          sky_frag.stageCreateInfo(vk::ShaderStageFlagBits::eFragment))
      .setVertexInput({}, {})
      .setRenderingFormats({vk::Format::eR8G8B8A8Unorm}, vk::Format::eD32Sfloat)
      .setDepthState(vk::False, vk::False)
      .setCullMode(vk::CullModeFlagBits::eNone)
      .setMultisampling(vk::SampleCountFlagBits::e8, vk::True, 0.5F);
  uSkyboxPipeline_ = sky_builder.build(dev);
  skyboxPipeline = *uSkyboxPipeline_;

  vk::DescriptorPoolSize pool_sizes[] = {{
      vk::DescriptorType::eUniformBuffer,
      framesInFlight * 4,
  }};
  vk::DescriptorPoolCreateInfo pool_info{};
  pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
      .setMaxSets(framesInFlight * 2)
      .setPoolSizes(pool_sizes);
  descriptorPool_ = dev.createDescriptorPoolUnique(pool_info);

  frames_.resize(framesInFlight);
  for (std::uint32_t i = 0; i < framesInFlight; ++i) {
    auto& frame = frames_[i];
    auto usage = static_cast<dataformat::BufferUsageFlags>(
        vk::BufferUsageFlagBits::eUniformBuffer);

    frame.sceneCameraBuffer =
        std::make_unique<graphics::DescriptorBuffer>(device_.allocator, usage);
    frame.materialCameraBuffer =
        std::make_unique<graphics::DescriptorBuffer>(device_.allocator, usage);
    frame.environmentBuffer =
        std::make_unique<graphics::DescriptorBuffer>(device_.allocator, usage);

    vk::DescriptorSetAllocateInfo alloc_info{*descriptorPool_, 1,
                                             &sceneSetLayout_->get()};
    frame.sceneDescriptorSet = dev.allocateDescriptorSets(alloc_info)[0];
    frame.materialDescriptorSet = dev.allocateDescriptorSets(alloc_info)[0];

    auto scene_cam = frame.sceneCameraBuffer->descriptorInfo();
    auto mat_cam = frame.materialCameraBuffer->descriptorInfo();
    auto env_buf = frame.environmentBuffer->descriptorInfo();

    std::array<vk::WriteDescriptorSet, 4> writes = {
        vk::WriteDescriptorSet{
            frame.sceneDescriptorSet,
            dsl::SceneSetLayout::kCameraBinding,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &scene_cam,
            nullptr,
        },
        vk::WriteDescriptorSet{
            frame.sceneDescriptorSet,
            dsl::SceneSetLayout::kEnvironmentBinding,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &env_buf,
            nullptr,
        },
        vk::WriteDescriptorSet{
            frame.materialDescriptorSet,
            dsl::SceneSetLayout::kCameraBinding,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &mat_cam,
            nullptr,
        },
        vk::WriteDescriptorSet{
            frame.materialDescriptorSet,
            dsl::SceneSetLayout::kEnvironmentBinding,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &env_buf,
            nullptr,
        },
    };
    dev.updateDescriptorSets(writes, nullptr);
  }
}

void SceneRenderer::updateUniforms(std::uint32_t frameIndex,
                                   const types::CameraUBO& sceneCam,
                                   const types::CameraUBO& matCam,
                                   const env::EnvironmentParams& envParams) {
  auto& frame = frames_[frameIndex];
  std::ignore =
      frame.sceneCameraBuffer->writeAt(std::as_bytes(std::span{&sceneCam, 1}));
  std::ignore =
      frame.materialCameraBuffer->writeAt(std::as_bytes(std::span{&matCam, 1}));
  std::ignore =
      frame.environmentBuffer->writeAt(std::as_bytes(std::span{&envParams, 1}));
}

};  // namespace vkit::renderer

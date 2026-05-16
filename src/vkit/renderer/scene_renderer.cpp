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
  lightSetLayout_ = std::make_unique<dsl::LightSetLayout>(dev);

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

  uShadowLayout_ = std::make_unique<pl::ShadowPipelineLayout>(
      dev, std::forward_as_tuple(*lightSetLayout_, *primitiveSetLayout_));
  shadowLayout = uShadowLayout_.get();

  uLightGizmoLayout_ = std::make_unique<pl::LightGizmoPipelineLayout>(
      dev, std::forward_as_tuple(*sceneSetLayout_));
  lightGizmoLayout = uLightGizmoLayout_.get();

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
      .setMultisampling(vk::SampleCountFlagBits::e8);
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
      .setCullMode(vk::CullModeFlagBits::eBack)
      .setColorBlendAttachment(0, graphics::pipeline::blend::kAlpha)
      .setMultisampling(vk::SampleCountFlagBits::e8);
  uTransparentPipeline_ = trans_builder.build(dev);
  transparentPipeline = *uTransparentPipeline_;

  auto trans_bf_builder =
      graphics::pipeline::GraphicsPipelineBuilder{uPrimLayout_->get()};
  trans_bf_builder
      .addShaderStage(
          prim_vert.stageCreateInfo(vk::ShaderStageFlagBits::eVertex))
      .addShaderStage(
          prim_frag.stageCreateInfo(vk::ShaderStageFlagBits::eFragment))
      .setVertexInput({}, {})
      .setRenderingFormats({vk::Format::eR8G8B8A8Unorm}, vk::Format::eD32Sfloat)
      .setDepthState(vk::True, vk::False)
      .setCullMode(vk::CullModeFlagBits::eFront)
      .setColorBlendAttachment(0, graphics::pipeline::blend::kAlpha)
      .setMultisampling(vk::SampleCountFlagBits::e8);
  uTransparentBackFacePipeline_ = trans_bf_builder.build(dev);
  transparentBackFacePipeline = *uTransparentBackFacePipeline_;

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
      .setMultisampling(vk::SampleCountFlagBits::e8);
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
      .setCullMode(vk::CullModeFlagBits::eBack)
      .setColorBlendAttachment(0, graphics::pipeline::blend::kAlpha)
      .setMultisampling(vk::SampleCountFlagBits::e8);
  uRaySphereTransparentPipeline_ = trans_ray_builder.build(dev);
  transparentRaySpherePipeline = *uRaySphereTransparentPipeline_;

  auto trans_ray_bf_builder =
      graphics::pipeline::GraphicsPipelineBuilder{uRaySphereLayout_->get()};
  trans_ray_bf_builder
      .addShaderStage(
          ray_vert.stageCreateInfo(vk::ShaderStageFlagBits::eVertex))
      .addShaderStage(
          ray_frag.stageCreateInfo(vk::ShaderStageFlagBits::eFragment))
      .setVertexInput({}, {})
      .setRenderingFormats({vk::Format::eR8G8B8A8Unorm}, vk::Format::eD32Sfloat)
      .setDepthState(vk::True, vk::True)
      .setCullMode(vk::CullModeFlagBits::eFront)
      .setColorBlendAttachment(0, graphics::pipeline::blend::kAlpha)
      .setMultisampling(vk::SampleCountFlagBits::e8);
  uRaySphereTransparentBackFacePipeline_ = trans_ray_bf_builder.build(dev);
  transparentRaySphereBackFacePipeline = *uRaySphereTransparentBackFacePipeline_;

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
      .setMultisampling(vk::SampleCountFlagBits::e8);
  uSkyboxPipeline_ = sky_builder.build(dev);
  skyboxPipeline = *uSkyboxPipeline_;

  auto shadow_vert = graphics::SpirVShaderModule{
      dev,
      shaders::shaderPath(shaders::kShadowVertShaderPath),
  };
  auto shadow_builder =
      graphics::pipeline::GraphicsPipelineBuilder{uShadowLayout_->get()};
  shadow_builder
      .addShaderStage(
          shadow_vert.stageCreateInfo(vk::ShaderStageFlagBits::eVertex))
      .setVertexInput({}, {})
      .setRenderingFormats({}, vk::Format::eD32Sfloat)
      .setDepthState(vk::True, vk::True)
      .setCullMode(vk::CullModeFlagBits::eFront)
      .setMultisampling(vk::SampleCountFlagBits::e1)
      .clearColorBlendAttachments();
  uShadowPipeline_ = shadow_builder.build(dev);
  shadowPipeline = *uShadowPipeline_;

  auto gizmo_vert = graphics::SpirVShaderModule{
      dev,
      shaders::shaderPath(shaders::kLightGizmoVertShaderPath),
  };
  auto gizmo_frag = graphics::SpirVShaderModule{
      dev,
      shaders::shaderPath(shaders::kLightGizmoFragShaderPath),
  };
  auto gizmo_builder =
      graphics::pipeline::GraphicsPipelineBuilder{uLightGizmoLayout_->get()};
  gizmo_builder
      .addShaderStage(
          gizmo_vert.stageCreateInfo(vk::ShaderStageFlagBits::eVertex))
      .addShaderStage(
          gizmo_frag.stageCreateInfo(vk::ShaderStageFlagBits::eFragment))
      .setVertexInput({}, {})
      .setRenderingFormats({vk::Format::eR8G8B8A8Unorm}, vk::Format::eD32Sfloat)
      .setDepthState(vk::True, vk::False)
      .setCullMode(vk::CullModeFlagBits::eNone)
      .setColorBlendAttachment(0, graphics::pipeline::blend::kAlpha)
      .setMultisampling(vk::SampleCountFlagBits::e8);
  uLightGizmoPipeline_ = gizmo_builder.build(dev);
  lightGizmoPipeline = *uLightGizmoPipeline_;

  auto outline_vert = graphics::SpirVShaderModule{
      dev,
      shaders::shaderPath(shaders::kOutlineVertShaderPath),
  };
  auto outline_frag = graphics::SpirVShaderModule{
      dev,
      shaders::shaderPath(shaders::kOutlineFragShaderPath),
  };
  auto outline_builder =
      graphics::pipeline::GraphicsPipelineBuilder{uPrimLayout_->get()};
  outline_builder
      .addShaderStage(
          outline_vert.stageCreateInfo(vk::ShaderStageFlagBits::eVertex))
      .addShaderStage(
          outline_frag.stageCreateInfo(vk::ShaderStageFlagBits::eFragment))
      .setVertexInput({}, {})
      .setRenderingFormats({vk::Format::eR8G8B8A8Unorm}, vk::Format::eD32Sfloat)
      .setDepthState(vk::True, vk::False)
      .setCullMode(vk::CullModeFlagBits::eFront)
      .setMultisampling(vk::SampleCountFlagBits::e8);
  uOutlinePipeline_ = outline_builder.build(dev);
  outlinePipeline = *uOutlinePipeline_;

  // Shadow map depth image
  {
    vk::ImageCreateInfo img_info{};
    img_info.setImageType(vk::ImageType::e2D)
        .setFormat(vk::Format::eD32Sfloat)
        .setExtent({kShadowMapSize, kShadowMapSize, 1})
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment |
                  vk::ImageUsageFlagBits::eSampled);
    shadowImage_ =
        graphics::AllocatedImage{device_.allocator, img_info,
                                 graphics::allocation::kDeviceLocal};

    vk::ImageViewCreateInfo view_info{};
    view_info.setImage(static_cast<vk::Image>(shadowImage_))
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(vk::Format::eD32Sfloat)
        .setSubresourceRange({vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
    shadowView_ = dev.createImageViewUnique(view_info);

    vk::SamplerCreateInfo sampler_info{};
    sampler_info.setMagFilter(vk::Filter::eLinear)
        .setMinFilter(vk::Filter::eLinear)
        .setAddressModeU(vk::SamplerAddressMode::eClampToBorder)
        .setAddressModeV(vk::SamplerAddressMode::eClampToBorder)
        .setAddressModeW(vk::SamplerAddressMode::eClampToBorder)
        .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
        .setCompareEnable(vk::True)
        .setCompareOp(vk::CompareOp::eLessOrEqual)
        .setMinLod(0.0F)
        .setMaxLod(0.0F);
    shadowSampler_ = dev.createSamplerUnique(sampler_info);
  }

  // Per frame descriptor pools:
  //   sceneSet:    cam(UBO) + env(UBO) + params(UBO) + lights(SSBO) + shadowMap(CIS)
  //   materialSet: matCam(UBO) + env(UBO) + params(UBO) + lights(SSBO) + shadowMap(CIS)
  //   lightSet:    shadowLight(UBO)
  vk::DescriptorPoolSize pool_sizes[] = {
      {vk::DescriptorType::eUniformBuffer, framesInFlight * 7},
      {vk::DescriptorType::eStorageBuffer, framesInFlight * 2},
      {vk::DescriptorType::eCombinedImageSampler, framesInFlight * 2},
  };
  vk::DescriptorPoolCreateInfo pool_info{};
  pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
      .setMaxSets(framesInFlight * 3)
      .setPoolSizes(pool_sizes);
  descriptorPool_ = dev.createDescriptorPoolUnique(pool_info);

  frames_.resize(framesInFlight);
  for (std::uint32_t i = 0; i < framesInFlight; ++i) {
    auto& frame = frames_[i];
    auto ubo_usage = static_cast<dataformat::BufferUsageFlags>(
        vk::BufferUsageFlagBits::eUniformBuffer);
    auto ssbo_usage = static_cast<dataformat::BufferUsageFlags>(
        vk::BufferUsageFlagBits::eStorageBuffer);

    frame.sceneCameraBuffer =
        std::make_unique<graphics::DescriptorBuffer>(device_.allocator, ubo_usage);
    frame.materialCameraBuffer =
        std::make_unique<graphics::DescriptorBuffer>(device_.allocator, ubo_usage);
    frame.environmentBuffer =
        std::make_unique<graphics::DescriptorBuffer>(device_.allocator, ubo_usage);
    frame.sceneParamsBuffer =
        std::make_unique<graphics::DescriptorBuffer>(device_.allocator, ubo_usage);
    frame.lightsBuffer =
        std::make_unique<graphics::DescriptorBuffer>(device_.allocator, ssbo_usage,
                                                     sizeof(types::LightsSSBO));
    frame.shadowLightBuffer =
        std::make_unique<graphics::DescriptorBuffer>(device_.allocator, ubo_usage);

    vk::DescriptorSetAllocateInfo scene_alloc{*descriptorPool_, 1,
                                              &sceneSetLayout_->get()};
    frame.sceneDescriptorSet = dev.allocateDescriptorSets(scene_alloc)[0];
    frame.materialDescriptorSet = dev.allocateDescriptorSets(scene_alloc)[0];

    vk::DescriptorSetAllocateInfo light_alloc{*descriptorPool_, 1,
                                              &lightSetLayout_->get()};
    frame.lightDescriptorSet = dev.allocateDescriptorSets(light_alloc)[0];

    auto scene_cam    = frame.sceneCameraBuffer->descriptorInfo();
    auto mat_cam      = frame.materialCameraBuffer->descriptorInfo();
    auto env_buf      = frame.environmentBuffer->descriptorInfo();
    auto params_buf   = frame.sceneParamsBuffer->descriptorInfo();
    auto lights_buf   = frame.lightsBuffer->descriptorInfo();
    auto shadow_light = frame.shadowLightBuffer->descriptorInfo();

    vk::DescriptorImageInfo shadow_img_info{};
    shadow_img_info.setSampler(*shadowSampler_)
        .setImageView(*shadowView_)
        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    std::array<vk::WriteDescriptorSet, 11> writes = {
        // --- sceneDescriptorSet ---
        vk::WriteDescriptorSet{frame.sceneDescriptorSet,
                               dsl::SceneSetLayout::kCameraBinding,
                               0, 1, vk::DescriptorType::eUniformBuffer,
                               nullptr, &scene_cam, nullptr},
        vk::WriteDescriptorSet{frame.sceneDescriptorSet,
                               dsl::SceneSetLayout::kEnvironmentBinding,
                               0, 1, vk::DescriptorType::eUniformBuffer,
                               nullptr, &env_buf, nullptr},
        vk::WriteDescriptorSet{frame.sceneDescriptorSet,
                               dsl::SceneSetLayout::kSceneParamsBinding,
                               0, 1, vk::DescriptorType::eUniformBuffer,
                               nullptr, &params_buf, nullptr},
        vk::WriteDescriptorSet{frame.sceneDescriptorSet,
                               dsl::SceneSetLayout::kLightBinding,
                               0, 1, vk::DescriptorType::eStorageBuffer,
                               nullptr, &lights_buf, nullptr},
        vk::WriteDescriptorSet{frame.sceneDescriptorSet,
                               dsl::SceneSetLayout::kShadowMapBinding,
                               0, 1, vk::DescriptorType::eCombinedImageSampler,
                               &shadow_img_info, nullptr, nullptr},
        // --- materialDescriptorSet ---
        vk::WriteDescriptorSet{frame.materialDescriptorSet,
                               dsl::SceneSetLayout::kCameraBinding,
                               0, 1, vk::DescriptorType::eUniformBuffer,
                               nullptr, &mat_cam, nullptr},
        vk::WriteDescriptorSet{frame.materialDescriptorSet,
                               dsl::SceneSetLayout::kEnvironmentBinding,
                               0, 1, vk::DescriptorType::eUniformBuffer,
                               nullptr, &env_buf, nullptr},
        vk::WriteDescriptorSet{frame.materialDescriptorSet,
                               dsl::SceneSetLayout::kSceneParamsBinding,
                               0, 1, vk::DescriptorType::eUniformBuffer,
                               nullptr, &params_buf, nullptr},
        vk::WriteDescriptorSet{frame.materialDescriptorSet,
                               dsl::SceneSetLayout::kLightBinding,
                               0, 1, vk::DescriptorType::eStorageBuffer,
                               nullptr, &lights_buf, nullptr},
        vk::WriteDescriptorSet{frame.materialDescriptorSet,
                               dsl::SceneSetLayout::kShadowMapBinding,
                               0, 1, vk::DescriptorType::eCombinedImageSampler,
                               &shadow_img_info, nullptr, nullptr},
        // --- lightDescriptorSet (shadow pass: viewProj only) ---
        vk::WriteDescriptorSet{frame.lightDescriptorSet,
                               dsl::LightSetLayout::kLightBinding,
                               0, 1, vk::DescriptorType::eUniformBuffer,
                               nullptr, &shadow_light, nullptr},
    };
    dev.updateDescriptorSets(writes, nullptr);
  }
}

void SceneRenderer::updateUniforms(std::uint32_t frameIndex,
                                   const types::CameraUBO& sceneCam,
                                   const types::CameraUBO& matCam,
                                   const env::EnvironmentParams& envParams,
                                   const types::SceneParamsUBO& sceneParams,
                                   const types::LightsSSBO& lightsData,
                                   const types::ShadowLightUBO& shadowLight) {
  auto& frame = frames_[frameIndex];
  std::ignore =
      frame.sceneCameraBuffer->writeAt(std::as_bytes(std::span{&sceneCam, 1}));
  std::ignore =
      frame.materialCameraBuffer->writeAt(std::as_bytes(std::span{&matCam, 1}));
  std::ignore =
      frame.environmentBuffer->writeAt(std::as_bytes(std::span{&envParams, 1}));
  std::ignore =
      frame.sceneParamsBuffer->writeAt(std::as_bytes(std::span{&sceneParams, 1}));
  std::ignore =
      frame.lightsBuffer->writeAt(std::as_bytes(std::span{&lightsData, 1}));
  std::ignore =
      frame.shadowLightBuffer->writeAt(std::as_bytes(std::span{&shadowLight, 1}));
}

};  // namespace vkit::renderer

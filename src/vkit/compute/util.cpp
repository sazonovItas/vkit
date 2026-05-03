#include "vkit/compute/util.hpp"

namespace vkit::compute::util {

auto makeOutputTexture(const graphics::GfxDevice& device, uint32_t w,
                       uint32_t h, vk::Format fmt, vk::Sampler sampler,
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

};  // namespace vkit::compute::util

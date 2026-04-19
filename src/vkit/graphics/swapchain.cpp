#include "vkit/graphics/swapchain.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "vkit/graphics/util.hpp"

namespace {

constexpr auto kSrgbFormatsV = std::array{
    vk::Format::eR8G8B8A8Srgb,
    vk::Format::eB8G8R8A8Srgb,
};

auto needsRecreation(vk::Result result) -> bool {
  switch (result) {
    case vk::Result::eSuccess:
    case vk::Result::eSuboptimalKHR:
      return false;
    case vk::Result::eErrorOutOfDateKHR:
      return true;
    default:
      throw std::runtime_error{"Swapchain error"};
  }
}

auto getSurfaceFormat(std::span<const vk::SurfaceFormatKHR> supported)
    -> vk::SurfaceFormatKHR {
  for (auto desired : kSrgbFormatsV) {
    auto it = std::ranges::find_if(supported, [=](auto const& f) {
      return f.format == desired &&
             f.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear;
    });

    if (it != supported.end()) {
      return *it;
    }
  }

  return supported.front();
}

auto getImageExtent(const vk::SurfaceCapabilitiesKHR& caps,
                    const glm::ivec2 size) -> vk::Extent2D {
  constexpr auto kLimitless = 0xFFFFFFFF;

  if (caps.currentExtent.width != kLimitless) {
    return caps.currentExtent;
  }

  return {std::clamp(static_cast<std::uint32_t>(size.x),
                     caps.minImageExtent.width, caps.maxImageExtent.width),
          std::clamp(static_cast<std::uint32_t>(size.y),
                     caps.minImageExtent.height, caps.maxImageExtent.height)};
}

auto getImageCount(const vk::SurfaceCapabilitiesKHR& capabilities,
                   std::uint32_t minImageCount) -> std::uint32_t {
  std::uint32_t max = capabilities.maxImageCount;
  if (max == 0) {
    return std::max(minImageCount, capabilities.minImageCount);
  }

  return std::clamp(minImageCount, capabilities.minImageCount, max);
}

constexpr auto kColorRange = [] {
  vk::ImageSubresourceRange r{};
  r.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLevelCount(1)
      .setLayerCount(1);
  return r;
}();

}  // namespace

namespace vkit::graphics {

Swapchain::Swapchain(const GfxDevice& gfxDevice, const vk::SurfaceKHR& surface,
                     std::uint32_t minImageCount, const glm::ivec2 size)
    : device_{gfxDevice},
      ci_{createSwapchainCreateInfo(surface, minImageCount)} {
  if (!recreate(size)) {
    throw std::runtime_error{"Failed to create swapchain"};
  }
}

auto Swapchain::recreate(const glm::ivec2 size) -> bool {
  if (size.x <= 0 || size.y <= 0) {
    return false;
  }

  auto caps = device_.physicalDevice.getSurfaceCapabilitiesKHR(ci_.surface);

  ci_.setImageExtent(getImageExtent(caps, size))
      .setOldSwapchain(swapchain_ ? *swapchain_ : vk::SwapchainKHR{})
      .setQueueFamilyIndices(device_.queueFamilies.graphicsPresent);

  // TODO: itas - it's better to remove wait idle to not block device
  // almost forever until generation of some texture is not finished
  // HACK: for a momemnt it's just commented but need wait in some way
  // need to be resolved on the higher level
  // device_.get().waitIdle();

  swapchain_ = device_.get().createSwapchainKHRUnique(ci_);

  recreateImages();
  recreateImageViews();

  return true;
}

auto Swapchain::getExtent() const -> vk::Extent2D { return ci_.imageExtent; }

auto Swapchain::getFormat() const -> dataformat::Format {
  return ci_.imageFormat;
}

auto Swapchain::acquireNextImage(vk::Semaphore signal)
    -> std::optional<std::uint32_t> {
  static constexpr auto kTimeout = std::numeric_limits<std::uint64_t>::max();

  std::uint32_t index = 0;

  auto result = device_.get().acquireNextImageKHR(*swapchain_, kTimeout, signal,
                                                  nullptr, &index);

  if (needsRecreation(result)) {
    return std::nullopt;
  }

  return index;
}

auto Swapchain::present(std::uint32_t imageIndex, vk::Semaphore waitSemaphore)
    -> bool {
  vk::PresentInfoKHR info{};
  info.setSwapchains(*swapchain_)
      .setImageIndices(imageIndex)
      .setWaitSemaphores(waitSemaphore);

  auto result = device_.queues.graphicsPresent.presentKHR(&info);

  return !needsRecreation(result);
}

auto Swapchain::getImage(std::uint32_t index) const -> vk::Image {
  return images_.at(index);
}

auto Swapchain::getImageView(std::uint32_t index) const -> vk::ImageView {
  return *imageViews_.at(index);
}

auto Swapchain::imageBaseBarrier(std::uint32_t index) const
    -> vk::ImageMemoryBarrier2 {
  vk::ImageMemoryBarrier2 barrier{};
  barrier.setImage(images_.at(index))
      .setSubresourceRange(kColorRange)
      .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
      .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);

  return barrier;
}

void Swapchain::recreateImages() {
  std::uint32_t count = 0;

  auto result =
      device_.get().getSwapchainImagesKHR(*swapchain_, &count, nullptr);
  util::requireSuccess(result, "Failed to get swapchain images");

  images_.resize(count);

  result =
      device_.get().getSwapchainImagesKHR(*swapchain_, &count, images_.data());
  util::requireSuccess(result, "Failed to get swapchain images");
}

void Swapchain::recreateImageViews() {
  vk::ImageViewCreateInfo ci{};
  ci.setViewType(vk::ImageViewType::e2D)
      .setFormat(ci_.imageFormat)
      .setSubresourceRange(kColorRange);

  imageViews_.clear();
  imageViews_.reserve(images_.size());

  for (auto img : images_) {
    ci.setImage(img);
    imageViews_.push_back(device_.get().createImageViewUnique(ci));
  }
}

auto Swapchain::createSwapchainCreateInfo(const vk::SurfaceKHR& surface,
                                          std::uint32_t minImageCount)
    -> vk::SwapchainCreateInfoKHR {
  auto format =
      getSurfaceFormat(device_.physicalDevice.getSurfaceFormatsKHR(surface));

  auto capabilities = device_.physicalDevice.getSurfaceCapabilitiesKHR(surface);

  auto image_count = getImageCount(capabilities, minImageCount);

  vk::SwapchainCreateInfoKHR ci{};
  ci.setSurface(surface)
      .setMinImageCount(image_count)
      .setImageFormat(format.format)
      .setImageColorSpace(format.colorSpace)
      .setImageArrayLayers(1)
      .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
      .setPresentMode(vk::PresentModeKHR::eFifo);

  return ci;
}

};  // namespace vkit::graphics

#include "swapchain.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>

#include "render_target.hpp"
#include "resource_buffering.hpp"
#include "vku/images/allocated_image.hpp"
#include "vku/utils/utils.hpp"
#include "vulkan/vulkan.hpp"

namespace lvk {
namespace {
constexpr auto kSrgbFormatsV = std::array{
    vk::Format::eR8G8B8A8Srgb,
    vk::Format::eB8G8R8A8Srgb,
};

[[nodiscard]] constexpr auto getSurfaceFormat(
    std::span<vk::SurfaceFormatKHR const> supported) -> vk::SurfaceFormatKHR {
  for (auto const desired : kSrgbFormatsV) {
    auto const is_match = [desired](vk::SurfaceFormatKHR const& in) {
      return in.format == desired &&
             in.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear;
    };

    auto const it = std::ranges::find_if(supported, is_match);
    if (it == supported.end()) {
      continue;
    }

    return *it;
  }

  return supported.front();
}

constexpr std::uint32_t kMinImagesV{kResourceBufferingV};

[[nodiscard]] constexpr auto getImageExtent(
    vk::SurfaceCapabilitiesKHR const& capabilities, glm::uvec2 const size)
    -> vk::Extent2D {
  constexpr auto kLimitlessV = 0xFFFFFFFF;

  if (capabilities.currentExtent.width < kLimitlessV &&
      capabilities.currentExtent.height < kLimitlessV) {
    return capabilities.currentExtent;
  }

  const auto x = std::clamp(size.x, capabilities.minImageExtent.width,
                            capabilities.maxImageExtent.width);
  const auto y = std::clamp(size.y, capabilities.minImageExtent.height,
                            capabilities.maxImageExtent.height);

  return vk::Extent2D{x, y};
}

[[nodiscard]] constexpr auto getImageCount(
    vk::SurfaceCapabilitiesKHR const& capabilities) -> std::uint32_t {
  if (capabilities.maxImageCount < capabilities.minImageCount) {
    return std::max(kMinImagesV, capabilities.minImageCount);
  }

  return std::clamp(kMinImagesV, capabilities.minImageCount,
                    capabilities.maxImageCount);
}

auto needsRecreation(vk::Result const result) -> bool {
  switch (result) {
    case vk::Result::eSuccess:
    case vk::Result::eSuboptimalKHR:
      return false;
    case vk::Result::eErrorOutOfDateKHR:
      return true;
    default:
      break;
  }

  throw std::runtime_error{"swapchain error"};
}

constexpr auto kColorSubresourceRangeV = [] {
  auto ret = vk::ImageSubresourceRange{};
  ret.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1)
      .setLevelCount(1);
  return ret;
}();

};  // namespace

Swapchain::Swapchain(vk::Device device, vk::PhysicalDevice physical_device,
                     const std::uint32_t queueFamily, vma::Allocator allocator,
                     vk::SurfaceKHR surface, glm::ivec2 const size)
    : device_(device),
      physicalDevice_(physical_device),
      queueFamily_(queueFamily),
      allocator_(allocator) {
  auto const surface_format =
      getSurfaceFormat(physicalDevice_.getSurfaceFormatsKHR(surface));
  ci_.setSurface(surface)
      .setImageFormat(surface_format.format)
      .setImageColorSpace(surface_format.colorSpace)
      .setImageArrayLayers(1)
      .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
      .setPresentMode(vk::PresentModeKHR::eFifo);
  if (!recreate(size)) {
    throw std::runtime_error{"Failed to create Vulkan Swapchain"};
  }
}

auto Swapchain::recreate(glm::ivec2 size) -> bool {
  if (size.x <= 0 || size.y <= 0) return false;

  auto const capabilities =
      physicalDevice_.getSurfaceCapabilitiesKHR(ci_.surface);
  ci_.setImageExtent(getImageExtent(capabilities, size))
      .setMinImageCount(getImageCount(capabilities))
      .setOldSwapchain(swapchain_ ? *swapchain_ : vk::SwapchainKHR{})
      .setQueueFamilyIndices(queueFamily_);
  assert(ci_.imageExtent.width > 0 && ci_.imageExtent.height > 0 &&
         ci_.minImageCount >= kMinImagesV);

  device_.waitIdle();
  swapchain_ = device_.createSwapchainKHRUnique(ci_);

  populateImages();
  createImageViews();
  createPresentSemaphores();

  populateColorImage();
  populateDepthImage();

  return true;
}

auto Swapchain::acquireNextImage(vk::Semaphore const to_signal)
    -> std::optional<RenderTarget> {
  assert(!imageIdx_);

  static constexpr auto kTimeoutV = std::numeric_limits<std::uint64_t>::max();

  auto image_index = std::uint32_t{};
  auto const result = device_.acquireNextImageKHR(*swapchain_, kTimeoutV,
                                                  to_signal, {}, &image_index);
  if (needsRecreation(result)) {
    return {};
  }

  imageIdx_ = static_cast<std::size_t>(image_index);

  return RenderTarget{
      .swapchainImage = images_.at(*imageIdx_),
      .swapchainImageView = *imageViews_.at(*imageIdx_),
      .colorImage = colorImage_.image,
      .colorImageView = *colorImageView_,
      .depthImage = depthImage_.image,
      .depthImageView = *depthImageView_,
      .extent = ci_.imageExtent,
  };
}

auto Swapchain::baseBarrier() const -> vk::ImageMemoryBarrier2 {
  auto ret = vk::ImageMemoryBarrier2{};
  ret.setImage(images_.at(imageIdx_.value()))
      .setSubresourceRange(kColorSubresourceRangeV)
      .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
      .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);
  return ret;
};

auto Swapchain::baseColorBarrier() const -> vk::ImageMemoryBarrier2 {
  auto ret = vk::ImageMemoryBarrier2{};
  ret.setImage(colorImage_.image)
      .setSubresourceRange(colorImage_.subresourceRange())
      .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
      .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);
  return ret;
}

auto Swapchain::baseDepthBarrier() const -> vk::ImageMemoryBarrier2 {
  auto ret = vk::ImageMemoryBarrier2{};
  ret.setImage(depthImage_.image)
      .setSubresourceRange(depthImage_.subresourceRange())
      .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
      .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);
  return ret;
}

auto Swapchain::getPresentSemaphore() const -> vk::Semaphore {
  return *presentSemaphores_.at(imageIdx_.value());
}

auto Swapchain::present(vk::Queue const queue) -> bool {
  auto const image_index = static_cast<std::uint32_t>(imageIdx_.value());
  auto const wait_semaphore =
      *presentSemaphores_.at(static_cast<std::size_t>(image_index));
  auto present_info = vk::PresentInfoKHR{};
  present_info.setSwapchains(*swapchain_)
      .setImageIndices(image_index)
      .setWaitSemaphores(wait_semaphore);

  auto const result = queue.presentKHR(&present_info);
  imageIdx_.reset();

  return !needsRecreation(result);
}

void Swapchain::populateImages() {
  auto image_count = std::uint32_t{};
  auto result =
      device_.getSwapchainImagesKHR(*swapchain_, &image_count, nullptr);
  vku::requireSuccess(result, "failed to get Swapchain Images");

  images_.resize(image_count);
  result =
      device_.getSwapchainImagesKHR(*swapchain_, &image_count, images_.data());
  vku::requireSuccess(result, "failed to get swapchain images");
}

void Swapchain::populateColorImage() {
  auto image_ci = vk::ImageCreateInfo{}
                      .setImageType(vk::ImageType::e2D)
                      .setUsage(vk::ImageUsageFlagBits::eColorAttachment)
                      .setExtent(vku::toExtent3D(ci_.imageExtent))
                      .setFormat(ci_.imageFormat)
                      .setSamples(kSampleCount)
                      .setMipLevels(1)
                      .setArrayLayers(1);
  colorImage_ = {allocator_, image_ci};

  colorImageView_ =
      device_.createImageViewUnique(colorImage_.getViewCreateInfo());
}

void Swapchain::populateDepthImage() {
  auto image_ci = vk::ImageCreateInfo{}
                      .setImageType(vk::ImageType::e2D)
                      .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
                      .setExtent(vku::toExtent3D(ci_.imageExtent))
                      .setFormat(vk::Format::eD32SfloatS8Uint)
                      .setSamples(kSampleCount)
                      .setMipLevels(1)
                      .setArrayLayers(1);
  depthImage_ = {allocator_, image_ci};

  depthImageView_ =
      device_.createImageViewUnique(depthImage_.getViewCreateInfo());
}

void Swapchain::createImageViews() {
  auto subresource_range = vk::ImageSubresourceRange{};
  subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1)
      .setLevelCount(1);

  auto image_view_ci = vk::ImageViewCreateInfo{};
  image_view_ci.setViewType(vk::ImageViewType::e2D)
      .setFormat(ci_.imageFormat)
      .setSubresourceRange(subresource_range);

  imageViews_.clear();
  imageViews_.reserve(images_.size());
  for (auto const image : images_) {
    image_view_ci.setImage(image);
    imageViews_.push_back(device_.createImageViewUnique(image_view_ci));
  }
}

void Swapchain::createPresentSemaphores() {
  presentSemaphores_.clear();
  presentSemaphores_.resize(images_.size());
  for (auto& semaphore : presentSemaphores_) {
    semaphore = device_.createSemaphoreUnique({});
  }
}
};  // namespace lvk

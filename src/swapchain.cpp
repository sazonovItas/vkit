#include "swapchain.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>

#include "resource_buffering.hpp"

namespace lvk {

namespace {
constexpr auto kSrgbFormatsV = std::array{
    vk::Format::eR8G8B8A8Srgb,
    vk::Format::eB8G8R8A8Srgb,
};

[[nodiscard]] constexpr auto get_surface_format(
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

[[nodiscard]] constexpr auto get_image_extent(
    vk::SurfaceCapabilitiesKHR const& capabilities, glm::uvec2 const size)
    -> vk::Extent2D {
  constexpr auto kLimitlessV = 0xFFFFFFFF;

  if (capabilities.currentExtent.width < kLimitlessV &&
      capabilities.currentExtent.height < kLimitlessV) {
    return capabilities.currentExtent;
  }

  auto const x = std::clamp(size.x, capabilities.minImageExtent.width,
                            capabilities.maxImageExtent.width);
  auto const y = std::clamp(size.y, capabilities.minImageExtent.height,
                            capabilities.maxImageExtent.height);

  return vk::Extent2D{x, y};
}

[[nodiscard]] constexpr auto get_image_count(
    vk::SurfaceCapabilitiesKHR const& capabilities) -> std::uint32_t {
  if (capabilities.maxImageCount < capabilities.minImageCount) {
    return std::max(kMinImagesV, capabilities.minImageCount);
  }

  return std::clamp(kMinImagesV, capabilities.minImageCount,
                    capabilities.maxImageCount);
}

void require_success(vk::Result const result, char const* error_msg) {
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error{error_msg};
  }
}

auto needs_recreation(vk::Result const result) -> bool {
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

constexpr auto kSubresourceRangeV = [] {
  auto ret = vk::ImageSubresourceRange{};
  ret.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1)
      .setLevelCount(1);
  return ret;
}();
};  // namespace

Swapchain::Swapchain(vk::Device const device, Gpu const& gpu,
                     vk::SurfaceKHR const surface, glm::ivec2 const size)
    : m_device_(device), m_gpu_(gpu) {
  auto const surface_format =
      get_surface_format(m_gpu_.device.getSurfaceFormatsKHR(surface));
  m_ci_.setSurface(surface)
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
      m_gpu_.device.getSurfaceCapabilitiesKHR(m_ci_.surface);
  m_ci_.setImageExtent(get_image_extent(capabilities, size))
      .setMinImageCount(get_image_count(capabilities))
      .setOldSwapchain(m_swapchain_ ? *m_swapchain_ : vk::SwapchainKHR{})
      .setQueueFamilyIndices(m_gpu_.queue_family);
  assert(m_ci_.imageExtent.width > 0 && m_ci_.imageExtent.height > 0 &&
         m_ci_.minImageCount >= kMinImagesV);

  m_device_.waitIdle();
  m_swapchain_ = m_device_.createSwapchainKHRUnique(m_ci_);

  populate_images();
  create_image_views();
  create_present_semaphores();

  return true;
}

auto Swapchain::acquire_next_image(vk::Semaphore const to_signal)
    -> std::optional<RenderTarget> {
  assert(!m_image_index_);

  static constexpr auto kTimeoutV = std::numeric_limits<std::uint64_t>::max();

  auto image_index = std::uint32_t{};
  auto const result = m_device_.acquireNextImageKHR(
      *m_swapchain_, kTimeoutV, to_signal, {}, &image_index);
  if (needs_recreation(result)) {
    return {};
  }

  m_image_index_ = static_cast<std::size_t>(image_index);
  return RenderTarget{
      .image = m_images_.at(*m_image_index_),
      .image_view = *m_image_views_.at(*m_image_index_),
      .extent = m_ci_.imageExtent,
  };
}

auto Swapchain::base_barrier() const -> vk::ImageMemoryBarrier2 {
  auto ret = vk::ImageMemoryBarrier2{};
  ret.setImage(m_images_.at(m_image_index_.value()))
      .setSubresourceRange(kSubresourceRangeV)
      .setSrcQueueFamilyIndex(m_gpu_.queue_family)
      .setDstQueueFamilyIndex(m_gpu_.queue_family);
  return ret;
};

auto Swapchain::get_present_semaphore() const -> vk::Semaphore {
  return *m_present_semaphorses_.at(m_image_index_.value());
}

auto Swapchain::present(vk::Queue const queue) -> bool {
  auto const image_index = static_cast<std::uint32_t>(m_image_index_.value());
  auto const wait_semaphore =
      *m_present_semaphorses_.at(static_cast<std::size_t>(image_index));
  auto present_info = vk::PresentInfoKHR{};
  present_info.setSwapchains(*m_swapchain_)
      .setImageIndices(image_index)
      .setWaitSemaphores(wait_semaphore);

  auto const result = queue.presentKHR(&present_info);
  m_image_index_.reset();

  return !needs_recreation(result);
}

void Swapchain::populate_images() {
  auto image_count = std::uint32_t{};
  auto result =
      m_device_.getSwapchainImagesKHR(*m_swapchain_, &image_count, nullptr);
  require_success(result, "failed to get Swapchain Images");

  m_images_.resize(image_count);
  result = m_device_.getSwapchainImagesKHR(*m_swapchain_, &image_count,
                                           m_images_.data());
  require_success(result, "failed to get swapchain images");
}

void Swapchain::create_image_views() {
  auto subresource_range = vk::ImageSubresourceRange{};
  subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1)
      .setLevelCount(1);

  auto image_view_ci = vk::ImageViewCreateInfo{};
  image_view_ci.setViewType(vk::ImageViewType::e2D)
      .setFormat(m_ci_.imageFormat)
      .setSubresourceRange(subresource_range);
  m_image_views_.clear();
  m_image_views_.reserve(m_images_.size());
  for (auto const image : m_images_) {
    image_view_ci.setImage(image);
    m_image_views_.push_back(m_device_.createImageViewUnique(image_view_ci));
  }
}

void Swapchain::create_present_semaphores() {
  m_present_semaphorses_.clear();
  m_present_semaphorses_.resize(m_images_.size());
  for (auto& semaphore : m_present_semaphorses_) {
    semaphore = m_device_.createSemaphoreUnique({});
  }
}
};  // namespace lvk

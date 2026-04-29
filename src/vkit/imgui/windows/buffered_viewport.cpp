#include "vkit/imgui/windows/buffered_viewport.hpp"

#include <algorithm>

namespace vkit::imgui::windows {

BufferedViewport::BufferedViewport(std::string_view name, vk::Device device,
                                   vma::Allocator allocator,
                                   imgui::ImguiRenderer& imguiRenderer,
                                   std::uint32_t framesInFlight,
                                   const std::vector<vk::Format>& colorFormats,
                                   vk::Format depthFormat)
    : ViewportWindow(name, [](std::uint32_t, std::uint32_t) {}),
      device_{device},
      allocator_{allocator},
      imguiRenderer_{imguiRenderer} {
  viewports_.resize(framesInFlight);
  textureIds_.resize(framesInFlight, 0);

  for (auto& vp : viewports_) {
    for (const auto& format : colorFormats) {
      vp.addColorTarget(format);
    }

    if (depthFormat != vk::Format::eUndefined) {
      vp.setDepthTarget(depthFormat);
    }
  }
}

BufferedViewport::~BufferedViewport() {
  for (auto id : textureIds_) {
    if (id) {
      imguiRenderer_.unregisterTexture(id);
    }
  }
}

void BufferedViewport::prepareForFrame(std::uint32_t frameIndex) {
  auto target_width = std::max(1U, getWidth());
  auto target_height = std::max(1U, getHeight());

  auto& current_viewport = viewports_[frameIndex];

  current_viewport.ensureSize(device_, allocator_, target_width, target_height);

  if (!current_viewport.colorTargets.empty() &&
      current_viewport.colorTargets[0].view) {
    textureIds_[frameIndex] = imguiRenderer_.updateOrRegisterTexture(
        textureIds_[frameIndex], *current_viewport.colorTargets[0].view);

    setCurrentTexture(textureIds_[frameIndex]);
  }
}

auto BufferedViewport::getViewport(std::uint32_t frameIndex)
    -> renderer::Viewport& {
  return viewports_[frameIndex];
}

}  // namespace vkit::imgui::windows

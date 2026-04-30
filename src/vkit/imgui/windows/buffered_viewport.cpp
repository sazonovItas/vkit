#include "vkit/imgui/windows/buffered_viewport.hpp"

#include <algorithm>

namespace vkit::imgui::windows {

BufferedViewport::BufferedViewport(std::string_view name, vk::Device device,
                                   vma::Allocator allocator,
                                   imgui::ImguiRenderer& imguiRenderer,
                                   const renderer::ViewportInfo& viewportInfo,
                                   std::uint32_t framesInFlight,
                                   std::uint32_t displayTargetIndex)
    : ViewportWindow(name, [](std::uint32_t, std::uint32_t) {}),
      device_{device},
      allocator_{allocator},
      imguiRenderer_{imguiRenderer},
      displayTargetIndex_{displayTargetIndex} {
  viewports_.reserve(framesInFlight);
  for (std::uint32_t i = 0; i < framesInFlight; ++i) {
    viewports_.emplace_back(viewportInfo);
  }

  textureIds_.resize(framesInFlight, 0);
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

  if (current_viewport.colorTargets.size() > displayTargetIndex_ &&
      current_viewport.colorTargets[displayTargetIndex_].view) {
    textureIds_[frameIndex] = imguiRenderer_.updateOrRegisterTexture(
        textureIds_[frameIndex],
        *current_viewport.colorTargets[displayTargetIndex_].view);

    setCurrentTexture(textureIds_[frameIndex]);
  }
}

auto BufferedViewport::getViewport(std::uint32_t frameIndex)
    -> renderer::Viewport& {
  return viewports_[frameIndex];
}

}  // namespace vkit::imgui::windows

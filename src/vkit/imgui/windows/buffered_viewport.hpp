#pragma once

#include <string_view>
#include <vector>
#include <vk_mem_alloc.hpp>

#include "vkit/imgui/imgui_renderer.hpp"
#include "vkit/imgui/windows/viewport.hpp"
#include "vkit/renderer/viewport.hpp"

namespace vkit::imgui::windows {

class BufferedViewport : public ViewportWindow {
 public:
  BufferedViewport(std::string_view name, vk::Device device,
                   vma::Allocator allocator,
                   imgui::ImguiRenderer& imguiRenderer,
                   const renderer::ViewportInfo& viewportInfo,
                   std::uint32_t framesInFlight = 3,
                   std::uint32_t displayTargetIndex = 0);

  ~BufferedViewport() override;

  void prepareForFrame(std::uint32_t frameIndex) override;

  [[nodiscard]] auto getViewport(std::uint32_t frameIndex)
      -> renderer::Viewport&;

 private:
  vk::Device device_;
  vma::Allocator allocator_;
  imgui::ImguiRenderer& imguiRenderer_;

  std::vector<renderer::Viewport> viewports_;
  std::vector<ImTextureID> textureIds_;
  std::uint32_t displayTargetIndex_{0};
};

}  // namespace vkit::imgui::windows

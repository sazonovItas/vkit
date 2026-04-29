#pragma once

#include <string_view>
#include <vector>
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

#include "vkit/imgui/imgui_renderer.hpp"
#include "vkit/imgui/windows/viewport.hpp"
#include "vkit/renderer/viewport.hpp"

namespace vkit::imgui::windows {

class BufferedViewport : public ViewportWindow {
 public:
  BufferedViewport(std::string_view name, vk::Device device,
                   vma::Allocator allocator,
                   imgui::ImguiRenderer& imguiRenderer,
                   std::uint32_t framesInFlight = 3,
                   const std::vector<vk::Format>& colorFormats =
                       {vk::Format::eR8G8B8A8Unorm},
                   vk::Format depthFormat = vk::Format::eD32Sfloat);

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
};

}  // namespace vkit::imgui::windows

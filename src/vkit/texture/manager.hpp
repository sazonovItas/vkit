#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "vkit/item/storage.hpp"
#include "vkit/texture/texture.hpp"

namespace vkit::graphics {
class BindlessTextureManager;
}
namespace vkit::imgui {
class ImguiRenderer;
}

namespace vkit::texture {

class TextureManager : public vkit::Storage<Texture> {
 public:
  explicit TextureManager(std::uint32_t maxFramesInFlight = 3)
      : maxFramesInFlight_{maxFramesInFlight} {}

  TextureManager(TextureManager&) = delete;
  auto operator=(TextureManager&) -> TextureManager& = delete;

  ~TextureManager() override;

  TextureManager(const TextureManager&) = delete;
  auto operator=(const TextureManager&) -> TextureManager& = delete;

  auto add(const std::shared_ptr<Texture>& texture) -> std::uint32_t override;

  void release(std::uint32_t storageId);

  void setBindlessManager(graphics::BindlessTextureManager* manager);
  void setImguiRenderer(imgui::ImguiRenderer* renderer);

  void processGC();

 private:
  void remove(std::uint32_t storageId) override;

  graphics::BindlessTextureManager* bindlessManager_{nullptr};
  imgui::ImguiRenderer* imguiRenderer_{nullptr};

  std::unordered_map<std::uint32_t, std::uint32_t> refCounts_;

  struct GCTask {
    std::shared_ptr<Texture> texture;
    int framesRemaining;
  };

  std::vector<GCTask> gcQueue_;
  std::uint32_t maxFramesInFlight_{3};
};

};  // namespace vkit::texture

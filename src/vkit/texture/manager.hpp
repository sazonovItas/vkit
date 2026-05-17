#pragma once

#include <memory>

#include "vkit/item/storage.hpp"
#include "vkit/texture/texture.hpp"

namespace vkit::graphics {
class BindlessTextureManager;
};

namespace vkit::imgui {
class ImguiRenderer;
};

namespace vkit::texture {

class TextureManager : public vkit::Storage<Texture> {
 public:
  explicit TextureManager(std::uint32_t maxFramesInFlight = 3)
      : maxFramesInFlight_{maxFramesInFlight} {}

  TextureManager(TextureManager&) = delete;
  auto operator=(TextureManager&) -> TextureManager& = delete;

  ~TextureManager() override;

  auto add(const std::shared_ptr<Texture>& texture) -> std::uint32_t override;
  void remove(std::uint32_t storageId) override;

  [[nodiscard]] auto getBindlessManager() const
      -> graphics::BindlessTextureManager* {
    return bindlessManager_;
  }
  void setBindlessManager(graphics::BindlessTextureManager* manager);

  [[nodiscard]] auto getImguiRenderer() const -> imgui::ImguiRenderer* {
    return imguiRenderer_;
  }
  void setImguiRenderer(imgui::ImguiRenderer* renderer);

  void processGC();
  void forceFlushGC();

 private:
  graphics::BindlessTextureManager* bindlessManager_{nullptr};
  imgui::ImguiRenderer* imguiRenderer_{nullptr};

  struct GCTask {
    std::shared_ptr<Texture> texture;
    int framesRemaining;
  };

  std::vector<GCTask> gcQueue_;
  std::uint32_t maxFramesInFlight_{3};
};

};  // namespace vkit::texture

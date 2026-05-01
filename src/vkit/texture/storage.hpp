#pragma once

#include <memory>

#include "vkit/item/storage.hpp"
#include "vkit/texture/texture.hpp"

namespace vkit::graphics {
class BindlessTextureManager;
}
namespace vkit::imgui {
class ImguiRenderer;
}

namespace vkit::texture {

class TextureStorage : public vkit::Storage<Texture> {
 public:
  TextureStorage() = default;
  ~TextureStorage() override;

  TextureStorage(const TextureStorage&) = delete;
  auto operator=(const TextureStorage&) -> TextureStorage& = delete;

  auto add(const std::shared_ptr<Texture>& texture) -> std::uint32_t override;
  void remove(std::uint32_t storageId) override;

  void setBindlessManager(graphics::BindlessTextureManager* manager);
  void setImguiRenderer(imgui::ImguiRenderer* renderer);

 private:
  graphics::BindlessTextureManager* bindlessManager_{nullptr};
  imgui::ImguiRenderer* imguiRenderer_{nullptr};
};

};  // namespace vkit::texture

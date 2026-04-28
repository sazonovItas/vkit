#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "vkit/graphics/bindless_texture_manager.hpp"
#include "vkit/imgui/imgui_renderer.hpp"
#include "vkit/texture/texture.hpp"

namespace vkit::texture {

class Storage {
 public:
  Storage() = default;
  ~Storage();

  Storage(const Storage&) = delete;
  auto operator=(const Storage&) -> Storage& = delete;

  auto add(const std::shared_ptr<Texture>& texture) -> std::uint32_t;
  void remove(const std::shared_ptr<Texture>& texture);

  void setBindlessManager(graphics::BindlessTextureManager* manager);
  void setImguiRenderer(imgui::ImguiRenderer* renderer);

  [[nodiscard]] auto get(std::uint32_t storageId) const
      -> std::shared_ptr<Texture>;
  [[nodiscard]] auto getTextures() const
      -> std::vector<std::shared_ptr<Texture>>;
  [[nodiscard]] auto getActiveCount() const -> std::size_t;

 private:
  mutable std::mutex mutex_;
  std::vector<std::shared_ptr<Texture>> textures_;
  std::vector<std::uint32_t> freeIds_;
  std::size_t activeCount_{0};

  graphics::BindlessTextureManager* bindlessManager_{nullptr};
  imgui::ImguiRenderer* imguiRenderer_{nullptr};
};

}  // namespace vkit::texture

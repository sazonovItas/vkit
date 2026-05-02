#include "vkit/texture/manager.hpp"

#include "vkit/graphics/bindless_texture_manager.hpp"
#include "vkit/imgui/imgui_renderer.hpp"
#include "vkit/item/storage_item.hpp"

namespace vkit::texture {

TextureManager::~TextureManager() {
  const auto lock = std::scoped_lock{this->mutex_};

  for (auto& texture : this->items_) {
    if (texture) {
      if ((bindlessManager_ != nullptr) &&
          texture->getBindlessId().has_value()) {
        bindlessManager_->removeTexture(texture->getBindlessId().value());
        texture->setBindlessId(std::nullopt);
      }

      if ((imguiRenderer_ != nullptr) && texture->getImguiId().has_value()) {
        imguiRenderer_->unregisterTexture(texture->getImguiId().value());
        texture->setImguiId(std::nullopt);
      }
    }
  }
}

auto TextureManager::add(const std::shared_ptr<Texture>& texture)
    -> std::uint32_t {
  const std::uint32_t id = Storage<Texture>::add(texture);

  if (id != kStorageItemInvalidId && texture) {
    const auto lock = std::scoped_lock{this->mutex_};

    const auto graphics_tex = texture->getGraphicsTexture();
    if (graphics_tex) {
      if (bindlessManager_) {
        const auto bindless_id = bindlessManager_->addTexture(graphics_tex);
        texture->setBindlessId(bindless_id);
      }
      if (imguiRenderer_) {
        const auto imgui_id = imguiRenderer_->registerTexture(
            graphics_tex->getImageView(), graphics_tex->getSampler());
        texture->setImguiId(imgui_id);
      }
    }
  }

  return id;
}

void TextureManager::remove(std::uint32_t id) {
  {
    const auto lock = std::scoped_lock{this->mutex_};

    if (id < this->items_.size() && this->items_[id]) {
      auto texture = this->items_[id];

      if ((bindlessManager_ != nullptr) &&
          texture->getBindlessId().has_value()) {
        bindlessManager_->removeTexture(texture->getBindlessId().value());
        texture->setBindlessId(std::nullopt);
      }

      if ((imguiRenderer_ != nullptr) && texture->getImguiId().has_value()) {
        imguiRenderer_->unregisterTexture(texture->getImguiId().value());
        texture->setImguiId(std::nullopt);
      }

      gcQueue_.push_back({
          .texture = texture,
          .framesRemaining = static_cast<int>(maxFramesInFlight_) + 1,
      });
    }
  }

  Storage<Texture>::remove(id);
}

void TextureManager::processGC() {
  const auto lock = std::scoped_lock{this->mutex_};

  for (auto it = gcQueue_.begin(); it != gcQueue_.end();) {
    it->framesRemaining--;

    if (it->framesRemaining <= 0) {
      it = gcQueue_.erase(it);
    } else {
      ++it;
    }
  }
}

void TextureManager::setBindlessManager(
    graphics::BindlessTextureManager* manager) {
  const auto lock = std::scoped_lock{this->mutex_};
  bindlessManager_ = manager;

  if (bindlessManager_) {
    for (auto& tex : this->items_) {
      if (tex && !tex->getBindlessId().has_value() &&
          tex->getGraphicsTexture()) {
        const auto bindless_id =
            bindlessManager_->addTexture(tex->getGraphicsTexture());
        tex->setBindlessId(bindless_id);
      }
    }
  }
}

void TextureManager::setImguiRenderer(imgui::ImguiRenderer* renderer) {
  const auto lock = std::scoped_lock{this->mutex_};
  imguiRenderer_ = renderer;

  if (imguiRenderer_) {
    for (auto& tex : this->items_) {
      if (tex && !tex->getImguiId().has_value() && tex->getGraphicsTexture()) {
        const auto graphics_tex = tex->getGraphicsTexture();
        const auto imgui_id = imguiRenderer_->registerTexture(
            graphics_tex->getImageView(), graphics_tex->getSampler());
        tex->setImguiId(imgui_id);
      }
    }
  }
}

};  // namespace vkit::texture

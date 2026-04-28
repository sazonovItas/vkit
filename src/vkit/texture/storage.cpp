#include "storage.hpp"

#include "vkit/graphics/bindless_texture_manager.hpp"
#include "vkit/imgui/imgui_renderer.hpp"

namespace vkit::texture {

auto Storage::add(const std::shared_ptr<Texture>& texture) -> std::uint32_t {
  if (!texture) return 0;

  const auto lock = std::scoped_lock{mutex_};
  auto storage_id = std::uint32_t{0};

  if (!freeIds_.empty()) {
    storage_id = freeIds_.back();
    freeIds_.pop_back();
    textures_[storage_id] = texture;
  } else {
    storage_id = static_cast<std::uint32_t>(textures_.size());
    textures_.push_back(texture);
  }

  texture->setStorageId(storage_id);
  activeCount_++;

  const auto graphics_tex = texture->getGraphicsTexture();
  if (graphics_tex) {
    if (bindlessManager_) {
      const auto bindless_id = bindlessManager_->addTexture(graphics_tex);
      texture->setBindlessId(bindless_id);
    }
    if (imguiRenderer_) {
      const auto imgui_id = imguiRenderer_->registerTexture(
          graphics_tex->getView(), graphics_tex->getSampler());
      texture->setImguiId(imgui_id);
    }
  }

  return storage_id;
}

void Storage::remove(const std::shared_ptr<Texture>& texture) {
  if (!texture || !texture->getStorageId().has_value()) return;

  const auto lock = std::scoped_lock{mutex_};
  const auto storage_id = texture->getStorageId().value();

  if (storage_id < textures_.size() && textures_[storage_id] == texture) {
    if ((bindlessManager_ != nullptr) && texture->getBindlessId().has_value()) {
      bindlessManager_->removeTexture(texture->getBindlessId().value());
      texture->setBindlessId(std::nullopt);
    }
    if ((imguiRenderer_ != nullptr) && texture->getImguiId().has_value()) {
      imguiRenderer_->unregisterTexture(texture->getImguiId().value());
      texture->setImguiId(std::nullopt);
    }

    textures_[storage_id] = nullptr;
    freeIds_.push_back(storage_id);
    texture->setStorageId(std::nullopt);
    activeCount_--;
  }
}

void Storage::setBindlessManager(graphics::BindlessTextureManager* manager) {
  const auto lock = std::scoped_lock{mutex_};
  bindlessManager_ = manager;

  if (bindlessManager_) {
    for (auto& tex : textures_) {
      if (tex && !tex->getBindlessId().has_value() &&
          tex->getGraphicsTexture()) {
        const auto bindless_id =
            bindlessManager_->addTexture(tex->getGraphicsTexture());
        tex->setBindlessId(bindless_id);
      }
    }
  }
}

void Storage::setImguiRenderer(imgui::ImguiRenderer* renderer) {
  const auto lock = std::scoped_lock{mutex_};
  imguiRenderer_ = renderer;

  if (imguiRenderer_) {
    for (auto& tex : textures_) {
      if (tex && !tex->getImguiId().has_value() && tex->getGraphicsTexture()) {
        const auto graphics_tex = tex->getGraphicsTexture();
        const auto imgui_id = imguiRenderer_->registerTexture(
            graphics_tex->getView(), graphics_tex->getSampler());
        tex->setImguiId(imgui_id);
      }
    }
  }
}

auto Storage::get(std::uint32_t storageId) const -> std::shared_ptr<Texture> {
  const auto lock = std::scoped_lock{mutex_};

  if (storageId < textures_.size()) {
    return textures_[storageId];
  }
  return nullptr;
}

auto Storage::getTextures() const -> std::vector<std::shared_ptr<Texture>> {
  const auto lock = std::scoped_lock{mutex_};

  auto result = std::vector<std::shared_ptr<Texture>>{};
  result.reserve(activeCount_);

  for (auto const& tex : textures_) {
    if (tex) {
      result.push_back(tex);
    }
  }
  return result;
}

auto Storage::getActiveCount() const -> std::size_t {
  const auto lock = std::scoped_lock{mutex_};
  return activeCount_;
}

Storage::~Storage() {
  const auto lock = std::scoped_lock{mutex_};

  for (auto& texture : textures_) {
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

  textures_.clear();
  freeIds_.clear();
  activeCount_ = 0;
}

};  // namespace vkit::texture

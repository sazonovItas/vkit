#pragma once

#include <imgui.h>

#include <memory>
#include <optional>
#include <string_view>

#include "vkit/graphics/texture.hpp"
#include "vkit/item/item.hpp"

namespace vkit::texture {

class Texture : public Item<Texture> {
 public:
  explicit Texture(std::string_view name,
                   const std::shared_ptr<vkit::graphics::Texture>& texture);
  ~Texture() override = default;

  [[nodiscard]] auto getStorageId() const -> std::optional<std::uint32_t>;
  [[nodiscard]] auto getBindlessId() const -> std::optional<std::uint32_t>;
  [[nodiscard]] auto getImguiId() const -> std::optional<ImTextureID>;

  [[nodiscard]] auto getGraphicsTexture() const
      -> std::shared_ptr<vkit::graphics::Texture>;

  void setStorageId(std::optional<std::uint32_t> id);
  void setBindlessId(std::optional<std::uint32_t> id);
  void setImguiId(std::optional<ImTextureID> id);

 private:
  std::shared_ptr<vkit::graphics::Texture> texture_;
  std::optional<std::uint32_t> storageId_;
  std::optional<std::uint32_t> bindlessId_;
  std::optional<ImTextureID> imguiId_;
};

}  // namespace vkit::texture

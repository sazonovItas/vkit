#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "vkit/item/item.hpp"
#include "vkit/item/storage_item.hpp"
#include "vkit/primitive/primitive_attachment.hpp"

namespace vkit::material {

enum class Type : std::uint32_t {
  kNone = 0,
  kDiffuse = 1,
  kDiffuseSpecular = 2,
  kPrincipledBSDF = 3,
};

enum class AlphaMode : std::uint32_t { kOpaque = 0, kMask = 1, kBlend = 2 };

class Material : public Item<Material>,
                 public StorageItem,
                 public primitive::Attachment {
 public:
  explicit Material(std::string_view name) : Item{name} {}

  [[nodiscard]] virtual auto getType() const -> Type = 0;

  [[nodiscard]] auto isDirty() const -> bool { return isDirty_; };
  void setDirty(const bool dirty) { isDirty_ = dirty; }

  [[nodiscard]] auto getStorageId() const -> std::optional<std::uint32_t> {
    return storageId_;
  };
  void setStorageId(const std::optional<std::uint32_t> id) { storageId_ = id; }

  [[nodiscard]] auto getAlphaMode() const -> AlphaMode { return alphaMode_; }
  void setAlphaMode(AlphaMode mode) { alphaMode_ = mode; }

  [[nodiscard]] auto isDoubleSided() const -> bool { return doubleSided_; }
  void setDoubleSided(bool doubleSided) { doubleSided_ = doubleSided; }

  ~Material() override;

 private:
  bool isDirty_{true};
  std::optional<std::uint32_t> storageId_;

  AlphaMode alphaMode_{AlphaMode::kOpaque};
  bool doubleSided_{false};
};

};  // namespace vkit::material

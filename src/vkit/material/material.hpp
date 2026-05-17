#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "vkit/item/item.hpp"
#include "vkit/item/storage_item.hpp"

namespace vkit::material {

static constexpr std::uint32_t kItemInvalidId = kStorageItemInvalidId;

enum class Type : std::uint32_t {
  kNone = 0,
  kDiffuse = 1,
  kDiffuseSpecular = 2,
  kPrincipledBSDF = 3,
  kMix = 4,
};

enum class AlphaMode : std::uint32_t { kOpaque = 0, kMask = 1, kBlend = 2 };

class Material : public Item<Material>, public StorageItem {
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

  ~Material() override = default;

 private:
  bool isDirty_{true};
  std::optional<std::uint32_t> storageId_;

  AlphaMode alphaMode_{AlphaMode::kOpaque};
  bool doubleSided_{false};
};

};  // namespace vkit::material

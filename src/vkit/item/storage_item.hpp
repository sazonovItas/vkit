#pragma once

#include <cstdint>
#include <limits>
#include <optional>

namespace vkit {

static constexpr std::uint32_t kStorageItemInvalidId =
    std::numeric_limits<std::uint32_t>::max();

class StorageItem {
 public:
  StorageItem() = default;
  virtual ~StorageItem() = default;

  [[nodiscard]] auto getStorageId() const -> std::optional<std::uint32_t> {
    return storageId_;
  }

  void setStorageId(std::optional<std::uint32_t> id) { storageId_ = id; }

  [[nodiscard]] auto hasStorageId() const -> bool {
    return storageId_.has_value();
  }

 private:
  std::optional<std::uint32_t> storageId_{std::nullopt};
};

};  // namespace vkit

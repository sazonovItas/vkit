#pragma once

#include <concepts>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "vkit/item/storage_item.hpp"

namespace vkit {

template <typename T>
  requires std::derived_from<T, StorageItem>
class Storage {
 public:
  Storage() = default;
  virtual ~Storage() = default;

  Storage(const Storage&) = delete;
  auto operator=(const Storage&) -> Storage& = delete;

  virtual auto add(const std::shared_ptr<T>& item) -> std::uint32_t {
    if (!item) return kStorageItemInvalidId;

    std::lock_guard<std::mutex> lock{mutex_};

    std::uint32_t id;
    if (!freeIds_.empty()) {
      id = freeIds_.back();
      freeIds_.pop_back();
      items_[id] = item;
    } else {
      id = static_cast<std::uint32_t>(items_.size());
      items_.push_back(item);
    }

    item->setStorageId(id);

    activeCount_++;
    return id;
  }

  virtual void remove(std::uint32_t storageId) {
    std::lock_guard<std::mutex> lock{mutex_};

    if (storageId < items_.size() && items_[storageId] != nullptr) {
      items_[storageId]->setStorageId(std::nullopt);
      items_[storageId] = nullptr;
      freeIds_.push_back(storageId);
      activeCount_--;
    }
  }

  void clear() {
    std::lock_guard<std::mutex> lock{mutex_};
    for (auto& item : items_) {
      if (item) {
        item->setStorageId(std::nullopt);
        item = nullptr;
      }
    }
    items_.clear();
    freeIds_.clear();
    activeCount_ = 0;
  }

  [[nodiscard]] auto get(std::uint32_t storageId) const -> std::shared_ptr<T> {
    std::lock_guard<std::mutex> lock{mutex_};
    if (storageId < items_.size()) {
      return items_[storageId];
    }
    return nullptr;
  }

  [[nodiscard]] auto getItems() const -> std::vector<std::shared_ptr<T>> {
    std::lock_guard<std::mutex> lock{mutex_};
    std::vector<std::shared_ptr<T>> active_items;
    active_items.reserve(activeCount_);

    for (const auto& item : items_) {
      if (item) {
        active_items.push_back(item);
      }
    }
    return active_items;
  }

  [[nodiscard]] auto getActiveCount() const -> std::size_t {
    std::lock_guard<std::mutex> lock{mutex_};
    return activeCount_;
  }

 protected:
  mutable std::mutex mutex_;
  std::vector<std::shared_ptr<T>> items_;
  std::vector<std::uint32_t> freeIds_;
  std::size_t activeCount_{0};
};

};  // namespace vkit

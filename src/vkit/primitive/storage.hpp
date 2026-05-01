#pragma once

#include <cstddef>
#include <mutex>
#include <span>
#include <vector>

#include "vkit/item/storage.hpp"
#include "vkit/primitive/primitive.hpp"

namespace vkit::primitive {

class PrimitiveStorage : public Storage<Primitive> {
 public:
  PrimitiveStorage() = default;

  void update() {
    std::lock_guard<std::mutex> lock{this->mutex_};

    if (data_.size() < this->items_.size()) {
      data_.resize(this->items_.size());
    }

    for (std::size_t i = 0; i < this->items_.size(); ++i) {
      if (this->items_[i]) {
        data_[i] = this->items_[i]->getData();
      }
    }
  }

  [[nodiscard]] auto getData() const -> std::span<const Primitive::Data> {
    std::lock_guard<std::mutex> lock{this->mutex_};
    return data_;
  }

  [[nodiscard]] auto getBytes() const -> std::span<const std::byte> {
    std::lock_guard<std::mutex> lock{this->mutex_};
    return std::as_bytes(std::span{data_});
  }

 private:
  std::vector<Primitive::Data> data_;
};

}  // namespace vkit::primitive

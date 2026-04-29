#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

#include "vkit/primitive/primitive.hpp"

namespace vkit::primitive {

class Storage {
 public:
  Storage() = default;

  Storage(const Storage&) = delete;
  auto operator=(const Storage&) -> Storage& = delete;

  auto add(const std::shared_ptr<Primitive>& prim) -> std::uint32_t;
  void remove(std::uint32_t index);

  void update();

  [[nodiscard]] auto getPrimitives() const
      -> std::vector<std::shared_ptr<Primitive>>;

  [[nodiscard]] auto getPrimitive(std::uint32_t index) const
      -> std::shared_ptr<Primitive>;

  [[nodiscard]] auto getData() const -> std::span<const std::byte>;

 private:
  mutable std::mutex mutex_;
  std::vector<std::shared_ptr<Primitive>> primitives_;
  std::vector<Primitive::Data> primitiveData_;
  std::vector<std::uint32_t> freeList_;
};

};  // namespace vkit::primitive

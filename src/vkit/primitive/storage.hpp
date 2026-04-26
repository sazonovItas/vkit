#pragma once

#include <memory>

#include "vkit/primitive/primitive.hpp"

namespace vkit::primitive {

class Storage {
 public:
  Storage() = default;

  auto add(const std::shared_ptr<Primitive>& prim) -> std::uint32_t;
  void remove(std::uint32_t index);

  void update();

  [[nodiscard]] auto getPrimitives() const
      -> const std::vector<std::shared_ptr<Primitive>>&;

  [[nodiscard]] auto getData() const -> std::span<const std::byte>;

 private:
  std::vector<std::shared_ptr<Primitive>> primitives_;
  std::vector<Primitive::Data> primitiveData_;
  std::vector<std::uint32_t> freeList_;
};

};  // namespace vkit::primitive

#pragma once

#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "vkit/item/item.hpp"
#include "vkit/primitive/primitive.hpp"
#include "vkit/scene/node_attachment.hpp"

namespace vkit::scene {

class Mesh : public Item<Mesh>, public NodeAttachment {
 public:
  explicit Mesh(
      std::string_view name = "Mesh",
      std::span<const std::shared_ptr<primitive::Primitive>> primitives = {})
      : Item(name), primitives_{primitives.begin(), primitives.end()} {}

  [[nodiscard]] auto getPrimitives() const
      -> std::span<const std::shared_ptr<primitive::Primitive>> {
    return primitives_;
  }

  void addPrimitive(std::shared_ptr<primitive::Primitive> primitive) {
    if (primitive) {
      primitives_.push_back(std::move(primitive));
    }
  }

  void removePrimitive(const std::shared_ptr<primitive::Primitive>& primitive) {
    if (primitive) {
      std::erase(primitives_, primitive);
    }
  }

 private:
  std::vector<std::shared_ptr<primitive::Primitive>> primitives_;
};

};  // namespace vkit::scene

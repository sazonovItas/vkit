#pragma once

#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "vkit/item/item.hpp"
#include "vkit/item/storage_item.hpp"
#include "vkit/primitive/primitive.hpp"

namespace vkit::scene {

class Mesh : public Item<Mesh>, public StorageItem {
 public:
  bool visible{true};

  // Local-space AABB (populated from glTF position accessor bounds).
  glm::vec3 aabbMin{1e30F};
  glm::vec3 aabbMax{-1e30F};

  [[nodiscard]] auto aabbCenter() const -> glm::vec3 {
    return (aabbMin + aabbMax) * 0.5F;
  }
  [[nodiscard]] auto aabbValid() const -> bool { return aabbMin.x <= aabbMax.x; }

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

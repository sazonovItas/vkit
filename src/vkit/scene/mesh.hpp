#pragma once

#include <vector>

#include "vkit/item/item.hpp"
#include "vkit/primitive/primitive.hpp"
#include "vkit/scene/node_attachment.hpp"

namespace vkit::scene {

class Mesh : public Item<Mesh>, public NodeAttachment {
 public:
  explicit Mesh(std::string_view name = "Mesh",
                const std::vector<vkit::primitive::Primitive>& primitives = {})
      : Item(name), primitives_{std::move(primitives)} {}

  [[nodiscard]] auto getPrimitives() const
      -> const std::vector<vkit::primitive::Primitive>& {
    return primitives_;
  }
  [[nodiscard]] auto getPrimitives()
      -> std::vector<vkit::primitive::Primitive>& {
    return primitives_;
  }

  void addPrimitive(const vkit::primitive::Primitive& primitive) {
    primitives_.push_back(std::move(primitive));
  }

 private:
  std::vector<vkit::primitive::Primitive> primitives_;
};

};  // namespace vkit::scene

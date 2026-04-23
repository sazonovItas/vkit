#pragma once

#include <memory>
#include <vector>

#include "vkit/primitive/primitive.hpp"
#include "vkit/scene/material.hpp"
#include "vkit/scene/node_attachment.hpp"

namespace vkit::scene {

struct MeshPrimitive {
  std::shared_ptr<primitive::Primitive> geometry;
  std::shared_ptr<MaterialBsdf> material;
};

class Mesh : public NodeAttachment {
 public:
  explicit Mesh(std::string_view name = "Mesh",
                std::vector<MeshPrimitive> primitives = {})
      : NodeAttachment(name), primitives_{std::move(primitives)} {}

  [[nodiscard]] auto getType() const -> AttachmentType override {
    return AttachmentType::kMesh;
  }

  [[nodiscard]] auto getPrimitives() const
      -> const std::vector<MeshPrimitive>& {
    return primitives_;
  }
  [[nodiscard]] auto getPrimitives() -> std::vector<MeshPrimitive>& {
    return primitives_;
  }

  void addPrimitive(MeshPrimitive primitive) {
    primitives_.push_back(std::move(primitive));
  }

 private:
  std::vector<MeshPrimitive> primitives_;
};

};  // namespace vkit::scene

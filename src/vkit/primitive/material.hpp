#pragma once

#include "vkit/item/item.hpp"
#include "vkit/primitive/primitive_attachment.hpp"

namespace vkit::primitive {

class Material : public Item<Material>, public PrimitiveAttachment {
 public:
  explicit Material(std::string_view name) : Item{name} {}
};

};  // namespace vkit::primitive

#pragma once

namespace vkit::primitive {

class Primitive;

class Attachment {
 public:
  void setPrimitive(Primitive* node) { primitive_ = node; }
  [[nodiscard]] auto getPrimitive() const -> Primitive* { return primitive_; }

 protected:
  Primitive* primitive_{nullptr};
};

};  // namespace vkit::primitive

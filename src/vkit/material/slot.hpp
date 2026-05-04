#pragma once

#include <cstdint>

#include "vkit/item/storage_item.hpp"
#include "vkit/material/material.hpp"

namespace vkit::material {

class Slot : public StorageItem {
 public:
  Slot() = default;
  Slot(Type type, std::uint32_t id) : type_{type}, id_{id} {}

  [[nodiscard]] auto getMaterialType() const -> Type { return type_; }
  void setMaterialType(Type type) { type_ = type; }

  [[nodiscard]] auto getMaterialId() const -> std::uint32_t { return id_; }
  void setMaterialId(std::uint32_t id) { id_ = id; }

 private:
  Type type_{Type::kNone};
  std::uint32_t id_{0};
};

};  // namespace vkit::material

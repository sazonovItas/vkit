#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include "vkit/item/item.hpp"
#include "vkit/item/storage_item.hpp"
#include "vkit/material/material.hpp"

namespace vkit::material {

class Slot : public Item<Slot>, public StorageItem {
 public:
  explicit Slot(std::string_view name) : Item{name} {}
  ~Slot() override = default;

  [[nodiscard]] auto getMaterial() const -> std::shared_ptr<Material> {
    return material_;
  }

  void setMaterial(std::shared_ptr<Material> material) {
    material_ = std::move(material);
  }

  [[nodiscard]] auto getMaterialStorageId() const
      -> std::optional<std::uint32_t> {
    return material_ ? material_->getStorageId() : std::nullopt;
  }

 private:
  std::shared_ptr<Material> material_{nullptr};
};

};  // namespace vkit::material

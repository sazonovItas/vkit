#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

#include "vkit/item/storage.hpp"
#include "vkit/material/diffuse.hpp"
#include "vkit/material/diffuse_specular.hpp"
#include "vkit/material/principled_bsdf.hpp"
#include "vkit/material/slog.hpp"

namespace vkit::material {

template <typename T>
class TypedMaterialStorage : public vkit::Storage<T> {
 public:
  TypedMaterialStorage() = default;

  void update() {
    std::lock_guard<std::mutex> lock{this->mutex_};

    if (data_.size() < this->items_.size()) {
      data_.resize(this->items_.size());
    }

    for (std::size_t i = 0; i < this->items_.size(); ++i) {
      if (this->items_[i] && this->items_[i]->isDirty()) {
        data_[i] = this->items_[i]->getData();
        this->items_[i]->setDirty(false);
      }
    }
  }

  [[nodiscard]] auto getData() const -> std::span<const typename T::Data> {
    std::lock_guard<std::mutex> lock{this->mutex_};
    return data_;
  }

 private:
  std::vector<typename T::Data> data_;
};

class MaterialManager {
 public:
  MaterialManager() = default;
  ~MaterialManager() = default;

  MaterialManager(const MaterialManager&) = delete;
  auto operator=(const MaterialManager&) -> MaterialManager& = delete;

  TypedMaterialStorage<Diffuse> diffuse;
  TypedMaterialStorage<DiffuseSpecular> diffuseSpecular;
  TypedMaterialStorage<PrincipledBSDF> principledBSDF;

  vkit::Storage<Slot> slots;

  void update();
  void clear();

  auto addMaterial(const std::shared_ptr<Material>& mat) -> std::uint32_t;
  void removeMaterial(const std::shared_ptr<Material>& mat);
  void removeMaterial(Type type, std::uint32_t id);

  [[nodiscard]] auto getMaterial(Type type, std::uint32_t id) const
      -> std::shared_ptr<Material>;
  [[nodiscard]] auto getAllMaterials() const
      -> std::vector<std::shared_ptr<Material>>;

  auto addSlot(const std::shared_ptr<Slot>& slot) -> std::uint32_t {
    return slots.add(slot);
  }

  void removeSlot(std::uint32_t id) { slots.remove(id); }

  [[nodiscard]] auto getSlot(std::uint32_t id) const -> std::shared_ptr<Slot> {
    return slots.get(id);
  }

  [[nodiscard]] auto getSlots() const -> std::vector<std::shared_ptr<Slot>> {
    return slots.getItems();
  }
};

};  // namespace vkit::material

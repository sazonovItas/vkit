#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <vector>

#include "vkit/item/item.hpp"
#include "vkit/item/storage.hpp"
#include "vkit/material/diffuse.hpp"
#include "vkit/material/diffuse_specular.hpp"
#include "vkit/material/principled_bsdf.hpp"

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

class MaterialStorage {
 public:
  MaterialStorage() = default;

  TypedMaterialStorage<Diffuse> diffuse;
  TypedMaterialStorage<DiffuseSpecular> diffuseSpecular;
  TypedMaterialStorage<PrincipledBSDF> principledBSDF;

  void update() {
    diffuse.update();
    diffuseSpecular.update();
    principledBSDF.update();
  }

  auto add(const std::shared_ptr<Material>& mat) -> std::uint32_t {
    if (!mat) return kItemInvalidId;

    switch (mat->getType()) {
      case Type::kDiffuse:
        return diffuse.add(std::static_pointer_cast<Diffuse>(mat));
      case Type::kDiffuseSpecular:
        return diffuseSpecular.add(
            std::static_pointer_cast<DiffuseSpecular>(mat));
      case Type::kPrincipledBSDF:
        return principledBSDF.add(
            std::static_pointer_cast<PrincipledBSDF>(mat));
      default:
        throw std::runtime_error{
            "Unknown material type passed to MaterialStorage::add"};
    }
  }

  void remove(const std::shared_ptr<Material>& mat) {
    if (!mat || !mat->hasStorageId()) return;

    const std::uint32_t id = mat->getStorageId().value();

    switch (mat->getType()) {
      case Type::kDiffuse:
        diffuse.remove(id);
        break;
      case Type::kDiffuseSpecular:
        diffuseSpecular.remove(id);
        break;
      case Type::kPrincipledBSDF:
        principledBSDF.remove(id);
        break;
      default:
        throw std::runtime_error{
            "Unknown material type passed to MaterialStorage::remove"};
    }
  }
};

};  // namespace vkit::material

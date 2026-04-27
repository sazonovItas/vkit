#include "vkit/material/storage.hpp"

#include <stdexcept>

namespace vkit::material {

auto Storage::add(const std::shared_ptr<Material>& mat) -> std::uint32_t {
  if (!mat) {
    throw std::invalid_argument{"Cannot add a null material"};
  }

  std::uint32_t index = 0;
  Type type = mat->getType();

  if (type == Type::kDiffuse) {
    auto specific_mat = std::static_pointer_cast<Diffuse>(mat);

    if (!diffuseFreeList_.empty()) {
      index = diffuseFreeList_.back();
      diffuseFreeList_.pop_back();
      diffuse_[index] = specific_mat;
    } else {
      index = static_cast<std::uint32_t>(diffuse_.size());
      diffuse_.push_back(specific_mat);
      diffuseData_.emplace_back();
    }
  } else if (type == Type::kDiffuseSpecular) {
    auto specific_mat = std::static_pointer_cast<DiffuseSpecular>(mat);

    if (!diffuseSpecularFreeList_.empty()) {
      index = diffuseSpecularFreeList_.back();
      diffuseSpecularFreeList_.pop_back();
      diffuseSpecular_[index] = specific_mat;
    } else {
      index = static_cast<std::uint32_t>(diffuseSpecular_.size());
      diffuseSpecular_.push_back(specific_mat);
      diffuseSpecularData_.emplace_back();
    }
  } else if (type == Type::kPrincipledBSDF) {
    auto specific_mat = std::static_pointer_cast<PrincipledBSDF>(mat);

    if (!principledBSDFFreeList_.empty()) {
      index = principledBSDFFreeList_.back();
      principledBSDFFreeList_.pop_back();
      principledBSDF_[index] = specific_mat;
    } else {
      index = static_cast<std::uint32_t>(principledBSDF_.size());
      principledBSDF_.push_back(specific_mat);
      principledBSDFData_.emplace_back();
    }
  } else {
    throw std::runtime_error{"Unknown material type passed to Storage::add"};
  }

  mat->setStorageId(index);
  mat->setDirty(true);

  materialMap_[mat->getId()] = mat;

  return index;
}

void Storage::remove(std::size_t id) {
  auto it = materialMap_.find(id);
  if (it != materialMap_.end() && it->second->getStorageId().has_value()) {
    removeByType(it->second->getType(), it->second->getStorageId().value());
  }
}

auto Storage::getMaterial(std::size_t id) const -> std::shared_ptr<Material> {
  auto it = materialMap_.find(id);
  return it != materialMap_.end() ? it->second : nullptr;
}

void Storage::update() {
  for (std::size_t i = 0; i < diffuse_.size(); ++i) {
    if (diffuse_[i] && diffuse_[i]->isDirty()) {
      diffuseData_[i] = diffuse_[i]->data;
      diffuse_[i]->setDirty(false);
    }
  }

  for (std::size_t i = 0; i < diffuseSpecular_.size(); ++i) {
    if (diffuseSpecular_[i] && diffuseSpecular_[i]->isDirty()) {
      diffuseSpecularData_[i] = diffuseSpecular_[i]->data;
      diffuseSpecular_[i]->setDirty(false);
    }
  }

  for (std::size_t i = 0; i < principledBSDF_.size(); ++i) {
    if (principledBSDF_[i] && principledBSDF_[i]->isDirty()) {
      principledBSDFData_[i] = principledBSDF_[i]->data;
      principledBSDF_[i]->setDirty(false);
    }
  }
}

auto Storage::getData(Type type) const -> std::span<const std::byte> {
  switch (type) {
    case Type::kDiffuse:
      return std::as_bytes(std::span{diffuseData_});
    case Type::kDiffuseSpecular:
      return std::as_bytes(std::span{diffuseSpecularData_});
    case Type::kPrincipledBSDF:
      return std::as_bytes(std::span{principledBSDFData_});
    default:
      throw std::runtime_error{"Unknown material type to get data"};
  }
}

void Storage::removeByType(Type type, std::uint32_t index) {
  if (type == Type::kDiffuse && index < diffuse_.size() && diffuse_[index]) {
    materialMap_.erase(diffuse_[index]->getId());
    diffuse_[index]->setStorageId(std::nullopt);
    diffuse_[index] = nullptr;
    diffuseFreeList_.push_back(index);
  } else if (type == Type::kDiffuseSpecular &&
             index < diffuseSpecular_.size() && diffuseSpecular_[index]) {
    materialMap_.erase(diffuseSpecular_[index]->getId());
    diffuseSpecular_[index]->setStorageId(std::nullopt);
    diffuseSpecular_[index] = nullptr;
    diffuseSpecularFreeList_.push_back(index);
  } else if (type == Type::kPrincipledBSDF && index < principledBSDF_.size() &&
             principledBSDF_[index]) {
    materialMap_.erase(principledBSDF_[index]->getId());
    principledBSDF_[index]->setStorageId(std::nullopt);
    principledBSDF_[index] = nullptr;
    principledBSDFFreeList_.push_back(index);
  }
}

};  // namespace vkit::material

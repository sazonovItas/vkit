#include "vkit/material/storage.hpp"

#include <stdexcept>

#include "vkit/util/util.hpp"

namespace vkit::material {

auto Storage::add(const std::shared_ptr<Diffuse>& mat) -> std::uint32_t {
  std::uint32_t index = 0;

  if (!diffuseFreeList_.empty()) {
    index = diffuseFreeList_.back();
    diffuseFreeList_.pop_back();
    diffuse_[index] = mat;
  } else {
    index = static_cast<std::uint32_t>(diffuse_.size());
    diffuse_.push_back(mat);
    diffuseData_.emplace_back();
  }

  mat->setStorageId(index);
  mat->setDirty(true);
  return index;
}

auto Storage::add(const std::shared_ptr<DiffuseSpecular>& mat)
    -> std::uint32_t {
  std::uint32_t index = 0;

  if (!diffuseSpecularFreeList_.empty()) {
    index = diffuseSpecularFreeList_.back();
    diffuseSpecularFreeList_.pop_back();
    diffuseSpecular_[index] = mat;
  } else {
    index = static_cast<std::uint32_t>(diffuseSpecular_.size());
    diffuseSpecular_.push_back(mat);
    diffuseSpecularData_.emplace_back();
  }

  mat->setStorageId(index);
  mat->setDirty(true);
  return index;
}

auto Storage::add(const std::shared_ptr<PrincipledBSDF>& mat) -> std::uint32_t {
  std::uint32_t index = 0;

  if (!principledBSDFFreeList_.empty()) {
    index = principledBSDFFreeList_.back();
    principledBSDFFreeList_.pop_back();
    principledBSDF_[index] = mat;
  } else {
    index = static_cast<std::uint32_t>(principledBSDF_.size());
    principledBSDF_.push_back(mat);
    principledBSDFData_.emplace_back();
  }

  mat->setStorageId(index);
  mat->setDirty(true);
  return index;
}

void Storage::remove(Type type, std::uint32_t index) {
  if (type == Type::kDiffuse && index < diffuse_.size() && diffuse_[index]) {
    diffuse_[index]->setStorageId(std::nullopt);
    diffuse_[index] = nullptr;
    diffuseFreeList_.push_back(index);
  } else if (type == Type::kDiffuseSpecular &&
             index < diffuseSpecular_.size() && diffuseSpecular_[index]) {
    diffuseSpecular_[index]->setStorageId(std::nullopt);
    diffuseSpecular_[index] = nullptr;
    diffuseSpecularFreeList_.push_back(index);
  } else if (type == Type::kPrincipledBSDF && index < principledBSDF_.size() &&
             principledBSDF_[index]) {
    principledBSDF_[index]->setStorageId(std::nullopt);
    principledBSDF_[index] = nullptr;
    principledBSDFFreeList_.push_back(index);
  }
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
      return util::toByteSpan(diffuseSpecularData_);
    case Type::kPrincipledBSDF:
      return util::toByteSpan(principledBSDFData_);
    default:
      throw std::runtime_error{"Unkown material type"};
  }
}

};  // namespace vkit::material

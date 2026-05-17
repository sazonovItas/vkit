#include "vkit/material/manager.hpp"

namespace vkit::material {

void MaterialManager::update() {
  diffuse.update();
  diffuseSpecular.update();
  principledBSDF.update();
  mix.update();
}

void MaterialManager::clear() {
  diffuse.clear();
  diffuseSpecular.clear();
  principledBSDF.clear();
  mix.clear();
}

auto MaterialManager::addMaterial(const std::shared_ptr<Material>& mat)
    -> std::uint32_t {
  if (!mat) return kItemInvalidId;

  switch (mat->getType()) {
    case Type::kDiffuse:
      return diffuse.add(std::static_pointer_cast<Diffuse>(mat));
    case Type::kDiffuseSpecular:
      return diffuseSpecular.add(
          std::static_pointer_cast<DiffuseSpecular>(mat));
    case Type::kPrincipledBSDF:
      return principledBSDF.add(std::static_pointer_cast<PrincipledBSDF>(mat));
    case Type::kMix:
      return mix.add(std::static_pointer_cast<MixMaterial>(mat));
    default:
      throw std::runtime_error{
          "Unknown material type passed to MaterialManager"};
  }
}

void MaterialManager::removeMaterial(const std::shared_ptr<Material>& mat) {
  if (!mat || !mat->getStorageId().has_value()) return;
  removeMaterial(mat->getType(), mat->getStorageId().value());
}

void MaterialManager::removeMaterial(Type type, std::uint32_t id) {
  switch (type) {
    case Type::kDiffuse:
      diffuse.remove(id);
      break;
    case Type::kDiffuseSpecular:
      diffuseSpecular.remove(id);
      break;
    case Type::kPrincipledBSDF:
      principledBSDF.remove(id);
      break;
    case Type::kMix:
      mix.remove(id);
      break;
    default:
      break;
  }
}

auto MaterialManager::getMaterial(Type type, std::uint32_t id) const
    -> std::shared_ptr<Material> {
  switch (type) {
    case Type::kDiffuse:
      return diffuse.get(id);
    case Type::kDiffuseSpecular:
      return diffuseSpecular.get(id);
    case Type::kPrincipledBSDF:
      return principledBSDF.get(id);
    case Type::kMix:
      return mix.get(id);
    default:
      return nullptr;
  }
}

auto MaterialManager::getAllMaterials() const
    -> std::vector<std::shared_ptr<Material>> {
  std::vector<std::shared_ptr<Material>> all_mats;
  all_mats.reserve(diffuse.getActiveCount() + diffuseSpecular.getActiveCount() +
                   principledBSDF.getActiveCount() + mix.getActiveCount());

  for (const auto& m : diffuse.getItems()) all_mats.push_back(m);
  for (const auto& m : diffuseSpecular.getItems()) all_mats.push_back(m);
  for (const auto& m : principledBSDF.getItems()) all_mats.push_back(m);
  for (const auto& m : mix.getItems()) all_mats.push_back(m);

  return all_mats;
}

};  // namespace vkit::material

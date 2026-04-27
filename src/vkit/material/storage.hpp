#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include "vkit/material/diffuse.hpp"
#include "vkit/material/diffuse_specular.hpp"
#include "vkit/material/material.hpp"
#include "vkit/material/principled_bsdf.hpp"

namespace vkit::material {

class Storage {
 public:
  Storage() = default;

  auto add(const std::shared_ptr<Material>& mat) -> std::uint32_t;
  void remove(std::size_t id);

  [[nodiscard]] auto getMaterial(std::size_t id) const
      -> std::shared_ptr<Material>;

  template <typename T>
  [[nodiscard]] auto getMaterialAs(std::size_t id) const -> std::shared_ptr<T> {
    return std::dynamic_pointer_cast<T>(getMaterial(id));
  }

  void update();

  [[nodiscard]] auto getData(Type type) const -> std::span<const std::byte>;

 private:
  std::vector<std::shared_ptr<Diffuse>> diffuse_;
  std::vector<std::shared_ptr<DiffuseSpecular>> diffuseSpecular_;
  std::vector<std::shared_ptr<PrincipledBSDF>> principledBSDF_;

  std::vector<Diffuse::Data> diffuseData_;
  std::vector<DiffuseSpecular::Data> diffuseSpecularData_;
  std::vector<PrincipledBSDF::Data> principledBSDFData_;

  std::vector<std::uint32_t> diffuseFreeList_;
  std::vector<std::uint32_t> diffuseSpecularFreeList_;
  std::vector<std::uint32_t> principledBSDFFreeList_;

  std::unordered_map<std::size_t, std::shared_ptr<Material>> materialMap_;

  void removeByType(Type type, std::uint32_t index);
};

};  // namespace vkit::material

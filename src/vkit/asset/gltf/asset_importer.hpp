#pragma once

#include <fastgltf/core.hpp>
#include <filesystem>
#include <memory>
#include <vector>

#include "vkit/animation/animation.hpp"
#include "vkit/asset/asset.hpp"
#include "vkit/dataformat/vertex_format.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/primitive/buffers.hpp"

namespace vkit::asset::gltf {

class AssetImporter {
 public:
  explicit AssetImporter(const graphics::GfxDevice& gfxDevice);

  [[nodiscard]] auto load(const std::filesystem::path& filepath)
      -> std::shared_ptr<Asset>;

 private:
  const graphics::GfxDevice& gfxDevice_;

  std::shared_ptr<Asset> asset_;
  std::shared_ptr<primitive::DeviceBuffers> deviceBuffers_;

  std::vector<std::shared_ptr<scene::Mesh>> loadedMeshes_;
  std::vector<std::shared_ptr<scene::Node>> loadedNodes_;
  std::vector<std::shared_ptr<scene::Skin>> loadedSkins_;
  std::vector<std::shared_ptr<animation::Animation>> loadedAnimations_;

  void resetState();
  void loadBuffers(const fastgltf::Asset& asset,
                   const std::filesystem::path& directory);
  void loadMeshes(const fastgltf::Asset& asset);
  void loadNodes(const fastgltf::Asset& asset);
  void loadSkins(const fastgltf::Asset& asset);
  void loadAnimations(const fastgltf::Asset& asset);
  void loadScenes(const fastgltf::Asset& asset);

  [[nodiscard]] static auto mapAttributeFormat(fastgltf::ComponentType compType,
                                               fastgltf::AccessorType accType)
      -> dataformat::AttributeFormat;

  static void populateAttribute(primitive::Attribute& attr,
                                const fastgltf::Asset& asset,
                                std::size_t accessorIdx);
};

};  // namespace vkit::asset::gltf

#pragma once

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vk_mem_alloc.hpp>

#include "fastgltf/core.hpp"
#include "fastgltf/types.hpp"
#include "vku/buffers/device_buffer.hpp"
#include "vku/texture/texture.hpp"

namespace gltf {

class Asset {
  fastgltf::GltfDataBuffer dataBuffer_;

 public:
  struct Vertex {
    glm::vec3 position{};
    float uv_x{};
    glm::vec3 normal{};
    float uv_y{};
    glm::vec4 tangent{};
  };

  struct Primitive {
    std::uint32_t materialIdx{};
    std::uint32_t indexCount{};
    std::uint32_t vertexCount{};
    vku::DeviceBuffer indexBuffer;
    vku::DeviceBuffer vertexBuffer;
  };

  struct Mesh {
    std::vector<Primitive> primitives;
    explicit Mesh(std::vector<Primitive>&& primitives)
        : primitives(std::move(primitives)) {}
  };

  struct Material {
    bool doubleSided{false};
    fastgltf::AlphaMode alphaMode{fastgltf::AlphaMode::Opaque};
    float alphaCutoff{1.0F};

    float metallicFactor{1.0F};
    float roughnessFactor{1.0F};

    glm::vec4 baseColorFactor{1.0F};
    glm::vec4 emissiveFactor{0.0F};

    std::optional<std::uint32_t> baseColorTexture;
    std::optional<std::uint32_t> metallicRoughnessTexture;
    std::optional<std::uint32_t> normalTexture;
    std::optional<std::uint32_t> occlusionTexture;
    std::optional<std::uint32_t> emissiveTexture;
  };

  std::filesystem::path directory;
  fastgltf::Asset asset;

  std::unordered_map<std::size_t, Material> materials;
  std::unordered_map<std::size_t, std::shared_ptr<vku::Texture>> textures;
  std::unordered_map<std::size_t, std::shared_ptr<Mesh>> meshes;

  std::size_t sceneIdx{0};

  Asset(const std::filesystem::path& path, vma::Allocator allocator,
        const vku::DeviceCopyInfo& copyInfo);

  [[nodiscard]] fastgltf::Scene& getScene() noexcept {
    return asset.scenes[sceneIdx];
  }

  [[nodiscard]] const fastgltf::Scene& getScene() const noexcept {
    return asset.scenes[sceneIdx];
  }

 private:
  vma::Allocator allocator_;
  vku::DeviceCopyInfo copyInfo_;

  void loadMaterial(const fastgltf::Material&, std::size_t);
  void loadTexture(const fastgltf::Texture&, std::size_t, vk::Format format);

  void loadMesh(const fastgltf::Mesh&, std::size_t);
  Primitive loadPrimitive(const fastgltf::Primitive&);
};

}  // namespace gltf

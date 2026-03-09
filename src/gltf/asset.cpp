#include "gltf/asset.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "fastgltf/core.hpp"
#include "fastgltf/glm_element_traits.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include "gltf/helpers.hpp"

namespace gltf {
namespace {
auto parser = fastgltf::Parser{
    fastgltf::Extensions::KHR_materials_variants |
        fastgltf::Extensions::KHR_texture_transform |
        fastgltf::Extensions::KHR_mesh_quantization |
        fastgltf::Extensions::EXT_meshopt_compression |
        fastgltf::Extensions::KHR_draco_mesh_compression,
};

auto options = fastgltf::Options::GenerateMeshIndices |
               fastgltf::Options::DecomposeNodeMatrices |
               fastgltf::Options::LoadExternalBuffers;
};  // namespace

Asset::Asset(const std::filesystem::path& path, vma::Allocator allocator,
             const vku::DeviceCopyInfo& copyInfo)
    : dataBuffer_{get_checked(fastgltf::GltfDataBuffer::FromPath(path))},
      directory(path.parent_path()),
      asset{get_checked(parser.loadGltf(dataBuffer_, directory, options))},
      sceneIdx{asset.defaultScene.value_or(0)},
      allocator_{allocator},
      copyInfo_{copyInfo} {
  for (std::size_t i = 0; i < asset.materials.size(); ++i) {
    loadMaterial(asset.materials[i], i);
  }

  for (std::size_t i = 0; i < asset.meshes.size(); ++i) {
    loadMesh(asset.meshes[i], i);
  }
}

void Asset::loadMaterial(const fastgltf::Material& material, std::size_t idx) {
  Material mat{};
  mat.doubleSided = material.doubleSided;
  mat.alphaMode = material.alphaMode;
  mat.alphaCutoff = material.alphaCutoff;

  mat.baseColorFactor = glm::make_vec4(material.pbrData.baseColorFactor.data());
  mat.metallicFactor = material.pbrData.metallicFactor;
  mat.roughnessFactor = material.pbrData.roughnessFactor;
  mat.emissiveFactor =
      glm::vec4(glm::make_vec3(material.emissiveFactor.data()), 1.0F);

  auto get_tex = [&](const auto& gltfTex,
                     vk::Format format) -> std::optional<uint32_t> {
    if constexpr (requires { gltfTex.has_value(); }) {
      if (!gltfTex.has_value()) return std::nullopt;

      auto tex_idx = gltfTex->textureIndex;
      if (!textures.contains(tex_idx)) {
        loadTexture(asset.textures[tex_idx], tex_idx, format);
      }

      return static_cast<uint32_t>(tex_idx);
    }

    return std::nullopt;
  };

  mat.baseColorTexture =
      get_tex(material.pbrData.baseColorTexture, vk::Format::eR8G8B8A8Srgb);

  mat.emissiveTexture =
      get_tex(material.emissiveTexture, vk::Format::eR8G8B8A8Srgb);

  mat.metallicRoughnessTexture = get_tex(
      material.pbrData.metallicRoughnessTexture, vk::Format::eR8G8B8A8Unorm);

  mat.normalTexture =
      get_tex(material.normalTexture, vk::Format::eR8G8B8A8Unorm);

  mat.occlusionTexture =
      get_tex(material.occlusionTexture, vk::Format::eR8G8B8A8Unorm);

  materials.emplace(idx, mat);
}

void Asset::loadTexture(const fastgltf::Texture& texture, std::size_t idx,
                        vk::Format format) {
  if (!texture.imageIndex.has_value()) return;

  const auto& image = asset.images[*texture.imageIndex];
  vku::TextureInfo info;

  std::visit(fastgltf::visitor{
                 [&](const fastgltf::sources::URI& uri) {
                   auto path = directory / uri.uri.path();
                   info = vku::TextureInfo::fromPath(path, format);
                 },
                 [&](const fastgltf::sources::Vector& vector) {
                   info = vku::TextureInfo::fromBuffer(
                       reinterpret_cast<const std::byte*>(vector.bytes.data()),
                       vector.bytes.size(), format);
                 },
                 [&](const fastgltf::sources::Array& array) {
                   info = vku::TextureInfo::fromBuffer(
                       reinterpret_cast<const std::byte*>(array.bytes.data()),
                       array.bytes.size(), format);
                 },
                 [&](const fastgltf::sources::BufferView& view) {
                   auto& buffer_view = asset.bufferViews[view.bufferViewIndex];
                   auto& buffer = asset.buffers[buffer_view.bufferIndex];

                   std::visit(
                       fastgltf::visitor{
                           [&](const fastgltf::sources::Vector& vector) {
                             info = vku::TextureInfo::fromBuffer(
                                 reinterpret_cast<const std::byte*>(
                                     vector.bytes.data()) +
                                     buffer_view.byteOffset,
                                 buffer_view.byteLength, format);
                           },
                           [&](const fastgltf::sources::Array& array) {
                             info = vku::TextureInfo::fromBuffer(
                                 reinterpret_cast<const std::byte*>(
                                     array.bytes.data()) +
                                     buffer_view.byteOffset,
                                 buffer_view.byteLength, format);
                           },
                           [](auto&) {
                             throw std::runtime_error(
                                 "Unsupported buffer source in BufferView");
                           }},
                       buffer.data);
                 },
                 [](auto&) {
                   throw std::runtime_error("Unsupported image source type");
                 }},
             image.data);

  auto allocated_texture = std::make_shared<vku::Texture2D>(
      info, allocator_, copyInfo_, 1, vk::ImageUsageFlagBits::eSampled);

  textures.emplace(idx, allocated_texture);
}

void Asset::loadMesh(const fastgltf::Mesh& mesh, std::size_t idx) {
  auto primitives = std::vector<Primitive>{};
  primitives.reserve(mesh.primitives.size());

  for (const auto& primitive : mesh.primitives) {
    primitives.emplace_back(loadPrimitive(primitive));
  }

  meshes.emplace(idx, std::make_shared<Mesh>(std::move(primitives)));
}

auto Asset::loadPrimitive(const fastgltf::Primitive& primitive) -> Primitive {
  auto indices = std::vector<std::uint32_t>{};
  auto vertices = std::vector<Vertex>{};

  // --- INDICES ---
  if (primitive.indicesAccessor.has_value()) {
    const auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
    indices.resize(accessor.count);
    fastgltf::copyFromAccessor<std::uint32_t>(asset, accessor, indices.data());
  }

  // --- POSITIONS ---
  const auto* pos_it = primitive.findAttribute("POSITION");
  if (pos_it != primitive.attributes.end()) {
    const auto& accessor = asset.accessors[pos_it->accessorIndex];
    vertices.resize(accessor.count);
    fastgltf::iterateAccessorWithIndex<glm::vec3>(
        asset, accessor,
        [&](glm::vec3 pos, std::size_t i) { vertices[i].position = pos; });
  }

  // --- NORMALS ---
  if (const auto* it = primitive.findAttribute("NORMAL");
      it != primitive.attributes.end()) {
    fastgltf::iterateAccessorWithIndex<glm::vec3>(
        asset, asset.accessors[it->accessorIndex],
        [&](glm::vec3 n, std::size_t i) { vertices[i].normal = n; });
  }

  // --- UVs ---
  if (const auto* it = primitive.findAttribute("TEXCOORD_0");
      it != primitive.attributes.end()) {
    fastgltf::iterateAccessorWithIndex<glm::vec2>(
        asset, asset.accessors[it->accessorIndex],
        [&](glm::vec2 uv, std::size_t i) {
          vertices[i].uv_x = uv.x;
          vertices[i].uv_y = uv.y;
        });
  }

  // --- TANGENTS ---
  if (const auto* it = primitive.findAttribute("TANGENT");
      it != primitive.attributes.end()) {
    fastgltf::iterateAccessorWithIndex<glm::vec4>(
        asset, asset.accessors[it->accessorIndex],
        [&](glm::vec4 t, std::size_t i) { vertices[i].tangent = t; });
  }

  return Primitive{
      .materialIdx = static_cast<uint32_t>(primitive.materialIndex.value_or(0)),
      .indexCount = static_cast<uint32_t>(indices.size()),
      .vertexCount = static_cast<uint32_t>(vertices.size()),
      .indexBuffer =
          vku::DeviceBuffer{
              allocator_,
              copyInfo_,
              std::from_range,
              indices,
              vk::BufferUsageFlagBits::eIndexBuffer,
          },
      .vertexBuffer =
          vku::DeviceBuffer{
              allocator_,
              copyInfo_,
              std::from_range,
              vertices,
              vk::BufferUsageFlagBits::eShaderDeviceAddress |
                  vk::BufferUsageFlagBits::eVertexBuffer,
          },
  };
}
}  // namespace gltf

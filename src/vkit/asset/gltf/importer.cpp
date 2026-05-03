#include "vkit/asset/gltf/importer.hpp"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <limits>

#include "fastgltf/core.hpp"
#include "vkit/dataformat/vertex_format.hpp"

#define LOG_TRACE(msg) std::cout << "[glTF Importer] " << msg << std::endl

namespace vkit::asset::gltf {

namespace {

auto loadFileAsBinary(const std::filesystem::path& path)
    -> std::vector<std::byte> {
  LOG_TRACE("Opening binary file: " << path.string());
  auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
  if (!file) throw std::runtime_error{"failed to open file: " + path.string()};
  const auto size = file.tellg();
  if (size < 0)
    throw std::runtime_error{"failed to determine file size: " + path.string()};

  std::vector<std::byte> buffer(static_cast<std::size_t>(size));
  file.seekg(0, std::ios::beg);
  if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
    throw std::runtime_error{"failed to read file: " + path.string()};
  }
  LOG_TRACE("Successfully read " << size << " bytes.");
  return buffer;
}

template <std::move_constructible T>
[[nodiscard]] auto getChecked(fastgltf::Expected<T> expected) -> T {
  if (expected) return std::move(expected.get());
  std::cerr << "[glTF Error] fastgltf parsing failed: "
            << fastgltf::getErrorMessage(expected.error()) << std::endl;
  throw expected.error();
}

};  // namespace

Importer::Importer(const graphics::GfxDevice& gfxDevice)
    : gfxDevice_{gfxDevice} {}

void Importer::resetState() {
  asset_ = nullptr;
  deviceBuffers_ = nullptr;
  loadedMeshes_.clear();
  loadedNodes_.clear();
  loadedSkins_.clear();
}

auto Importer::load(const std::filesystem::path& filepath)
    -> std::shared_ptr<Asset> {
  LOG_TRACE("=== Starting to load asset: " << filepath.string() << " ===");
  resetState();

  asset_ = std::make_shared<Asset>(filepath.stem().string());
  if (!filepath.parent_path().filename().string().empty()) {
    asset_->setName(filepath.parent_path().filename().string());
  }

  auto parser = fastgltf::Parser{
      fastgltf::Extensions::KHR_mesh_quantization |
          fastgltf::Extensions::KHR_materials_pbrSpecularGlossiness,
  };

  auto options = fastgltf::Options::GenerateMeshIndices |
                 fastgltf::Options::DecomposeNodeMatrices |
                 fastgltf::Options::LoadExternalBuffers;

  LOG_TRACE("Loading GltfDataBuffer from path...");
  auto data_buffer = getChecked(fastgltf::GltfDataBuffer::FromPath(filepath));
  const auto directory = filepath.parent_path();

  LOG_TRACE("Parsing glTF file structure...");
  auto gltf_asset =
      getChecked(parser.loadGltf(data_buffer, directory, options));

  LOG_TRACE("Calling loadBuffers...");
  loadBuffers(gltf_asset, directory);

  LOG_TRACE("Calling loadMeshes...");
  loadMeshes(gltf_asset);

  LOG_TRACE("Calling loadNodes...");
  loadNodes(gltf_asset);

  LOG_TRACE("Calling loadSkins...");
  loadSkins(gltf_asset);

  LOG_TRACE("Calling loadScenes...");
  loadScenes(gltf_asset);

  auto final_asset = std::move(asset_);
  resetState();

  return final_asset;
}

auto Importer::mapAttributeFormat(fastgltf::ComponentType compType,
                                  fastgltf::AccessorType accType)
    -> dataformat::AttributeFormat {
  using AttributeFormat = dataformat::AttributeFormat;

  if (compType == fastgltf::ComponentType::Float) {
    if (accType == fastgltf::AccessorType::Scalar)
      return AttributeFormat::kScalarFloat32;
    if (accType == fastgltf::AccessorType::Vec2)
      return AttributeFormat::kVec2Float32;
    if (accType == fastgltf::AccessorType::Vec3)
      return AttributeFormat::kVec3Float32;
    if (accType == fastgltf::AccessorType::Vec4)
      return AttributeFormat::kVec4Float32;
    if (accType == fastgltf::AccessorType::Mat2)
      return AttributeFormat::kMat2Float32;
    if (accType == fastgltf::AccessorType::Mat3)
      return AttributeFormat::kMat3Float32;
    if (accType == fastgltf::AccessorType::Mat4)
      return AttributeFormat::kMat4Float32;
  } else if (compType == fastgltf::ComponentType::UnsignedInt) {
    if (accType == fastgltf::AccessorType::Scalar)
      return AttributeFormat::kScalarUInt32;
    if (accType == fastgltf::AccessorType::Vec2)
      return AttributeFormat::kVec2UInt32;
    if (accType == fastgltf::AccessorType::Vec3)
      return AttributeFormat::kVec3UInt32;
    if (accType == fastgltf::AccessorType::Vec4)
      return AttributeFormat::kVec4UInt32;
  } else if (compType == fastgltf::ComponentType::UnsignedShort) {
    if (accType == fastgltf::AccessorType::Scalar)
      return AttributeFormat::kScalarUInt16;
    if (accType == fastgltf::AccessorType::Vec2)
      return AttributeFormat::kVec2UInt16;
    if (accType == fastgltf::AccessorType::Vec3)
      return AttributeFormat::kVec3UInt16;
    if (accType == fastgltf::AccessorType::Vec4)
      return AttributeFormat::kVec4UInt16;
  } else if (compType == fastgltf::ComponentType::Short) {
    if (accType == fastgltf::AccessorType::Scalar)
      return AttributeFormat::kScalarInt16;
    if (accType == fastgltf::AccessorType::Vec2)
      return AttributeFormat::kVec2Int16;
    if (accType == fastgltf::AccessorType::Vec3)
      return AttributeFormat::kVec3Int16;
    if (accType == fastgltf::AccessorType::Vec4)
      return AttributeFormat::kVec4Int16;
  } else if (compType == fastgltf::ComponentType::UnsignedByte) {
    if (accType == fastgltf::AccessorType::Scalar)
      return AttributeFormat::kScalarUInt8;
    if (accType == fastgltf::AccessorType::Vec2)
      return AttributeFormat::kVec2UInt8;
    if (accType == fastgltf::AccessorType::Vec3)
      return AttributeFormat::kVec3UInt8;
    if (accType == fastgltf::AccessorType::Vec4)
      return AttributeFormat::kVec4UInt8;
  } else if (compType == fastgltf::ComponentType::Byte) {
    if (accType == fastgltf::AccessorType::Scalar)
      return AttributeFormat::kScalarInt8;
    if (accType == fastgltf::AccessorType::Vec2)
      return AttributeFormat::kVec2Int8;
    if (accType == fastgltf::AccessorType::Vec3)
      return AttributeFormat::kVec3Int8;
    if (accType == fastgltf::AccessorType::Vec4)
      return AttributeFormat::kVec4Int8;
  }

  return AttributeFormat::kInvalid;
}

void Importer::populateAttribute(primitive::Attribute& attr,
                                 const fastgltf::Asset& asset,
                                 std::size_t accessorIdx) {
  if (accessorIdx >= asset.accessors.size()) {
    std::cerr << "[glTF Error] Accessor index out of bounds: " << accessorIdx
              << std::endl;
    return;
  }

  const auto& accessor = asset.accessors[accessorIdx];

  attr.bufferViewIdx = accessor.bufferViewIndex.value_or(
      std::numeric_limits<std::size_t>::max());

  attr.info.format = mapAttributeFormat(accessor.componentType, accessor.type);
  attr.info.offset = static_cast<std::uint32_t>(accessor.byteOffset);
  attr.info.count = static_cast<std::uint32_t>(accessor.count);

  if (accessor.bufferViewIndex.has_value()) {
    if (accessor.bufferViewIndex.value() >= asset.bufferViews.size()) {
      std::cerr << "[glTF Error] BufferView index out of bounds!" << std::endl;
      attr.info.stride = 0;
      return;
    }
    const auto& bv = asset.bufferViews[accessor.bufferViewIndex.value()];
    if (bv.byteStride.has_value()) {
      attr.info.stride = static_cast<std::uint32_t>(bv.byteStride.value());
    } else {
      attr.info.stride = static_cast<std::uint32_t>(
          dataformat::getComponentByteSize(attr.info.format) *
          dataformat::getComponentCount(attr.info.format));
    }
  } else {
    attr.info.stride = 0;
  }
}

void Importer::loadBuffers(const fastgltf::Asset& asset,
                           const std::filesystem::path& directory) {
  LOG_TRACE("Loading " << asset.buffers.size() << " buffers...");
  auto cpu_buffers = std::make_shared<primitive::Buffers>();

  for (std::size_t i = 0; i < asset.buffers.size(); ++i) {
    LOG_TRACE("Processing buffer " << i);
    std::vector<std::byte> raw_data;
    std::visit(
        fastgltf::visitor{
            [&](const fastgltf::sources::Array& array) {
              LOG_TRACE("  Source: Array");
              raw_data = std::vector<std::byte>(
                  reinterpret_cast<const std::byte*>(array.bytes.data()),
                  reinterpret_cast<const std::byte*>(array.bytes.data()) +
                      array.bytes.size());
            },
            [&](const fastgltf::sources::Vector& vector) {
              LOG_TRACE("  Source: Vector");
              raw_data = std::vector<std::byte>(
                  reinterpret_cast<const std::byte*>(vector.bytes.data()),
                  reinterpret_cast<const std::byte*>(vector.bytes.data()) +
                      vector.bytes.size());
            },
            [&](const fastgltf::sources::ByteView& byte_view) {
              LOG_TRACE("  Source: ByteView");
              raw_data = std::vector<std::byte>(byte_view.bytes.begin(),
                                                byte_view.bytes.end());
            },
            [&](const fastgltf::sources::URI& uri) {
              LOG_TRACE("  Source: URI (" << uri.uri.fspath() << ")");
              if (uri.uri.isLocalPath()) {
                raw_data = loadFileAsBinary(directory / uri.uri.fspath());
              }
            },
            [](const auto&) {
              std::cerr << "[glTF Warning] Unhandled buffer source format!"
                        << std::endl;
            }},
        asset.buffers[i].data);

    LOG_TRACE(
        "  Buffer " << i << " added to CPU buffers. Size: " << raw_data.size());
    cpu_buffers->addBuffer(i, std::move(raw_data));
  }

  LOG_TRACE("Adding " << asset.bufferViews.size() << " buffer views...");
  for (const auto& bv : asset.bufferViews) {
    cpu_buffers->addBufferView(bv.bufferIndex, bv.byteOffset, bv.byteLength);
  }

  LOG_TRACE("Creating GPU DeviceBuffers...");
  graphics::util::RecordAndSubmitInfo rs_info{
      .device = gfxDevice_.get(),
      .queue = gfxDevice_.queues.transfer,
      .commandPool = gfxDevice_.getTransferCommandPool(),
  };

  deviceBuffers_ = std::make_shared<primitive::DeviceBuffers>(
      cpu_buffers, gfxDevice_.allocator, rs_info);
  LOG_TRACE("DeviceBuffers created.");
}

void Importer::loadMeshes(const fastgltf::Asset& asset) {
  LOG_TRACE("Loading " << asset.meshes.size() << " meshes...");
  loadedMeshes_.reserve(asset.meshes.size());

  for (std::size_t i = 0; i < asset.meshes.size(); ++i) {
    LOG_TRACE("  Mesh " << i << " (" << asset.meshes[i].name << ")");
    const auto& gltf_mesh = asset.meshes[i];
    auto mesh = std::make_shared<scene::Mesh>(gltf_mesh.name.c_str());

    for (std::size_t p = 0; p < gltf_mesh.primitives.size(); ++p) {
      LOG_TRACE("    Primitive " << p);
      const auto& gltf_prim = gltf_mesh.primitives[p];
      primitive::PrimitiveAttributes attrs{};

      if (gltf_prim.indicesAccessor.has_value()) {
        populateAttribute(attrs.index, asset,
                          gltf_prim.indicesAccessor.value());
      }

      if (const auto* it = gltf_prim.findAttribute("POSITION")) {
        populateAttribute(attrs.position, asset, it->accessorIndex);
      }
      if (const auto* it = gltf_prim.findAttribute("NORMAL")) {
        populateAttribute(attrs.normal, asset, it->accessorIndex);
      }
      if (const auto* it = gltf_prim.findAttribute("TANGENT")) {
        populateAttribute(attrs.tangent, asset, it->accessorIndex);
      }
      constexpr std::array<std::string_view, 4> kTexcoordNames = {
          "TEXCOORD_0", "TEXCOORD_1", "TEXCOORD_2", "TEXCOORD_3"};
      for (std::size_t i = 0; i < kTexcoordNames.size(); ++i) {
        if (const auto* it = gltf_prim.findAttribute(kTexcoordNames[i])) {
          populateAttribute(attrs.texcoords[i], asset, it->accessorIndex);
        }
      }

      constexpr std::array<std::string_view, 2> kJointNames = {"JOINTS_0",
                                                               "JOINTS_1"};
      constexpr std::array<std::string_view, 2> kWeightNames = {"WEIGHTS_0",
                                                                "WEIGHTS_1"};
      for (std::size_t i = 0; i < kJointNames.size(); ++i) {
        if (const auto* it = gltf_prim.findAttribute(kJointNames[i])) {
          populateAttribute(attrs.jointIndices[i], asset, it->accessorIndex);
        }
        if (const auto* it = gltf_prim.findAttribute(kWeightNames[i])) {
          populateAttribute(attrs.jointWeights[i], asset, it->accessorIndex);
        }
      }

      auto prim = std::make_shared<primitive::Primitive>(deviceBuffers_, attrs);
      prim->setMaterialSlot(gltf_prim.materialIndex.value_or(0));

      asset_->primitives.add(prim);
      mesh->addPrimitive(prim);
    }

    asset_->meshes.add(mesh);
    loadedMeshes_.push_back(mesh);
  }
}

void Importer::loadNodes(const fastgltf::Asset& asset) {
  LOG_TRACE("Loading " << asset.nodes.size() << " nodes...");
  loadedNodes_.resize(asset.nodes.size());

  for (std::size_t i = 0; i < asset.nodes.size(); ++i) {
    LOG_TRACE("  Node " << i);
    const auto& gltf_node = asset.nodes[i];
    auto node = std::make_shared<scene::Node>(gltf_node.name.c_str());

    std::visit(
        fastgltf::visitor{
            [&](const fastgltf::math::fmat4x4& matrix) {
              node->setLocalTransform(
                  scene::TrsTransform{glm::make_mat4x4(matrix.data())});
            },
            [&](const fastgltf::TRS& trs) {
              scene::TrsTransform transform;
              transform.setTranslation(glm::make_vec3(trs.translation.data()));
              transform.setRotation(glm::make_quat(trs.rotation.data()));
              transform.setScale(glm::make_vec3(trs.scale.data()));
              node->setLocalTransform(transform);
            }},
        gltf_node.transform);

    if (gltf_node.meshIndex.has_value()) {
      auto m_idx = gltf_node.meshIndex.value();
      if (m_idx < loadedMeshes_.size()) {
        node->mesh = loadedMeshes_[m_idx];
      } else {
        std::cerr << "[glTF Error] Node references out-of-bounds mesh!"
                  << std::endl;
      }
    }

    asset_->nodes.add(node);
    loadedNodes_[i] = node;
  }

  LOG_TRACE("Resolving node children hierarchy...");
  for (std::size_t i = 0; i < asset.nodes.size(); ++i) {
    for (auto child_idx : asset.nodes[i].children) {
      if (child_idx < loadedNodes_.size()) {
        loadedNodes_[i]->addChild(loadedNodes_[child_idx]);
      } else {
        std::cerr << "[glTF Error] Node references out-of-bounds child node!"
                  << std::endl;
      }
    }
  }
}

void Importer::loadSkins(const fastgltf::Asset& asset) {
  LOG_TRACE("Loading " << asset.skins.size() << " skins...");
  loadedSkins_.reserve(asset.skins.size());

  for (std::size_t i = 0; i < asset.skins.size(); ++i) {
    const auto& gltf_skin = asset.skins[i];
    auto skin = std::make_shared<scene::Skin>(gltf_skin.name.c_str());

    if (gltf_skin.skeleton.has_value()) {
      skin->skeleton = loadedNodes_[gltf_skin.skeleton.value()];
    }

    skin->joints.reserve(gltf_skin.joints.size());
    for (auto joint_idx : gltf_skin.joints) {
      skin->joints.push_back(loadedNodes_[joint_idx]);
    }

    if (gltf_skin.inverseBindMatrices.has_value()) {
      const auto& accessor =
          asset.accessors[gltf_skin.inverseBindMatrices.value()];
      skin->inverseBindMatrices.resize(skin->joints.size());

      fastgltf::iterateAccessorWithIndex<glm::mat4>(
          asset, accessor, [&](glm::mat4 mat, std::size_t idx) {
            skin->inverseBindMatrices[idx] = mat;
          });
    } else {
      skin->inverseBindMatrices.resize(skin->joints.size(), glm::mat4(1.0F));
    }

    asset_->skins.add(skin);
    loadedSkins_.push_back(skin);
  }

  LOG_TRACE("Attaching skins to nodes...");
  for (std::size_t i = 0; i < asset.nodes.size(); ++i) {
    if (asset.nodes[i].skinIndex.has_value()) {
      loadedNodes_[i]->skin = loadedSkins_[asset.nodes[i].skinIndex.value()];
    }
  }
}

void Importer::loadScenes(const fastgltf::Asset& asset) {
  LOG_TRACE("Loading " << asset.scenes.size() << " scenes...");
  for (std::size_t i = 0; i < asset.scenes.size(); ++i) {
    LOG_TRACE("  Scene " << i);
    const auto& gltf_scene = asset.scenes[i];
    auto new_scene = std::make_shared<scene::Scene>(gltf_scene.name.c_str());

    for (auto root_idx : gltf_scene.nodeIndices) {
      if (root_idx < loadedNodes_.size()) {
        if (auto storage_id = loadedNodes_[root_idx]->getStorageId()) {
          new_scene->addRootNode(storage_id.value());
        }
      } else {
        std::cerr << "[glTF Error] Scene references out-of-bounds root node!"
                  << std::endl;
      }
    }

    asset_->scenes.push_back(new_scene);
  }

  if (asset.defaultScene.has_value() &&
      asset.defaultScene.value() < asset_->scenes.size()) {
    asset_->activeSceneIndex =
        static_cast<std::int32_t>(asset.defaultScene.value());
  }
}

};  // namespace vkit::asset::gltf

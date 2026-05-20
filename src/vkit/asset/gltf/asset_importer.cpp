#include "vkit/asset/gltf/asset_importer.hpp"

#include <cmath>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fstream>
#include <print>
#include <glm/gtc/type_ptr.hpp>
#include <limits>

#include "fastgltf/core.hpp"
#include "vkit/dataformat/vertex_format.hpp"

namespace vkit::asset::gltf {

namespace {

auto loadFileAsBinary(const std::filesystem::path& path)
    -> std::vector<std::byte> {
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
  return buffer;
}

template <std::move_constructible T>
[[nodiscard]] auto getChecked(fastgltf::Expected<T> expected) -> T {
  if (expected) return std::move(expected.get());
  std::println(stderr, "[glTF Error] fastgltf parsing failed: {}",
               fastgltf::getErrorMessage(expected.error()));
  throw expected.error();
}

};  // namespace

AssetImporter::AssetImporter(const graphics::GfxDevice& gfxDevice)
    : gfxDevice_{gfxDevice} {}

void AssetImporter::resetState() {
  asset_ = nullptr;
  deviceBuffers_ = nullptr;
  loadedMeshes_.clear();
  loadedNodes_.clear();
  loadedSkins_.clear();
  loadedAnimations_.clear();
}

auto AssetImporter::load(const std::filesystem::path& filepath)
    -> std::shared_ptr<Asset> {
  resetState();

  asset_ = std::make_shared<Asset>(filepath.stem().string());
  if (!filepath.parent_path().filename().string().empty()) {
    asset_->setName(filepath.parent_path().filename().string());
  }

  auto parser = fastgltf::Parser{
      fastgltf::Extensions::KHR_mesh_quantization |
          fastgltf::Extensions::KHR_materials_ior |
          fastgltf::Extensions::KHR_materials_specular |
          fastgltf::Extensions::KHR_materials_transmission |
          fastgltf::Extensions::KHR_materials_volume |
          fastgltf::Extensions::KHR_materials_sheen |
          fastgltf::Extensions::KHR_materials_clearcoat |
          fastgltf::Extensions::KHR_materials_emissive_strength |
          fastgltf::Extensions::KHR_materials_iridescence |
          fastgltf::Extensions::KHR_materials_anisotropy |
          fastgltf::Extensions::KHR_materials_unlit |

          // enable deprectated extensinons for models compatibility
          fastgltf::Extensions::KHR_materials_pbrSpecularGlossiness,
  };

  auto options = fastgltf::Options::GenerateMeshIndices |
                 fastgltf::Options::DecomposeNodeMatrices |
                 fastgltf::Options::LoadExternalBuffers;

  auto data_buffer = getChecked(fastgltf::GltfDataBuffer::FromPath(filepath));
  const auto directory = filepath.parent_path();

  auto gltf_asset =
      getChecked(parser.loadGltf(data_buffer, directory, options));

  loadBuffers(gltf_asset, directory);
  loadMaterials(gltf_asset, directory);
  loadMeshes(gltf_asset);
  loadNodes(gltf_asset);
  loadSkins(gltf_asset);
  loadScenes(gltf_asset);
  loadAnimations(gltf_asset);

  auto final_asset = std::move(asset_);
  resetState();

  return final_asset;
}

auto AssetImporter::mapAttributeFormat(fastgltf::ComponentType compType,
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

void AssetImporter::populateAttribute(primitive::Attribute& attr,
                                      const fastgltf::Asset& asset,
                                      std::size_t accessorIdx) {
  if (accessorIdx >= asset.accessors.size()) {
    std::println(stderr, "[glTF Error] Accessor index out of bounds: {}", accessorIdx);
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
      std::println(stderr, "[glTF Error] BufferView index out of bounds!");
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

void AssetImporter::loadBuffers(const fastgltf::Asset& asset,
                                const std::filesystem::path& directory) {
  auto cpu_buffers = std::make_shared<primitive::Buffers>();

  for (std::size_t i = 0; i < asset.buffers.size(); ++i) {
    std::vector<std::byte> raw_data;
    std::visit(
        fastgltf::visitor{
            [&](const fastgltf::sources::Array& array) {
              raw_data = std::vector<std::byte>(
                  reinterpret_cast<const std::byte*>(array.bytes.data()),
                  reinterpret_cast<const std::byte*>(array.bytes.data()) +
                      array.bytes.size());
            },
            [&](const fastgltf::sources::Vector& vector) {
              raw_data = std::vector<std::byte>(
                  reinterpret_cast<const std::byte*>(vector.bytes.data()),
                  reinterpret_cast<const std::byte*>(vector.bytes.data()) +
                      vector.bytes.size());
            },
            [&](const fastgltf::sources::ByteView& byte_view) {
              raw_data = std::vector<std::byte>(byte_view.bytes.begin(),
                                                byte_view.bytes.end());
            },
            [&](const fastgltf::sources::URI& uri) {
              if (uri.uri.isLocalPath()) {
                raw_data = loadFileAsBinary(directory / uri.uri.fspath());
              }
            },
            [](const auto&) {
              std::println(stderr, "[glTF Warning] Unhandled buffer source format!");
            }},
        asset.buffers[i].data);

    cpu_buffers->addBuffer(i, std::move(raw_data));
  }

  for (const auto& bv : asset.bufferViews) {
    cpu_buffers->addBufferView(bv.bufferIndex, bv.byteOffset, bv.byteLength);
  }

  graphics::util::RecordAndSubmitInfo rs_info{
      .device = gfxDevice_.get(),
      .queue = gfxDevice_.queues.transfer,
      .commandPool = gfxDevice_.getTransferCommandPool(),
  };

  deviceBuffers_ = std::make_shared<primitive::DeviceBuffers>(
      cpu_buffers, gfxDevice_.allocator, rs_info);
}

void AssetImporter::loadMaterials(const fastgltf::Asset& asset,
                                  const std::filesystem::path& directory) {
  asset_->gltfMaterials.clear();
  asset_->gltfMaterials.reserve(asset.materials.size());

  auto getImagePath =
      [&](std::size_t texIdx) -> std::optional<std::filesystem::path> {
    if (texIdx >= asset.textures.size()) return std::nullopt;
    auto imgIdx = asset.textures[texIdx].imageIndex;
    if (!imgIdx.has_value() || imgIdx.value() >= asset.images.size())
      return std::nullopt;

    std::filesystem::path result;
    std::visit(fastgltf::visitor{[&](const fastgltf::sources::URI& uri) {
                                   if (uri.uri.isLocalPath())
                                     result = directory / uri.uri.fspath();
                                 },
                                 [](const auto&) {}},
               asset.images[imgIdx.value()].data);

    return result.empty() ? std::nullopt : std::optional{result};
  };

  auto texRef = [&](const fastgltf::Optional<fastgltf::TextureInfo>& ti)
      -> std::optional<GltfTextureRef> {
    if (!ti.has_value()) return std::nullopt;
    auto p = getImagePath(ti->textureIndex);
    return p ? std::optional{GltfTextureRef{*p}} : std::nullopt;
  };

  for (const auto& mat : asset.materials) {
    GltfMaterialInfo info;
    info.name = std::string{mat.name};

    // Alpha
    switch (mat.alphaMode) {
      case fastgltf::AlphaMode::Opaque:
        info.alphaMode = 0;
        break;
      case fastgltf::AlphaMode::Mask:
        info.alphaMode = 1;
        break;
      case fastgltf::AlphaMode::Blend:
        info.alphaMode = 2;
        break;
    }
    info.alphaCutoff = mat.alphaCutoff;
    info.doubleSided = mat.doubleSided;

    // Base PBR textures
    info.baseColorTexture = texRef(mat.pbrData.baseColorTexture);
    info.metallicRoughnessTexture =
        texRef(mat.pbrData.metallicRoughnessTexture);
    info.emissiveTexture = texRef(mat.emissiveTexture);

    if (mat.normalTexture.has_value()) {
      if (auto p = getImagePath(mat.normalTexture->textureIndex))
        info.normalTexture = GltfTextureRef{*p};
    }
    if (mat.occlusionTexture.has_value()) {
      if (auto p = getImagePath(mat.occlusionTexture->textureIndex))
        info.occlusionTexture = GltfTextureRef{*p};
      info.occlusionStrength = mat.occlusionTexture->strength;
    }

    // Base PBR factors
    const auto& bc = mat.pbrData.baseColorFactor;
    info.baseColorFactor = {bc[0], bc[1], bc[2], bc[3]};
    info.metallicFactor = mat.pbrData.metallicFactor;
    info.roughnessFactor = mat.pbrData.roughnessFactor;

    // Emissive: bake emissiveStrength into the factor
    // (KHR_materials_emissive_strength)
    const auto& em = mat.emissiveFactor;
    info.emissiveFactor = glm::vec3{em[0], em[1], em[2]} * mat.emissiveStrength;

    // KHR_materials_ior
    info.ior = mat.ior;

    // KHR_materials_specular
    if (mat.specular) {
      info.specularFactor = mat.specular->specularFactor;
      const auto& sc = mat.specular->specularColorFactor;
      info.specularColorFactor = {sc[0], sc[1], sc[2]};
      info.specularTexture = texRef(mat.specular->specularTexture);
      info.specularColorTexture = texRef(mat.specular->specularColorTexture);
    }

    // KHR_materials_transmission
    if (mat.transmission) {
      info.transmissionFactor = mat.transmission->transmissionFactor;
    }

    // KHR_materials_volume
    if (mat.volume) {
      info.thicknessFactor = mat.volume->thicknessFactor;
      info.attenuationDistance = mat.volume->attenuationDistance;
      const auto& ac = mat.volume->attenuationColor;
      info.attenuationColor = {ac[0], ac[1], ac[2]};
      info.thicknessTexture = texRef(mat.volume->thicknessTexture);
    }

    // KHR_materials_clearcoat
    if (mat.clearcoat) {
      info.clearcoatFactor = mat.clearcoat->clearcoatFactor;
      info.clearcoatRoughnessFactor = mat.clearcoat->clearcoatRoughnessFactor;
      info.clearcoatTexture = texRef(mat.clearcoat->clearcoatTexture);
      info.clearcoatRoughnessTexture =
          texRef(mat.clearcoat->clearcoatRoughnessTexture);
      if (mat.clearcoat->clearcoatNormalTexture.has_value()) {
        if (auto p = getImagePath(
                mat.clearcoat->clearcoatNormalTexture->textureIndex))
          info.clearcoatNormalTexture = GltfTextureRef{*p};
      }
    }

    // KHR_materials_sheen
    if (mat.sheen) {
      const auto& sc = mat.sheen->sheenColorFactor;
      info.sheenColorFactor = {sc[0], sc[1], sc[2]};
      info.sheenRoughnessFactor = mat.sheen->sheenRoughnessFactor;
      info.sheenColorTexture = texRef(mat.sheen->sheenColorTexture);
      info.sheenRoughnessTexture = texRef(mat.sheen->sheenRoughnessTexture);
    }

    // KHR_materials_anisotropy
    if (mat.anisotropy) {
      info.anisotropyStrength = mat.anisotropy->anisotropyStrength;
      const float angle = mat.anisotropy->anisotropyRotation;
      info.anisotropyRotation = {std::cos(angle), std::sin(angle)};
      info.anisotropyTexture = texRef(mat.anisotropy->anisotropyTexture);
    }

    // KHR_materials_iridescence
    if (mat.iridescence) {
      info.iridescenceFactor = mat.iridescence->iridescenceFactor;
      info.iridescenceIor = mat.iridescence->iridescenceIor;
      info.iridescenceThicknessMin =
          mat.iridescence->iridescenceThicknessMinimum;
      info.iridescenceThicknessMax =
          mat.iridescence->iridescenceThicknessMaximum;
      info.iridescenceTexture = texRef(mat.iridescence->iridescenceTexture);
      info.iridescenceThicknessTexture =
          texRef(mat.iridescence->iridescenceThicknessTexture);
    }

    asset_->gltfMaterials.push_back(std::move(info));
  }
}

void AssetImporter::loadMeshes(const fastgltf::Asset& asset) {
  loadedMeshes_.reserve(asset.meshes.size());

  for (std::size_t i = 0; i < asset.meshes.size(); ++i) {
    const auto& gltf_mesh = asset.meshes[i];
    auto mesh = std::make_shared<scene::Mesh>(gltf_mesh.name.c_str());

    for (std::size_t p = 0; p < gltf_mesh.primitives.size(); ++p) {
      const auto& gltf_prim = gltf_mesh.primitives[p];
      primitive::PrimitiveAttributes attrs{};

      if (gltf_prim.indicesAccessor.has_value()) {
        populateAttribute(attrs.index, asset,
                          gltf_prim.indicesAccessor.value());
      }

      if (auto* it = gltf_prim.findAttribute("POSITION");
          it != gltf_prim.attributes.end()) {
        populateAttribute(attrs.position, asset, it->accessorIndex);
        const auto& acc = asset.accessors[it->accessorIndex];
        if (acc.min.has_value() && acc.max.has_value() &&
            acc.min->size() >= 3 && acc.max->size() >= 3 &&
            acc.min->isType<double>() && acc.max->isType<double>()) {
          const auto& mn = *acc.min;
          const auto& mx = *acc.max;
          mesh->aabbMin = glm::min(
              mesh->aabbMin,
              glm::vec3(float(mn.get<double>(0)), float(mn.get<double>(1)),
                        float(mn.get<double>(2))));
          mesh->aabbMax = glm::max(
              mesh->aabbMax,
              glm::vec3(float(mx.get<double>(0)), float(mx.get<double>(1)),
                        float(mx.get<double>(2))));
        }
      }
      if (auto* it = gltf_prim.findAttribute("NORMAL");
          it != gltf_prim.attributes.end()) {
        populateAttribute(attrs.normal, asset, it->accessorIndex);
      }
      if (auto* it = gltf_prim.findAttribute("TANGENT");
          it != gltf_prim.attributes.end()) {
        populateAttribute(attrs.tangent, asset, it->accessorIndex);
      }
      constexpr std::array<std::string_view, 4> kTexcoordNames = {
          "TEXCOORD_0", "TEXCOORD_1", "TEXCOORD_2", "TEXCOORD_3"};
      for (std::size_t i = 0; i < kTexcoordNames.size(); ++i) {
        if (auto* it = gltf_prim.findAttribute(kTexcoordNames[i]);
            it != gltf_prim.attributes.end()) {
          populateAttribute(attrs.texcoords[i], asset, it->accessorIndex);
        }
      }

      constexpr std::array<std::string_view, 2> kJointNames = {"JOINTS_0",
                                                               "JOINTS_1"};
      constexpr std::array<std::string_view, 2> kWeightNames = {"WEIGHTS_0",
                                                                "WEIGHTS_1"};
      for (std::size_t i = 0; i < kJointNames.size(); ++i) {
        if (auto* it = gltf_prim.findAttribute(kJointNames[i]);
            it != gltf_prim.attributes.end()) {
          populateAttribute(attrs.jointIndices[i], asset, it->accessorIndex);
        }
        if (auto* it = gltf_prim.findAttribute(kWeightNames[i]);
            it != gltf_prim.attributes.end()) {
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

void AssetImporter::loadNodes(const fastgltf::Asset& asset) {
  loadedNodes_.resize(asset.nodes.size());

  for (std::size_t i = 0; i < asset.nodes.size(); ++i) {
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
        std::println(stderr, "[glTF Error] Node references out-of-bounds mesh!");
      }
    }

    asset_->nodes.add(node);
    loadedNodes_[i] = node;
  }

  for (std::size_t i = 0; i < asset.nodes.size(); ++i) {
    for (auto child_idx : asset.nodes[i].children) {
      if (child_idx < loadedNodes_.size()) {
        loadedNodes_[i]->addChild(loadedNodes_[child_idx]);
      } else {
        std::println(stderr, "[glTF Error] Node references out-of-bounds child node!");
      }
    }
  }
}

void AssetImporter::loadSkins(const fastgltf::Asset& asset) {
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

  for (std::size_t i = 0; i < asset.nodes.size(); ++i) {
    if (asset.nodes[i].skinIndex.has_value()) {
      loadedNodes_[i]->skin = loadedSkins_[asset.nodes[i].skinIndex.value()];
    }
  }
}

void AssetImporter::loadScenes(const fastgltf::Asset& asset) {
  for (const auto& gltf_scene : asset.scenes) {
    auto new_scene = std::make_shared<scene::Scene>(gltf_scene.name.c_str());

    for (auto root_idx : gltf_scene.nodeIndices) {
      if (root_idx < loadedNodes_.size()) {
        if (auto storage_id = loadedNodes_[root_idx]->getStorageId()) {
          new_scene->addRootNode(storage_id.value());
        }
      }
    }
    asset_->scenes.push_back(new_scene);
  }

  if (asset.scenes.empty() && !asset.nodes.empty()) {
    auto synthetic_scene = std::make_shared<scene::Scene>("Default Scene");

    std::vector<bool> is_child(asset.nodes.size(), false);
    for (const auto& node : asset.nodes) {
      for (auto child_idx : node.children) {
        if (child_idx < is_child.size()) is_child[child_idx] = true;
      }
    }
    for (std::size_t i = 0; i < asset.nodes.size(); ++i) {
      if (!is_child[i]) {
        if (auto storage_id = loadedNodes_[i]->getStorageId()) {
          synthetic_scene->addRootNode(storage_id.value());
        }
      }
    }
    asset_->scenes.push_back(synthetic_scene);
  }

  if (asset.defaultScene.has_value() &&
      asset.defaultScene.value() < asset_->scenes.size()) {
    asset_->activeSceneIndex =
        static_cast<std::int32_t>(asset.defaultScene.value());
  } else {
    asset_->activeSceneIndex = 0;
  }
}

void AssetImporter::loadAnimations(const fastgltf::Asset& asset) {
  for (const auto& gltf_anim : asset.animations) {
    auto anim = std::make_shared<animation::Animation>(
        gltf_anim.name.empty() ? "Animation" : gltf_anim.name.c_str());

    for (const auto& gltf_sampler : gltf_anim.samplers) {
      animation::AnimationSampler sampler;

      if (gltf_sampler.interpolation ==
          fastgltf::AnimationInterpolation::Linear) {
        sampler.interpolation =
            animation::AnimationSampler::Interpolation::kLinear;
      } else if (gltf_sampler.interpolation ==
                 fastgltf::AnimationInterpolation::Step) {
        sampler.interpolation =
            animation::AnimationSampler::Interpolation::kStep;
      } else {
        sampler.interpolation =
            animation::AnimationSampler::Interpolation::kCubicSpline;
      }

      const auto& input_accessor = asset.accessors[gltf_sampler.inputAccessor];
      sampler.inputs.resize(input_accessor.count);

      fastgltf::iterateAccessorWithIndex<float>(
          asset, input_accessor, [&](float value, std::size_t idx) {
            sampler.inputs[idx] = value;
            anim->start = std::min(anim->start, value);
            anim->end = std::max(anim->end, value);
          });

      const auto& output_accessor =
          asset.accessors[gltf_sampler.outputAccessor];
      sampler.outputs.resize(output_accessor.count);

      if (output_accessor.type == fastgltf::AccessorType::Vec3) {
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
            asset, output_accessor,
            [&](fastgltf::math::fvec3 value, std::size_t idx) {
              sampler.outputs[idx] =
                  glm::vec4(value.x(), value.y(), value.z(), 0.0F);
            });
      } else if (output_accessor.type == fastgltf::AccessorType::Vec4) {
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
            asset, output_accessor,
            [&](fastgltf::math::fvec4 value, std::size_t idx) {
              sampler.outputs[idx] =
                  glm::vec4(value.x(), value.y(), value.z(), value.w());
            });
      }

      anim->samplers.push_back(std::move(sampler));
    }

    for (const auto& gltf_channel : gltf_anim.channels) {
      if (!gltf_channel.nodeIndex.has_value()) continue;

      animation::AnimationChannel channel;
      channel.samplerIndex = gltf_channel.samplerIndex;
      channel.targetNode = loadedNodes_[gltf_channel.nodeIndex.value()];

      if (gltf_channel.path == fastgltf::AnimationPath::Translation) {
        channel.path = animation::AnimationChannel::Path::kTranslation;
      } else if (gltf_channel.path == fastgltf::AnimationPath::Rotation) {
        channel.path = animation::AnimationChannel::Path::kRotation;
      } else if (gltf_channel.path == fastgltf::AnimationPath::Scale) {
        channel.path = animation::AnimationChannel::Path::kScale;
      } else {
        continue;
      }

      anim->channels.push_back(std::move(channel));
    }

    loadedAnimations_.push_back(anim);
    asset_->animations.push_back(anim);
  }
}

};  // namespace vkit::asset::gltf

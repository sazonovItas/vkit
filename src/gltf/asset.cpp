#include "gltf/asset.hpp"

#include "gltf/helpers.hpp"

namespace {
auto parser = fastgltf::Parser{
    fastgltf::Extensions::KHR_materials_variants |
        fastgltf::Extensions::KHR_texture_transform |
        fastgltf::Extensions::KHR_mesh_quantization |
        fastgltf::Extensions::EXT_meshopt_compression,
};
};  // namespace

namespace gltf {
Asset::Asset(const std::filesystem::path& path)
    : dataBuffer_{get_checked(fastgltf::GltfDataBuffer::FromPath(path))},
      directory(path.parent_path()),
      asset{get_checked(parser.loadGltf(dataBuffer_, directory))},
      sceneIdx{asset.defaultScene.value_or(0)} {}
};  // namespace gltf

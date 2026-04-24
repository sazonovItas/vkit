#include "vkit/graphics/shader_module.hpp"

#include <fstream>

namespace vkit::graphics {

namespace {

auto toSpirV(std::filesystem::path const& path) -> std::vector<std::uint32_t> {
  auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
  if (!file.is_open()) {
    throw std::runtime_error{std::format(
        "[SHADER] [SPIRV] Failed to open file: '{}'", path.generic_string())};
  }

  auto const size = file.tellg();
  auto const usize = static_cast<std::uint64_t>(size);
  if (0 != usize % sizeof(std::uint32_t)) {
    throw std::runtime_error{
        std::format("[SHADER] [SPIRV] Invalid SPIR-V size: {}", usize)};
  }

  file.seekg({}, std::ios::beg);
  auto ret = std::vector<std::uint32_t>{};
  ret.resize(usize / sizeof(std::uint32_t));
  void* data = ret.data();
  file.read(static_cast<char*>(data), size);
  return ret;
}

};  // namespace

SpirVShaderModule::SpirVShaderModule(vk::Device device,
                                     const std::filesystem::path& path)
    : ShaderModule{device, toSpirV(path)} {}

};  // namespace vkit::graphics

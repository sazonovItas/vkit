#pragma once

#include <memory>

#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include "vulkan/vma/allocator.hpp"
#include "vulkan/vma/buffer.hpp"

namespace vkit::gltf {
using namespace std::string_view_literals;

class PrimitiveAttributeBuffers {
 public:
  struct AttributeInfo {
    std::shared_ptr<vulkan::vma::Buffer> buffer;
    vk::DeviceSize offset;
    vk::DeviceSize size;
    vk::DeviceSize stride;
    fastgltf::ComponentType component_type;
  };

  class AttributeInfoCache {
   public:
    template <typename BufferDataAdapter = fastgltf::DefaultBufferDataAdapter>
    class Config {
     public:
      const BufferDataAdapter& adapter;

      const std::uint32_t queue_family = {};
    };

    std::unordered_map<std::size_t, std::shared_ptr<vulkan::vma::Buffer>>
        buffer_view_buffers;
    std::unordered_map<std::size_t, AttributeInfo> accessor_attribute_infos;

    template <typename BufferDataAdapter = fastgltf::DefaultBufferDataAdapter>
    AttributeInfoCache(const fastgltf::Asset& asset,
                       const vulkan::vma::Allocator& allocator,
                       const Config<BufferDataAdapter>& config = {});

   private:
    [[nodiscard]] auto create_attribute_info_from_buffer(
        const fastgltf::Asset& asset, const fastgltf::Accessor& accessor) const
        -> AttributeInfo;
  };

  template <typename BufferDataAdapter = fastgltf::DefaultBufferDataAdapter>
  class Config {
   public:
    const BufferDataAdapter& adapter;
    const AttributeInfoCache* cache = nullptr;
  };

  template <typename BufferDataAdapter = fastgltf::DefaultBufferDataAdapter>
  PrimitiveAttributeBuffers(const fastgltf::Asset& asset,
                            const fastgltf::Primitive& primitive,
                            const vulkan::vma::Allocator& allocator,
                            const Config<BufferDataAdapter>& config = {});

 private:
  const fastgltf::Asset& asset_;
  const fastgltf::Primitive& primitive_;
};
};  // namespace vkit::gltf

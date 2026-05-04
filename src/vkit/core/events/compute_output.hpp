#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "vkit/message_bus/message_bus.hpp"
#include "vkit/texture/texture.hpp"

namespace vkit::core::events {

enum class ComputeBindingLayout : uint8_t {
  kGenerator,
  kSingleInput,
  kDualInput,
};

struct ComputeHandles {
  vk::Pipeline pipeline{nullptr};
  vk::PipelineLayout pipelineLayout{nullptr};
  vk::DescriptorSetLayout dsl{nullptr};
  ComputeBindingLayout bindingLayout{ComputeBindingLayout::kGenerator};
};

struct ComputeOutputJob {
  std::uint64_t requestId{0};
  uint32_t width{512};
  uint32_t height{512};
  std::shared_ptr<texture::Texture> inputA;
  std::shared_ptr<texture::Texture> inputB;
  std::shared_ptr<texture::Texture> inputC;
  ComputeHandles handles;
  std::vector<uint8_t> pushData;
};

struct ComputeOutputResult {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> imageF32;
  std::shared_ptr<texture::Texture> imageUnorm;
  std::string error;
};

using ComputeOutputBus =
    message_bus::MessageBus<ComputeOutputJob,
                            message_bus::DispatchPolicy::kBoth>;
using ComputeOutputResultBus =
    message_bus::MessageBus<ComputeOutputResult,
                            message_bus::DispatchPolicy::kBoth>;

}  // namespace vkit::core::events

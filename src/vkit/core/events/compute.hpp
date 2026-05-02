#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "vkit/compute/async_compute.hpp"
#include "vkit/message_bus/message_bus.hpp"
#include "vkit/texture/texture.hpp"

namespace vkit::core::events {

struct ComputeJobRequest {
  std::uint64_t requestId{0};
  std::shared_ptr<compute::ComputeTask> task;
  std::shared_ptr<texture::Texture> outputTexture;
};

struct ComputeJobResult {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> texture;
  std::string error;
};

using ComputeJobBus =
    message_bus::MessageBus<ComputeJobRequest,
                            message_bus::DispatchPolicy::kBoth>;
using ComputeResultBus =
    message_bus::MessageBus<ComputeJobResult,
                            message_bus::DispatchPolicy::kBoth>;

};  // namespace vkit::core::events

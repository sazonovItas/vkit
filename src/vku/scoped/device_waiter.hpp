#pragma once

#include "scoped.hpp"

namespace vku {
struct DeviceWaiterDeleter {
  void operator()(vk::Device const device) const noexcept { device.waitIdle(); }
};

using DeviceWaiter = Scoped<vk::Device, DeviceWaiterDeleter>;
};  // namespace vku

#pragma once

#include "scoped/scoped.hpp"

namespace lvk {
struct ScopedWaiterDeleter {
  void operator()(vk::Device const device) const noexcept { device.waitIdle(); }
};

using ScopedWaiter = vkit::Scoped<vk::Device, ScopedWaiterDeleter>;
};  // namespace lvk

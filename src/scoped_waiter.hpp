#pragma once

#include "util/scoped.hpp"

namespace lvk {
struct ScopedWaiterDeleter {
  void operator()(vk::Device const device) const noexcept { device.waitIdle(); }
};

using ScopedWaiter = vkit::util::Scoped<vk::Device, ScopedWaiterDeleter>;
};  // namespace lvk

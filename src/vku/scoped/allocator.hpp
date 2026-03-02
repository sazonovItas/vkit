#pragma once

#include "scoped.hpp"
#include "vk_mem_alloc.hpp"

namespace vku {
struct AllocatorDeleter {
  void operator()(vma::Allocator allocator) const noexcept {
    allocator.destroy();
  }
};

using Allocator = Scoped<vma::Allocator, AllocatorDeleter>;
};  // namespace vku

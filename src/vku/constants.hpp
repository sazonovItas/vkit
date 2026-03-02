#pragma once

#include "vk_mem_alloc.hpp"

namespace vku::allocation {
static constexpr auto kHostWrite = vma::AllocationCreateInfo{
    vma::AllocationCreateFlagBits::eHostAccessSequentialWrite |
        vma::AllocationCreateFlagBits::eMapped,
    vma::MemoryUsage::eAuto,
};

static constexpr auto kHostTransferWrite = vma::AllocationCreateInfo{
    vma::AllocationCreateFlagBits::eHostAccessSequentialWrite |
        vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead |
        vma::AllocationCreateFlagBits::eMapped,
    vma::MemoryUsage::eAuto,
};

static constexpr auto kHostRead = vma::AllocationCreateInfo{
    vma::AllocationCreateFlagBits::eHostAccessRandom |
        vma::AllocationCreateFlagBits::eMapped,
    vma::MemoryUsage::eAuto,
};

static constexpr auto kHostTransferRead = vma::AllocationCreateInfo{
    vma::AllocationCreateFlagBits::eHostAccessRandom |
        vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead |
        vma::AllocationCreateFlagBits::eMapped,
    vma::MemoryUsage::eAuto,
};

static constexpr auto kDeviceLocal = vma::AllocationCreateInfo{
    {},
    vma::MemoryUsage::eAutoPreferDevice,
};

static constexpr auto kDeviceLocalDedicated = vma::AllocationCreateInfo{
    vma::AllocationCreateFlagBits::eDedicatedMemory,
    vma::MemoryUsage::eAutoPreferDevice,
};
};  // namespace vku::allocation

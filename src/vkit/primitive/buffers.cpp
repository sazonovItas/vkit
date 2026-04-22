#include "vkit/primitive/buffers.hpp"

namespace vkit::primitive {

auto Buffers::operator()(const std::size_t bufferViewIdx) const
    -> std::span<const std::byte> {
  return bufferViewBytes_[bufferViewIdx];
}

};  // namespace vkit::primitive

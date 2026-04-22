#pragma once

namespace vkit::primitive {

class Buffers {
 public:
  [[nodiscard]] auto operator()(std::size_t bufferViewIdx) const
      -> std::span<const std::byte>;

 private:
  std::unordered_map<std::size_t, std::vector<std::byte>> bufferBytes_;
  std::vector<std::span<const std::byte>> bufferViewBytes_;
};

};  // namespace vkit::primitive

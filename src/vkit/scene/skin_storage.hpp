#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <span>
#include <vector>

#include "vkit/scene/skin.hpp"

namespace vkit::scene {

class SkinStorage {
 public:
  SkinStorage() = default;

  std::vector<std::shared_ptr<Skin>> skins;

  void add(const std::shared_ptr<Skin>& skin) {
    skins.push_back(skin);
    reallocateAndDistributeSpans();
  }

  void remove(const std::shared_ptr<Skin>& skin) {
    std::erase(skins, skin);
    reallocateAndDistributeSpans();
  }

  [[nodiscard]] auto getData() const -> std::span<const std::byte> {
    return std::as_bytes(std::span{data_});
  }

 private:
  void reallocateAndDistributeSpans() {
    std::size_t total_matrices = 0;
    for (const auto& skin : skins) {
      total_matrices += skin->joints.size();
    }

    data_.resize(total_matrices);

    std::uint32_t current_offset = 0;
    for (const auto& skin : skins) {
      std::size_t count = skin->joints.size();

      if (count > 0) {
        std::span<glm::mat4> skin_span{data_.data() + current_offset, count};

        skin->setJointData(skin_span, current_offset);

        current_offset += count;
      }
    }
  }

  std::vector<glm::mat4> data_;
};

};  // namespace vkit::scene

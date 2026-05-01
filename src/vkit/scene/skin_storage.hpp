#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <mutex>
#include <span>
#include <unordered_map>
#include <vector>

#include "vkit/item/storage.hpp"
#include "vkit/scene/node.hpp"
#include "vkit/scene/skin.hpp"

namespace vkit::scene {

class SkinStorage : public Storage<Skin> {
 public:
  SkinStorage() = default;

  void update(std::span<const std::shared_ptr<Node>> activeNodes) {
    std::lock_guard<std::mutex> lock{mutex_};

    nodeOffsets_.clear();

    std::size_t total_matrices = 0;
    for (const auto& node : activeNodes) {
      if (node && node->skin) {
        total_matrices += node->skin->joints.size();
      }
    }

    data_.resize(total_matrices);

    std::uint32_t current_offset = 0;
    for (const auto& node : activeNodes) {
      if (node && node->skin) {
        std::size_t count = node->skin->joints.size();

        if (count > 0) {
          nodeOffsets_[node.get()] = current_offset;

          std::span<glm::mat4> skin_span{data_.data() + current_offset, count};
          node->skin->computeJointMatrices(skin_span);

          current_offset += count;
        }
      }
    }
  }

  [[nodiscard]] auto getData() const -> std::span<const glm::mat4> {
    std::lock_guard<std::mutex> lock{mutex_};
    return data_;
  }

  [[nodiscard]] auto getOffsetForNode(const Node* node) const -> std::uint32_t {
    std::lock_guard<std::mutex> lock{mutex_};
    if (auto it = nodeOffsets_.find(node); it != nodeOffsets_.end()) {
      return it->second;
    }
    return 0;
  }

 private:
  std::vector<glm::mat4> data_;

  std::unordered_map<const Node*, std::uint32_t> nodeOffsets_;
};

};  // namespace vkit::scene

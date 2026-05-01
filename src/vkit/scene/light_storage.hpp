#pragma once

#include <memory>
#include <mutex>
#include <span>
#include <vector>

#include "vkit/item/storage.hpp"
#include "vkit/scene/light.hpp"
#include "vkit/scene/node.hpp"

namespace vkit::scene {

class LightStorage : public Storage<Light> {
 public:
  LightStorage() = default;

  void update(std::span<const std::shared_ptr<Node>> activeNodes) {
    std::lock_guard<std::mutex> lock{mutex_};

    data_.clear();
    data_.reserve(lastLightCount_);

    for (const auto& node : activeNodes) {
      if (node && node->light) {
        data_.push_back(
            node->light->getData(node->getGlobalTransform().getMatrix()));
      }
    }

    lastLightCount_ = data_.size();
  }

  [[nodiscard]] auto getData() const -> std::span<const Light::Data> {
    std::lock_guard<std::mutex> lock{mutex_};
    return data_;
  }

 private:
  std::vector<Light::Data> data_;
  std::size_t lastLightCount_{0};
};

};  // namespace vkit::scene

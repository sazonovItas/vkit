#pragma once

#include <cstdint>
#include <unordered_map>

#include "vkit/asset/asset.hpp"
#include "vkit/scene/trs_transform.hpp"

namespace vkit::scene {
class Node;
}

namespace vkit::animation {

class Animator {
 public:
  Animator() = default;
  ~Animator() = default;

  // Advances time and applies bone transforms. Only call when skinning is active.
  void update(float dt, asset::Asset* currentAsset);

  // Call when skinning is first enabled — saves the asset's current (bind) pose.
  void snapshotBindPose(asset::Asset* asset);

  // Call when skinning is disabled — restores the saved bind pose and stops.
  void resetToBindPose(asset::Asset* asset);

  void setActiveAnimation(std::uint32_t index) {
    activeAnimationIndex_ = index;
  }

  void play() { isPlaying_ = true; }
  void pause() { isPlaying_ = false; }
  void togglePlayback() { isPlaying_ = !isPlaying_; }
  void stop() {
    isPlaying_ = false;
    totalTime_ = 0.0F;
  }

  [[nodiscard]] bool isPlaying() const { return isPlaying_; }

  void setTimeScale(float scale) { timeScale_ = scale; }
  [[nodiscard]] float getTimeScale() const { return timeScale_; }

 private:
  float totalTime_{0.0F};
  std::uint32_t activeAnimationIndex_{0};

  bool isPlaying_{false};
  float timeScale_{1.0F};

  std::unordered_map<scene::Node*, scene::TrsTransform> bindPose_;
};

};  // namespace vkit::animation

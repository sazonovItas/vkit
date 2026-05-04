#pragma once

#include <cstdint>

#include "vkit/asset/asset.hpp"

namespace vkit::animation {

class Animator {
 public:
  Animator() = default;
  ~Animator() = default;

  void update(float dt, asset::Asset* currentAsset);

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

  bool isPlaying_{true};
  float timeScale_{1.0F};
};

};  // namespace vkit::animation

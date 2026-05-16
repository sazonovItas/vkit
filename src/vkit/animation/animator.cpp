#include "vkit/animation/animator.hpp"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "vkit/scene/node.hpp"

namespace vkit::animation {

void Animator::update(float dt, asset::Asset* currentAsset) {
  if (!currentAsset || currentAsset->animations.empty()) return;
  if (!isPlaying_) return;

  totalTime_ += (dt * timeScale_);

  activeAnimationIndex_ =
      std::min(activeAnimationIndex_,
               static_cast<std::uint32_t>(currentAsset->animations.size() - 1));
  auto& anim = currentAsset->animations[activeAnimationIndex_];

  float duration = anim->end - anim->start;
  float current_time =
      (duration > 0.0F) ? std::fmod(totalTime_, duration) + anim->start : 0.0F;

  for (const auto& channel : anim->channels) {
    const auto& sampler = anim->samplers[channel.samplerIndex];
    if (sampler.inputs.empty() || !channel.targetNode) continue;

    std::size_t kf0 = 0;
    std::size_t kf1 = 0;

    for (std::size_t i = 0; i < sampler.inputs.size() - 1; ++i) {
      if (current_time >= sampler.inputs[i] &&
          current_time <= sampler.inputs[i + 1]) {
        kf0 = i;
        kf1 = i + 1;
        break;
      }
    }

    if (kf0 == kf1) {
      kf0 = sampler.inputs.size() - 1;
      kf1 = kf0;
    }

    float t = 0.0F;
    if (kf0 != kf1 &&
        sampler.interpolation != AnimationSampler::Interpolation::kStep) {
      float t0 = sampler.inputs[kf0];
      float t1 = sampler.inputs[kf1];
      t = (current_time - t0) / (t1 - t0);
    }

    auto local_transform = channel.targetNode->getLocalTransform();

    if (channel.path == AnimationChannel::Path::kTranslation) {
      glm::vec3 v0(sampler.outputs[kf0]);
      glm::vec3 v1(sampler.outputs[kf1]);
      local_transform.setTranslation(glm::mix(v0, v1, t));

    } else if (channel.path == AnimationChannel::Path::kRotation) {
      glm::quat q0(sampler.outputs[kf0].w, sampler.outputs[kf0].x,
                   sampler.outputs[kf0].y, sampler.outputs[kf0].z);
      glm::quat q1(sampler.outputs[kf1].w, sampler.outputs[kf1].x,
                   sampler.outputs[kf1].y, sampler.outputs[kf1].z);
      local_transform.setRotation(glm::normalize(glm::slerp(q0, q1, t)));

    } else if (channel.path == AnimationChannel::Path::kScale) {
      glm::vec3 s0(sampler.outputs[kf0]);
      glm::vec3 s1(sampler.outputs[kf1]);
      local_transform.setScale(glm::mix(s0, s1, t));
    }

    channel.targetNode->setLocalTransform(local_transform);
  }

  currentAsset->update();
}

void Animator::snapshotBindPose(asset::Asset* asset) {
  if (!asset) return;
  bindPose_.clear();
  for (const auto& anim : asset->animations) {
    if (!anim) continue;
    for (const auto& channel : anim->channels) {
      if (!channel.targetNode) continue;
      auto* node = channel.targetNode.get();
      if (bindPose_.find(node) == bindPose_.end())
        bindPose_[node] = node->getLocalTransform();
    }
  }
}

void Animator::resetToBindPose(asset::Asset* asset) {
  stop();
  if (!asset) return;
  for (auto& [node, transform] : bindPose_)
    node->setLocalTransform(transform);
  if (!bindPose_.empty())
    asset->update();
}

};  // namespace vkit::animation

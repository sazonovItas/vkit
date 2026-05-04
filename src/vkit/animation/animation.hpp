#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <vector>

#include "vkit/item/item.hpp"
#include "vkit/scene/node.hpp"

namespace vkit::animation {

struct AnimationSampler {
  enum class Interpolation { kLinear, kStep, kCubicSpline };

  Interpolation interpolation{Interpolation::kLinear};
  std::vector<float> inputs;

  std::vector<glm::vec4> outputs;
};

struct AnimationChannel {
  enum class Path { kTranslation, kRotation, kScale, kWeights };

  Path path;
  std::shared_ptr<scene::Node> targetNode;
  std::uint32_t samplerIndex;
};

class Animation : public Item<Animation> {
 public:
  explicit Animation(std::string_view name) : Item{name} {}
  ~Animation() override = default;

  std::vector<AnimationSampler> samplers;
  std::vector<AnimationChannel> channels;

  float start{std::numeric_limits<float>::max()};
  float end{std::numeric_limits<float>::min()};
};

};  // namespace vkit::animation

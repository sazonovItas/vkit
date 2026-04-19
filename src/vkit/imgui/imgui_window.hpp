#pragma once

#include <memory>

#include "vkit/graphics/texture.hpp"

namespace vkit::imgui {

class ImguiWindow {
 public:
  ImguiWindow();
  virtual ~ImguiWindow() noexcept;

  void setMinSize(float width, float height);
  void setMaxSize(float width, float height);
  auto begin() -> bool;
  void end();

  void drawImage(const std::shared_ptr<graphics::Texture>& texture, int width,
                 int height);

  virtual void imgui() = 0;
  virtual void onBegin();
  virtual void onEnd();

 protected:
  std::string title_;
  float minSize_[2]{200.0F, 100.0F};
  float maxSize_[2]{99999.0F, 99999.0F};

 private:
  bool isHovered_{false};
  bool isVisible_{true};
};

};  // namespace vkit::imgui

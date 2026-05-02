#pragma once

#include <imgui.h>

#include <functional>

#include "vkit/graph/pin.hpp"

namespace vkit::imgui::windows::ge {

class PinUI {
 public:
  static void DrawInput(graph::Pin* pin, const char* label, ImVec4 color,
                        const std::function<void()>& inlineUI = nullptr);

  static void DrawOutput(graph::Pin* pin, const char* label, ImVec4 color,
                         float nodeWidth);
};

};  // namespace vkit::imgui::windows::ge

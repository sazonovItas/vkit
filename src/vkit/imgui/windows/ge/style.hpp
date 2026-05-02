#pragma once

#include <imgui.h>

namespace vkit::imgui::windows::ge::style::colors {

constexpr ImVec4 kTextWarning{1.0F, 0.8F, 0.2F, 1.0F};
constexpr ImVec4 kTextError{1.0F, 0.3F, 0.3F, 1.0F};
constexpr ImVec4 kTextSuccess{0.0F, 1.0F, 0.0F, 1.0F};
constexpr ImVec4 kTextExecuting{1.0F, 1.0F, 0.0F, 1.0F};

constexpr ImVec4 kPinColorYellow{0.78F, 0.78F, 0.16F, 1.0F};
constexpr ImVec4 kPinColorCyan  {0.20F, 0.80F, 0.90F, 1.0F};
constexpr ImVec4 kPinBorder{0.0F, 0.0F, 0.0F, 1.0F};
constexpr ImVec4 kNodeBg{0.12F, 0.12F, 0.12F, 1.0F};

constexpr ImVec4 kStatusReady{0.2F, 0.8F, 0.2F, 1.0F};
constexpr ImVec4 kStatusStale{0.8F, 0.8F, 0.2F, 1.0F};
constexpr ImVec4 kStatusExecuting{0.0F, 0.5F, 1.0F, 1.0F};
constexpr ImVec4 kStatusError{0.8F, 0.2F, 0.2F, 1.0F};
constexpr ImVec4 kStatusDefault{0.5F, 0.5F, 0.5F, 1.0F};

};  // namespace vkit::imgui::windows::ge::style::colors

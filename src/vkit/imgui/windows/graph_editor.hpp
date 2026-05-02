#pragma once

#include <imgui.h>
#include <imgui_node_editor.h>

#include <optional>
#include <string_view>

#include "vkit/core/events/texture.hpp"
#include "vkit/imgui/imgui_window.hpp"
#include "vkit/item/storage.hpp"
#include "vkit/texture/texture.hpp"
#include "vkit/workflow/workflow.hpp"

namespace vkit::imgui::windows {

class GraphEditorWindow : public ImguiWindow {
 public:
  explicit GraphEditorWindow(std::string_view title);
  ~GraphEditorWindow() override;

  void setWorkflow(workflow::Workflow* wf);
  void setLoadBus(core::events::TextureLoadBus* bus);
  void setReadyBus(core::events::TextureReadyBus* bus);
  void setTextureStorage(Storage<texture::Texture>* storage);

  [[nodiscard]] auto selectedTextureImguiId() const -> std::optional<ImTextureID>;

  void onDraw() override;

 private:
  void drawWorkspace();
  void drawInspector();

  ax::NodeEditor::EditorContext* editor_ctx_{nullptr};

  workflow::Workflow* workflow_{nullptr};
  core::events::TextureLoadBus* load_bus_{nullptr};
  core::events::TextureReadyBus* ready_bus_{nullptr};
  Storage<texture::Texture>* storage_{nullptr};

  std::optional<ImTextureID> selected_texture_imgui_id_;
  char path_buf_[512]{};
};

};  // namespace vkit::imgui::windows

#include "vkit/imgui/windows/ge/graph_editor.hpp"

#include <imgui.h>

namespace vkit::imgui::windows::ge {

GraphEditorWindow::GraphEditorWindow(const std::string_view name)
    : ImguiWindow(name) {
  ImNodes::CreateContext();
  ImNodes::StyleColorsDark();

  auto& style = ImNodes::GetStyle();
  style.NodeCornerRounding = 4.0F;
  style.PinCircleRadius = 5.0F;
  style.LinkThickness = 6.0F;
}

GraphEditorWindow::~GraphEditorWindow() { ImNodes::DestroyContext(); }

void GraphEditorWindow::onDraw() {
  if (!controller_ || !controller_->getWorkflow()) {
    ImGui::TextDisabled("No Workflow attached to Graph Editor.");
    return;
  }

  auto* workflow = controller_->getWorkflow();

  bool open_create_menu = false;
  if (ImGui::Button("Add Node +")) {
    open_create_menu = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Recenter View")) {
    ImNodes::EditorContextResetPanning(ImVec2(0, 0));
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear Selection")) {
    ImNodes::ClearNodeSelection();
    ImNodes::ClearLinkSelection();
  }
  ImGui::Spacing();

  ImNodes::BeginNodeEditor();

  if (open_create_menu || (ImNodes::IsEditorHovered() &&
                           ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
    ImGui::OpenPopup("CreateNodePopup");
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0F, 8.0F));
  if (ImGui::BeginPopup("CreateNodePopup")) {
    ImGui::TextDisabled("Create Node");
    ImGui::Separator();

    ImVec2 spawn_pos = ImGui::GetMousePosOnOpeningCurrentPopup();
    registry_.drawCreationMenu(controller_, spawn_pos);

    ImGui::EndPopup();
  }
  ImGui::PopStyleVar();

  for (auto* node : workflow->getNodes()) {
    auto* wf_node = static_cast<workflow::WorkflowNode*>(node);
    if (auto* ui = registry_.getUI(wf_node)) {
      ui->drawCanvas(wf_node);
    }
  }

  for (auto& link_ptr : workflow->getLinks()) {
    auto* link = link_ptr.get();
    ImNodes::Link(link->getId(), link->getSrc()->getId(),
                  link->getSink()->getId());
  }

  ImNodes::EndNodeEditor();

  int hovered_pin_id;
  if (ImNodes::IsPinHovered(&hovered_pin_id)) {
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
      if (controller_ && controller_->getWorkflow()) {
        auto* workflow = controller_->getWorkflow();
        auto* pin = workflow->findPin(hovered_pin_id);

        if (pin) {
          auto links_to_delete = pin->getLinks();

          for (auto* link : links_to_delete) {
            controller_->disconnectLink(link->getId());
          }
        }
      }
    }
  }

  const int num_selected_nodes = ImNodes::NumSelectedNodes();
  if (num_selected_nodes > 0 && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
    std::vector<int> selected_nodes;
    selected_nodes.resize(num_selected_nodes);
    ImNodes::GetSelectedNodes(selected_nodes.data());

    if (controller_) {
      controller_->deleteNodes(selected_nodes);
    }

    ImNodes::ClearNodeSelection();
  }

  int start_pin_id;
  int end_pin_id;
  if (ImNodes::IsLinkCreated(&start_pin_id, &end_pin_id)) {
    auto* start_pin = workflow->findPin(start_pin_id);
    auto* end_pin = workflow->findPin(end_pin_id);
    if (start_pin && end_pin && workflow->canConnect(start_pin, end_pin)) {
      workflow->connect(start_pin, end_pin);
      workflow->markDirty();
    }
  }

  int destroyed_link_id;
  if (ImNodes::IsLinkDestroyed(&destroyed_link_id)) {
    if (auto* link = workflow->findLink(destroyed_link_id)) {
      workflow->disconnect(link);
      workflow->markDirty();
    }
  }
}

};  // namespace vkit::imgui::windows::ge

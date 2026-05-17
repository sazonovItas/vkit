#include "vkit/imgui/windows/ge/graph_editor.hpp"

#include <imgui.h>

#include "vkit/platform/file_dialog.hpp"
#include "vkit/workflow/serializer.hpp"

namespace vkit::imgui::windows::ge {

GraphEditorWindow::GraphEditorWindow(const std::string_view name)
    : ImguiWindow(name) {
  ImNodes::CreateContext();
  ImNodes::StyleColorsDark();

  auto& style = ImNodes::GetStyle();
  style.NodeCornerRounding    = 4.0F;
  style.NodePadding           = ImVec2(10.0F, 6.0F);
  style.NodeBorderThickness   = 1.0F;
  style.LinkThickness         = 2.5F;
  style.LinkLineSegmentsPerLength = 0.1F;
  style.LinkHoverDistance     = 8.0F;
  style.PinCircleRadius       = 4.5F;
  style.PinQuadSideLength     = 8.0F;
  style.PinTriangleSideLength = 10.0F;
  style.PinLineThickness      = 1.5F;
  style.PinHoverRadius        = 8.0F;
  style.PinOffset             = 0.0F;
  style.MiniMapPadding        = ImVec2(8.0F, 8.0F);
  style.MiniMapOffset         = ImVec2(4.0F, 4.0F);
  style.Flags = ImNodesStyleFlags_GridLines | ImNodesStyleFlags_GridLinesPrimary;

  auto col = [](float r, float g, float b, float a = 1.0F) -> unsigned int {
    return IM_COL32(static_cast<int>(r * 255), static_cast<int>(g * 255),
                    static_cast<int>(b * 255), static_cast<int>(a * 255));
  };
  unsigned int* c = style.Colors;
  // Grid
  c[ImNodesCol_GridBackground]         = col(0.055F, 0.055F, 0.055F);
  c[ImNodesCol_GridLine]               = col(0.095F, 0.095F, 0.095F);
  c[ImNodesCol_GridLinePrimary]        = col(0.070F, 0.070F, 0.070F);
  // Node
  c[ImNodesCol_NodeBackground]         = col(0.120F, 0.120F, 0.120F);
  c[ImNodesCol_NodeBackgroundHovered]  = col(0.145F, 0.145F, 0.145F);
  c[ImNodesCol_NodeBackgroundSelected] = col(0.145F, 0.145F, 0.145F);
  c[ImNodesCol_NodeOutline]            = col(0.040F, 0.040F, 0.040F);
  // Title bar
  c[ImNodesCol_TitleBar]               = col(0.085F, 0.085F, 0.085F);
  c[ImNodesCol_TitleBarHovered]        = col(0.110F, 0.110F, 0.110F);
  c[ImNodesCol_TitleBarSelected]       = col(0.122F, 0.498F, 0.769F);
  // Links
  c[ImNodesCol_Link]                   = col(0.350F, 0.350F, 0.350F);
  c[ImNodesCol_LinkHovered]            = col(0.122F, 0.498F, 0.769F);
  c[ImNodesCol_LinkSelected]           = col(0.122F, 0.498F, 0.769F);
  // Pins
  c[ImNodesCol_Pin]                    = col(0.350F, 0.350F, 0.350F);
  c[ImNodesCol_PinHovered]             = col(0.122F, 0.498F, 0.769F);
  // Box select
  c[ImNodesCol_BoxSelector]            = col(0.122F, 0.498F, 0.769F, 0.20F);
  c[ImNodesCol_BoxSelectorOutline]     = col(0.122F, 0.498F, 0.769F, 0.80F);
  // Mini-map
  c[ImNodesCol_MiniMapBackground]          = col(0.063F, 0.063F, 0.063F, 0.85F);
  c[ImNodesCol_MiniMapBackgroundHovered]   = col(0.063F, 0.063F, 0.063F, 0.98F);
  c[ImNodesCol_MiniMapOutline]             = col(0.040F, 0.040F, 0.040F);
  c[ImNodesCol_MiniMapOutlineHovered]      = col(0.122F, 0.498F, 0.769F);
  c[ImNodesCol_MiniMapNodeBackground]      = col(0.120F, 0.120F, 0.120F);
  c[ImNodesCol_MiniMapNodeBackgroundHovered]  = col(0.145F, 0.145F, 0.145F);
  c[ImNodesCol_MiniMapNodeBackgroundSelected] = col(0.122F, 0.498F, 0.769F, 0.80F);
  c[ImNodesCol_MiniMapNodeOutline]         = col(0.040F, 0.040F, 0.040F);
  c[ImNodesCol_MiniMapLink]                = col(0.350F, 0.350F, 0.350F);
  c[ImNodesCol_MiniMapLinkSelected]        = col(0.122F, 0.498F, 0.769F);
  c[ImNodesCol_MiniMapCanvas]              = col(0.110F, 0.110F, 0.110F, 0.80F);
  c[ImNodesCol_MiniMapCanvasOutline]       = col(0.040F, 0.040F, 0.040F);
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
  ImGui::SameLine();
  ImGui::Text("|");
  ImGui::SameLine();
  if (ImGui::Button("Export Graph")) {
    static const platform::FileFilter kFilters[] = {{"Material Graph", "mgraph"}};
    auto path = platform::saveFileDialog(kFilters, {}, "material");
    if (path) {
      if (path->extension() != ".mgraph") *path += ".mgraph";
      pendingExportPath_ = std::move(*path);
      pendingExport_ = true;
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Import Graph")) {
    static const platform::FileFilter kFilters[] = {{"Material Graph", "mgraph"}};
    auto path = platform::openFileDialog(kFilters);
    if (path) {
      pendingImportPath_ = std::move(*path);
      pendingImport_ = true;
    }
  }
  ImGui::Spacing();

  ImNodes::BeginNodeEditor();

  for (const auto& p : controller_->drainPendingPositions()) {
    ImNodes::SetNodeEditorSpacePos(p.nodeId, ImVec2{p.x, p.y});
  }

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

  // Deferred export: positions are now current after EndNodeEditor
  if (pendingExport_) {
    pendingExport_ = false;
    std::vector<controller::NodePosition> positions;
    for (auto* node : workflow->getNodes()) {
      auto pos = ImNodes::GetNodeEditorSpacePos(node->getId());
      positions.push_back({node->getId(), pos.x, pos.y});
    }
    workflow::WorkflowSerializer::exportToFile(workflow, positions,
                                               pendingExportPath_);
  }

  // Deferred import: clear selection so stale ImNodes IDs don't linger
  if (pendingImport_) {
    pendingImport_ = false;
    ImNodes::ClearNodeSelection();
    ImNodes::ClearLinkSelection();
    std::vector<controller::NodePosition> positions;
    workflow::WorkflowSerializer::importFromFile(pendingImportPath_, controller_,
                                                 positions);
    controller_->pushPendingPositions(std::move(positions));
  }

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

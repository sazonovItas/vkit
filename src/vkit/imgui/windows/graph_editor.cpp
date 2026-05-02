#include "vkit/imgui/windows/graph_editor.hpp"

#include <imgui.h>

#include <algorithm>
#include <filesystem>

#include "vkit/graph/link.hpp"
#include "vkit/workflow/node/texture_load.hpp"
#include "vkit/workflow/node_status.hpp"
#include "vkit/workflow/pin_type.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace ed = ax::NodeEditor;

namespace vkit::imgui::windows {

namespace {

auto pinColor(std::size_t key) -> ImVec4 {
  switch (static_cast<workflow::PinType>(key)) {
    case workflow::PinType::kColor:
      return {0.9F, 0.6F, 0.1F, 1.0F};
    default:
      return {0.7F, 0.7F, 0.7F, 1.0F};
  }
}

auto statusColor(workflow::NodeStatus s) -> ImVec4 {
  switch (s) {
    case workflow::NodeStatus::kStale:
      return {0.9F, 0.6F, 0.1F, 1.0F};
    case workflow::NodeStatus::kExecuting:
      return {0.3F, 0.7F, 1.0F, 1.0F};
    case workflow::NodeStatus::kReady:
      return {0.2F, 0.9F, 0.3F, 1.0F};
  }
  return {1, 1, 1, 1};
}

auto statusLabel(workflow::NodeStatus s) -> const char* {
  switch (s) {
    case workflow::NodeStatus::kStale:
      return "stale";
    case workflow::NodeStatus::kExecuting:
      return "loading";
    case workflow::NodeStatus::kReady:
      return "ready";
  }
  return "";
}

};  // namespace

GraphEditorWindow::GraphEditorWindow(std::string_view title)
    : ImguiWindow(title, true) {
  ed::Config cfg;
  cfg.SettingsFile = "graph_editor.json";
  editor_ctx_ = ed::CreateEditor(&cfg);
}

GraphEditorWindow::~GraphEditorWindow() {
  if (editor_ctx_) {
    ed::DestroyEditor(editor_ctx_);
    editor_ctx_ = nullptr;
  }
}

void GraphEditorWindow::setWorkflow(workflow::Workflow* wf) { workflow_ = wf; }
void GraphEditorWindow::setLoadBus(core::events::TextureLoadBus* bus) {
  load_bus_ = bus;
}
void GraphEditorWindow::setReadyBus(core::events::TextureReadyBus* bus) {
  ready_bus_ = bus;
}
void GraphEditorWindow::setTextureStorage(Storage<texture::Texture>* storage) {
  storage_ = storage;
}

auto GraphEditorWindow::selectedTextureImguiId() const
    -> std::optional<ImTextureID> {
  return selected_texture_imgui_id_;
}

void GraphEditorWindow::onDraw() {
  if (!workflow_) {
    ImGui::TextUnformatted("No workflow.");
    return;
  }
  ImGui::BeginChild("Inspector", ImVec2(240.0F, 0), ImGuiChildFlags_Borders);
  drawInspector();
  ImGui::EndChild();
  ImGui::SameLine();
  ImGui::BeginChild("Workspace", ImVec2(0, 0), ImGuiChildFlags_None);
  drawWorkspace();
  ImGui::EndChild();
}

void GraphEditorWindow::drawWorkspace() {
  ed::SetCurrentEditor(editor_ctx_);
  ed::Begin("Graph");

  for (auto* base_node : workflow_->getNodes()) {
    auto* wn = dynamic_cast<workflow::WorkflowNode*>(base_node);
    ed::BeginNode(base_node->getId());

    ImGui::TextUnformatted(base_node->getName().data());
    if (wn) {
      ImGui::SameLine();
      ImGui::TextColored(statusColor(wn->status()), "● %s",
                         statusLabel(wn->status()));
    }

    auto* tex_node = dynamic_cast<workflow::node::TextureLoadNode*>(base_node);
    if (tex_node && tex_node->outputTextureId.has_value() && storage_) {
      if (auto tex = storage_->get(*tex_node->outputTextureId)) {
        if (tex->getImguiId().has_value()) {
          ImGui::Image(*tex->getImguiId(), ImVec2(80.0F, 80.0F));
        }
      }
    }

    for (auto& pin : base_node->getInputs()) {
      ed::BeginPin(pin->getId(), ed::PinKind::Input);
      ImGui::PushStyleColor(ImGuiCol_Text, pinColor(pin->getKey()));
      ImGui::Text("-> %s", pin->getName().data());
      ImGui::PopStyleColor();
      ed::EndPin();
    }
    for (auto& pin : base_node->getOutputs()) {
      ed::BeginPin(pin->getId(), ed::PinKind::Output);
      ImGui::PushStyleColor(ImGuiCol_Text, pinColor(pin->getKey()));
      ImGui::Text("%s ->", pin->getName().data());
      ImGui::PopStyleColor();
      ed::EndPin();
    }
    ed::EndNode();
  }

  for (auto& link : workflow_->getLinks()) {
    ed::Link(link->getId(), link->getSrc()->getId(), link->getSink()->getId());
  }

  if (ed::BeginCreate()) {
    ed::PinId a_id;
    ed::PinId b_id;
    if (ed::QueryNewLink(&a_id, &b_id) && a_id && b_id) {
      auto* pin_a = workflow_->findPin(static_cast<int>(a_id.Get()));
      auto* pin_b = workflow_->findPin(static_cast<int>(b_id.Get()));
      if (pin_a && pin_b && pin_a->getOwnerNode() != pin_b->getOwnerNode()) {
        auto* src = pin_a->isSrc() ? pin_a : (pin_b->isSrc() ? pin_b : nullptr);
        auto* sink =
            pin_a->isSink() ? pin_a : (pin_b->isSink() ? pin_b : nullptr);
        if (src && sink && src->getKey() == sink->getKey()) {
          if (sink->getLinks().empty()) {
            if (ed::AcceptNewItem()) workflow_->connect(src, sink);
          } else {
            ed::RejectNewItem(ImVec4{1, 0.3F, 0.3F, 1});
          }
        } else {
          ed::RejectNewItem(ImVec4{1, 0.3F, 0.3F, 1});
        }
      } else {
        ed::RejectNewItem(ImVec4{1, 0.3F, 0.3F, 1});
      }
    }
  }
  ed::EndCreate();

  if (ed::BeginDelete()) {
    ed::LinkId link_id;
    while (ed::QueryDeletedLink(&link_id)) {
      if (ed::AcceptDeletedItem()) {
        if (auto* link = workflow_->findLink(static_cast<int>(link_id.Get())))
          workflow_->disconnect(link);
      }
    }
    ed::NodeId node_id;
    while (ed::QueryDeletedNode(&node_id)) {
      if (ed::AcceptDeletedItem())
        workflow_->destroyNode(static_cast<int>(node_id.Get()));
    }
  }
  ed::EndDelete();

  ed::Suspend();

  if (ed::ShowBackgroundContextMenu()) ImGui::OpenPopup("GraphCtx");
  if (ImGui::BeginPopup("GraphCtx")) {
    ImGui::TextDisabled("Add Node");
    ImGui::Separator();
    if (ImGui::MenuItem("Texture Load") && load_bus_ && ready_bus_ &&
        storage_) {
      workflow_->createNode<workflow::node::TextureLoadNode>(
          "Texture Load", *load_bus_, *ready_bus_, storage_);
    }
    ImGui::EndPopup();
  }
  ed::Resume();

  ed::End();
}

void GraphEditorWindow::drawInspector() {
  ImGui::TextUnformatted("Inspector");
  ImGui::Separator();

  selected_texture_imgui_id_ = std::nullopt;
  if (!workflow_) return;

  ed::SetCurrentEditor(editor_ctx_);
  ed::NodeId selected[1];
  if (ed::GetSelectedNodes(selected, 1) <= 0) {
    ImGui::TextWrapped(
        "Right-click canvas to add a node.\nSelect a node to configure it.");
    return;
  }

  int sel_id = static_cast<int>(selected[0].Get());
  auto& nodes = workflow_->getNodes();
  auto it = std::ranges::find_if(
      nodes, [sel_id](auto* n) { return n->getId() == sel_id; });
  if (it == nodes.end()) return;

  auto* tex_node = dynamic_cast<workflow::node::TextureLoadNode*>(*it);
  if (!tex_node) {
    ImGui::TextWrapped("No properties for this node type.");
    return;
  }

  auto* wn = static_cast<workflow::WorkflowNode*>(tex_node);
  ImGui::TextColored(statusColor(wn->status()), "● %s",
                     statusLabel(wn->status()));
  ImGui::Separator();

  static int last_selected_node_id = -1;
  if (last_selected_node_id != sel_id) {
    last_selected_node_id = sel_id;
    if (!tex_node->getPath().empty()) {
      auto path_str = tex_node->getPath().string();
      path_str.copy(path_buf_, sizeof(path_buf_) - 1);
      path_buf_[path_str.size()] = '\0';
    } else {
      path_buf_[0] = '\0';
    }
  }

  ImGui::TextUnformatted("Path:");
  ImGui::SetNextItemWidth(-1.0F);
  ImGui::InputText("##path", path_buf_, sizeof(path_buf_));

  bool exists = path_buf_[0] != '\0' &&
                std::filesystem::exists(std::filesystem::path{path_buf_});
  if (!exists) ImGui::BeginDisabled();
  if (ImGui::Button("Load Texture", ImVec2(-1.0F, 0.0F))) {
    tex_node->setPath(std::filesystem::path{path_buf_});
  }
  if (!exists) ImGui::EndDisabled();

  if (path_buf_[0] != '\0' && !exists) {
    ImGui::TextColored(ImVec4{1, 0.4F, 0.4F, 1}, "Path not found");
  }

  ImGui::Separator();
  if (!tex_node->outputTextureId.has_value() || !storage_) return;
  auto tex = storage_->get(*tex_node->outputTextureId);
  if (!tex || !tex->getImguiId().has_value()) return;

  selected_texture_imgui_id_ = tex->getImguiId();
  if (auto* gfx = tex->getGraphicsTexture().get()) {
    ImGui::Text("%d x %d   %d mip(s)", gfx->getWidth(), gfx->getHeight(),
                gfx->getLevelCount());
  }

  float side = ImGui::GetContentRegionAvail().x;
  if (side > 0.0F) {
    ImGui::Image(*tex->getImguiId(), ImVec2(side, side));
  }
}

};  // namespace vkit::imgui::windows

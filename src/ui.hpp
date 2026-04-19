#pragma once

#include <imgui_internal.h>

#include <algorithm>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ImGuizmo.h"
#include "app_types.hpp"
#include "dear_imgui.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "gltf/asset.hpp"
#include "imgui_node_editor.h"
#include "transform.hpp"

namespace ed = ax::NodeEditor;

namespace vkit {

enum class NodeType : uint32_t { kTexture = 1, kMaterial = 2, kPrimitive = 3 };
enum class PinKind : uint32_t { kNode = 0, kInput = 1, kOutput = 2 };

static const uint32_t kInvalidMaterial = ~0U;

static uintptr_t EncodeId(NodeType type, PinKind kind, uint32_t slot,
                          uint32_t index, uint32_t subIndex = 0) {
  uint32_t id = ((static_cast<uint32_t>(type) & 0x3) << 30) |
                ((static_cast<uint32_t>(kind) & 0x3) << 28) |
                ((slot & 0x3F) << 22) | ((subIndex & 0xFF) << 14) |
                (index & 0x3FFF);
  return static_cast<uintptr_t>(id);
}

static ed::NodeId MakeNodeId(NodeType type, uint32_t index,
                             uint32_t subIndex = 0) {
  return ed::NodeId(EncodeId(type, PinKind::kNode, 0, index, subIndex));
}

static ed::PinId MakePinId(NodeType type, PinKind kind, uint32_t slot,
                           uint32_t index, uint32_t subIndex = 0) {
  return ed::PinId(EncodeId(type, kind, slot, index, subIndex));
}

static ed::LinkId MakeLinkId(uintptr_t startPin, uintptr_t endPin) {
  uintptr_t hash = startPin;
  hash ^= endPin + 0x9e3779b9 + (hash << 6) + (hash >> 2);

  return ed::LinkId(hash);
}

static NodeType GetNodeType(uintptr_t id) {
  return static_cast<NodeType>((id >> 30) & 0x3);
}
static PinKind GetPinKind(uintptr_t id) {
  return static_cast<PinKind>((id >> 28) & 0x3);
}
static uint32_t GetPinSlot(uintptr_t id) {
  return static_cast<uint32_t>((id >> 22) & 0x3F);
}
static uint32_t GetSubIndex(uintptr_t id) {
  return static_cast<uint32_t>((id >> 14) & 0xFF);
}
static uint32_t GetIndex(uintptr_t id) {
  return static_cast<uint32_t>(id & 0x3FFF);
}

class UI : public DearImGui {
 public:
  explicit UI(const DearImGuiCreateInfo& imguiCreateInfo)
      : DearImGui(imguiCreateInfo) {
    createTextureResources(imguiCreateInfo.device);

    ed::Config config;
    config.SettingsFile = "MaterialGraph.json";
    nodeEditorContext_ = ed::CreateEditor(&config);
  }

  ~UI() {
    if (nodeEditorContext_) {
      ed::DestroyEditor(nodeEditorContext_);
    }
  }

  void newFrame() override {
    DearImGui::newFrame();
    io_.emplace(ImGui::GetIO());
    ImGuizmo::BeginFrame();
    ImGuizmo::SetRect(0, 0, io_->DisplaySize.x, io_->DisplaySize.y);
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
  }

  void endFrame() override {
    io_ = std::nullopt;
    DearImGui::endFrame();
  }

  void moveCamera(Camera& camera) {
    if (!io_->WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      camera.yaw = camera.yaw + (io_->MouseDelta.x * 0.5F);
      camera.yaw = camera.yaw < 0.0F
                       ? 360.0F - std::fmod(std::fabs(camera.yaw), 360.0F)
                       : std::fmod(camera.yaw, 360.0F);
      camera.pitch += io_->MouseDelta.y * 0.5F;
      camera.pitch = glm::clamp(camera.pitch, -89.0F, 89.0F);
    }
    if (!io_->WantCaptureMouse) {
      camera.distance -= io_->MouseWheel * 0.5F;
      camera.distance = glm::max(camera.distance, 0.1F);
    }
  }

  void drawInspect(Camera& camera, Transform& transform, UBOParams& uboParams,
                   std::vector<Light>& lights) {
    assert(io_ && "ImGui io shouldn't nullopt");

    ImGui::SetNextWindowSize({350.0F, 400.0F}, ImGuiCond_Once);
    if (ImGui::Begin("Inspect")) {
      if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3("Target", glm::value_ptr(camera.target), 0.1F);
        ImGui::DragFloat("Distance", &camera.distance, 0.1F, 0.1F, 100.0F);
        ImGui::SliderFloat("Yaw", &camera.yaw, 0.0F, 360.0F);
        ImGui::SliderFloat("Pitch", &camera.pitch, -89.0F, 89.0F);
        ImGui::TreePop();
      }

      if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3("Position", glm::value_ptr(transform.position), 0.1F);
        glm::vec3 euler_rotation =
            glm::degrees(glm::eulerAngles(transform.rotation));
        if (ImGui::DragFloat3("Rotation", glm::value_ptr(euler_rotation),
                              0.1F)) {
          transform.rotation = glm::quat(glm::radians(euler_rotation));
        }
        ImGui::DragFloat3("Scale", glm::value_ptr(transform.scale), 0.05F);
        ImGui::Separator();
        static bool show_gizmo = false;
        ImGui::Checkbox("Show Gizmo", &show_gizmo);

        if (show_gizmo) {
          static ImGuizmo::OPERATION current_op(ImGuizmo::TRANSLATE);
          if (ImGui::RadioButton("Translate",
                                 current_op == ImGuizmo::TRANSLATE))
            current_op = ImGuizmo::TRANSLATE;
          ImGui::SameLine();
          if (ImGui::RadioButton("Rotate", current_op == ImGuizmo::ROTATE))
            current_op = ImGuizmo::ROTATE;
          ImGui::SameLine();
          if (ImGui::RadioButton("Scale", current_op == ImGuizmo::SCALE))
            current_op = ImGuizmo::SCALE;

          glm::mat4 model = transform.modelMatrix();
          glm::mat4 gizmo_view =
              glm::lookAt(camera.getPosition(), camera.target, camera.up);
          glm::mat4 gizmo_proj = glm::perspective(
              glm::radians(90.0F), io_->DisplaySize.x / io_->DisplaySize.y,
              0.1F, 1000.0F);

          ImGuizmo::Manipulate(glm::value_ptr(gizmo_view),
                               glm::value_ptr(gizmo_proj), current_op,
                               ImGuizmo::LOCAL, glm::value_ptr(model));

          if (ImGuizmo::IsUsing()) {
            glm::vec3 new_pos;
            glm::vec3 new_scale;
            glm::vec3 new_euler;
            ImGuizmo::DecomposeMatrixToComponents(
                glm::value_ptr(model), glm::value_ptr(new_pos),
                glm::value_ptr(new_euler), glm::value_ptr(new_scale));
            transform.position = new_pos;
            transform.scale = new_scale;
            transform.rotation = glm::quat(glm::radians(new_euler));
          }
        }
        ImGui::TreePop();
      }

      ImGui::Separator();
      if (ImGui::TreeNodeEx("Params", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("Exposure", &uboParams.exposure, 0.1F, 0.1F, 100.0F);
        ImGui::DragFloat("Gamma", &uboParams.gamma, 0.1F, 0.1F, 100.0F);
        ImGui::DragFloat("IBL Intensity", &uboParams.iblIntensity, 0.1F, 0.1F,
                         100.0F);
        ImGui::TreePop();
      }

      ImGui::Separator();
      if (ImGui::TreeNodeEx("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
        static int new_light_type = 1;
        const char* light_type_names[] = {"Directional", "Point", "Spot"};
        ImGui::PushItemWidth(120.0F);
        ImGui::Combo("##NewLightType", &new_light_type, light_type_names,
                     IM_ARRAYSIZE(light_type_names));
        ImGui::PopItemWidth();
        ImGui::SameLine();

        if (ImGui::Button("Add Light")) {
          Light l{};
          l.type = static_cast<std::uint32_t>(new_light_type);
          l.color = glm::vec3(1.0F);
          l.intensity = 5.0F;
          if (new_light_type == 0)
            l.direction = glm::vec3(0.0F, -1.0F, -0.5F);
          else if (new_light_type == 1) {
            l.position = glm::vec3(0.0F, 2.0F, 0.0F);
            l.range = 20.0F;
          } else {
            l.position = glm::vec3(0.0F, 2.0F, 0.0F);
            l.direction = glm::vec3(0.0F, -1.0F, 0.0F);
            l.range = 20.0F;
            l.scaleOffset = glm::vec2(2.0F, -1.0F);
          }
          lights.push_back(l);
        }

        ImGui::Separator();
        for (size_t i = 0; i < lights.size(); ++i) {
          ImGui::PushID(static_cast<int>(i));
          char label[32];
          snprintf(label, sizeof(label), "%s Light %zu",
                   light_type_names[lights[i].type], i);
          if (ImGui::TreeNode(label)) {
            if (ImGui::Button("Delete Light")) {
              lights.erase(lights.begin() + i);
              ImGui::TreePop();
              ImGui::PopID();
              --i;
              continue;
            }
            int current_type = static_cast<int>(lights[i].type);
            if (ImGui::Combo("Type", &current_type, light_type_names,
                             IM_ARRAYSIZE(light_type_names)))
              lights[i].type = static_cast<std::uint32_t>(current_type);
            ImGui::ColorEdit3("Color", glm::value_ptr(lights[i].color));
            ImGui::DragFloat("Intensity", &lights[i].intensity, 0.1F, 0.0F,
                             1000.0F);
            if (0 == current_type)
              ImGui::DragFloat3("Direction",
                                glm::value_ptr(lights[i].direction), 0.05F);
            else if (current_type == 1) {
              ImGui::DragFloat3("Position", glm::value_ptr(lights[i].position),
                                0.1F);
              ImGui::DragFloat("Range", &lights[i].range, 0.5F, 0.0F, 1000.0F);
            } else if (current_type == 2) {
              ImGui::DragFloat3("Position", glm::value_ptr(lights[i].position),
                                0.1F);
              ImGui::DragFloat3("Direction",
                                glm::value_ptr(lights[i].direction), 0.05F);
              ImGui::DragFloat("Range", &lights[i].range, 0.5F, 0.0F, 1000.0F);
              ImGui::DragFloat2("Scale / Offset",
                                glm::value_ptr(lights[i].scaleOffset), 0.05F);
            }
            ImGui::TreePop();
          }
          ImGui::PopID();
        }
        ImGui::TreePop();
      }
    }
    ImGui::End();
  }

  void drawProceduralGenerator(ProceduralTextureParams& params) {
    ImGui::SetNextWindowSize({350.0F, 350.0F}, ImGuiCond_Once);
    if (ImGui::Begin("Procedural Generator")) {
      const char* patterns[] = {"Grid / Tiles", "Bricks"};
      ImGui::Combo("Pattern", &params.patternType, patterns,
                   IM_ARRAYSIZE(patterns));
      const char* modes[] = {"Color Map Only", "Normal Map Only", "Both"};
      ImGui::Combo("Output Mode", &params.generationMode, modes,
                   IM_ARRAYSIZE(modes));
      ImGui::SeparatorText("Dimensions");
      ImGui::DragInt("Width", &params.width, 1.0F, 64, 2048);
      ImGui::DragInt("Height", &params.height, 1.0F, 64, 2048);
      ImGui::DragFloat2("Tile Size", glm::value_ptr(params.tileSize), 1.0F,
                        4.0F, 512.0F);
      ImGui::DragFloat("Mortar", &params.mortarThickness, 0.1F, 0.0F, 32.0F);
      if (params.generationMode != 1) {
        ImGui::SeparatorText("Colors");
        ImGui::ColorEdit4("Tile Color", glm::value_ptr(params.brickColor));
        ImGui::ColorEdit4("Mortar Color", glm::value_ptr(params.mortarColor));
      }
      ImGui::Separator();
      if (ImGui::Button("Generate Texture", ImVec2(-1, 40)))
        params.triggerGeneration = true;
    }
    ImGui::End();
  }

  void drawViewManipulation(Camera& camera) {
    glm::vec3 cam_pos = camera.getPosition();
    glm::mat4 view = glm::lookAt(cam_pos, camera.target, camera.up);
    ImGuizmo::ViewManipulate(
        glm::value_ptr(view), camera.distance,
        ImVec2(io_->DisplaySize.x - 128, io_->DisplaySize.y - 128),
        ImVec2(128, 128), 0x00000000);
  }

  void drawGraphEditor(gltf::Asset& asset) {
    ImGui::Begin("Material Graph");
    ed::SetCurrentEditor(nodeEditorContext_);

    ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(45, 45, 45, 255));
    ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(25, 25, 25, 255));
    ed::PushStyleColor(ed::StyleColor_PinRect, ImColor(60, 60, 60, 0));
    ed::PushStyleVar(ed::StyleVar_NodeRounding, 6.0F);
    ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 1.5F);
    ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 8, 8, 8));

    ed::Begin("Node Editor");

    struct LinkData {
      ed::PinId startPin;
      ed::PinId endPin;
    };
    std::unordered_map<uintptr_t, LinkData> active_links;

    auto push_link = [&](ed::PinId outPin, ed::PinId inPin) {
      ed::LinkId link_id = MakeLinkId(outPin.Get(), inPin.Get());
      active_links[link_id.Get()] = {.startPin = outPin, .endPin = inPin};
      ed::Link(link_id, outPin, inPin);
    };

    auto init_node_pos = [&](ed::NodeId id, NodeType type, size_t idx) {
      if (!initializedNodes_.contains(id.Get())) {
        float x = 0.0F;
        float y = 0.0F;
        if (type == NodeType::kTexture) {
          x = 50.0F;
          y = static_cast<float>(idx) * 250.0F;
        } else if (type == NodeType::kMaterial) {
          x = 400.0F;
          y = static_cast<float>(idx) * 350.0F;
        } else if (type == NodeType::kPrimitive) {
          x = 800.0F;
          y = static_cast<float>(idx) * 150.0F;
        }
        ed::SetNodePosition(id, ImVec2(x, y));
        initializedNodes_.insert(id.Get());
      }
    };

    auto draw_header = [](ed::NodeId node_id, ImU32 color) {
      ImDrawList* draw_list = ed::GetNodeBackgroundDrawList(node_id);
      if (!draw_list) return;

      float fixed_header_height = 28.0F;
      ImVec2 node_min = ed::GetNodePosition(node_id);
      ImVec2 node_size = ed::GetNodeSize(node_id);

      ImVec2 header_max =
          ImVec2(node_min.x + node_size.x, node_min.y + fixed_header_height);
      draw_list->AddRectFilled(node_min, header_max, color,
                               ed::GetStyle().NodeRounding,
                               ImDrawFlags_RoundCornersTop);
      draw_list->AddLine(ImVec2(node_min.x, header_max.y), header_max,
                         IM_COL32(0, 0, 0, 150), 1.5F);
    };

    auto draw_pin_icon = [&](ed::PinId pin_id, ImU32 color, bool connected,
                             bool is_input) {
      ImVec2 hitbox_size(16.0F, 16.0F);
      ImVec2 cursor = ImGui::GetCursorScreenPos();

      if (is_input)
        cursor.x -= 4.0F;
      else
        cursor.x += 4.0F;

      cursor.y += (ImGui::GetTextLineHeight() * 0.5F) - (hitbox_size.y * 0.5F);
      ImRect rect(cursor, cursor + hitbox_size);

      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      bool is_hovered = (ed::GetHoveredPin() == pin_id);
      float radius = is_hovered ? 7.0F : 5.0F;
      ImU32 draw_color = is_hovered ? IM_COL32(255, 255, 255, 255) : color;

      if (connected) {
        draw_list->AddCircleFilled(rect.GetCenter(), radius, draw_color);
      } else {
        draw_list->AddCircle(rect.GetCenter(), radius, draw_color, 12, 2.0F);
      }
      ImGui::Dummy(hitbox_size);
    };

    auto draw_resize_grip = [&](ImVec2& size, ImVec2 cursor_start) {
      ImGui::Dummy(ImVec2(0, 4.0F));
      ImVec2 grip_size(14.0F, 14.0F);

      // Perfectly localized X position relative to this specific node!
      ImGui::SetCursorPosX(cursor_start.x + size.x - grip_size.x);

      ImGui::PushID("RESIZE_GRIP");
      ImGui::InvisibleButton("##grip", grip_size);

      if (ImGui::IsItemActive() &&
          ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        size.x += ImGui::GetIO().MouseDelta.x / ed::GetCurrentZoom();
        size.y += ImGui::GetIO().MouseDelta.y / ed::GetCurrentZoom();
        size.x = std::max(size.x, 160.0F);
        size.y = std::max(size.y, 50.0F);
      }

      bool hovered = ImGui::IsItemHovered();
      ImGui::PopID();

      if (hovered || ImGui::IsItemActive()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
      }

      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      ImVec2 p_max = ImGui::GetItemRectMax();
      ImU32 color = (hovered || ImGui::IsItemActive())
                        ? IM_COL32(200, 200, 200, 255)
                        : IM_COL32(100, 100, 100, 150);
      draw_list->AddTriangleFilled(ImVec2(p_max.x - 12, p_max.y),
                                   ImVec2(p_max.x, p_max.y),
                                   ImVec2(p_max.x, p_max.y - 12), color);
    };

    ImU32 col_color = IM_COL32(200, 200, 50, 255);
    ImU32 col_float = IM_COL32(160, 160, 160, 255);
    ImU32 col_normal = IM_COL32(120, 120, 200, 255);
    ImU32 col_shader = IM_COL32(50, 200, 50, 255);

    // --- 1. Draw Texture Nodes ---
    for (auto& [idx, tex] : asset.textures) {
      ImGui::PushID(static_cast<int>(idx + 100000));
      ed::NodeId node_id = MakeNodeId(NodeType::kTexture, idx);
      init_node_pos(node_id, NodeType::kTexture, idx);

      if (!nodeSizes_.contains(node_id.Get()))
        nodeSizes_[node_id.Get()] = ImVec2(160.0F, 160.0F);
      ImVec2& n_size = nodeSizes_[node_id.Get()];

      ed::BeginNode(node_id);

      ImVec2 cursor_start = ImGui::GetCursorPos();

      ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
      ImGui::Text("  Texture %zu", idx);
      ImGui::PopStyleColor();

      ImGui::Dummy(ImVec2(n_size.x, 6.0F));

      if (textureDescriptorSets_.contains(idx)) {
        auto tex_id = reinterpret_cast<ImTextureID>(
            static_cast<VkDescriptorSet>(*textureDescriptorSets_[idx]));
        ImGui::Image(tex_id, ImVec2(n_size.x, n_size.y));
      } else {
        ImGui::Dummy(ImVec2(n_size.x, n_size.y));
      }

      auto out_pin = MakePinId(NodeType::kTexture, PinKind::kOutput, 0, idx);
      float offset = n_size.x - ImGui::CalcTextSize("Color").x - 12.0F;

      // Anchor correctly using cursor_start
      ImGui::SetCursorPosX(cursor_start.x + offset);

      ImGui::Text("Color");
      ImGui::SameLine();
      ed::BeginPin(out_pin, ed::PinKind::Output);
      draw_pin_icon(out_pin, col_color, true, false);
      ed::EndPin();

      // Pass cursor_start to grip
      draw_resize_grip(n_size, cursor_start);

      ed::EndNode();
      draw_header(node_id, IM_COL32(180, 80, 20, 255));
      ImGui::PopID();
    }

    // --- 2. Draw Material Nodes ---
    for (auto& [idx, mat] : asset.materials) {
      ImGui::PushID(static_cast<int>(idx + 200000));
      ed::NodeId node_id = MakeNodeId(NodeType::kMaterial, idx);
      init_node_pos(node_id, NodeType::kMaterial, idx);

      if (!nodeSizes_.contains(node_id.Get()))
        nodeSizes_[node_id.Get()] = ImVec2(240.0F, 50.0F);
      ImVec2& n_size = nodeSizes_[node_id.Get()];

      ed::BeginNode(node_id);

      ImVec2 cursor_start = ImGui::GetCursorPos();
      float start_y = ImGui::GetCursorPosY();

      ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
      ImGui::Text("  Principled BSDF %zu", idx);
      ImGui::PopStyleColor();

      ImGui::Dummy(ImVec2(n_size.x, 6.0F));

      auto draw_mat_pin = [&](const char* label,
                              std::optional<uint32_t>& texOpt, int slotIdx,
                              ImU32 pinColor, auto uiFunc) {
        auto in_pin =
            MakePinId(NodeType::kMaterial, PinKind::kInput, slotIdx, idx);
        ed::BeginPin(in_pin, ed::PinKind::Input);
        draw_pin_icon(in_pin, pinColor, texOpt.has_value(), true);
        ed::EndPin();
        ImGui::SameLine();

        if (!texOpt.has_value()) {
          float widget_w =
              std::max(50.0F, n_size.x - ImGui::CalcTextSize(label).x - 40.0F);
          ImGui::PushItemWidth(widget_w);
          uiFunc();
          ImGui::PopItemWidth();
        } else {
          ImGui::TextColored(ImVec4(0.8F, 0.8F, 0.8F, 1.0F), "%s", label);
        }
      };

      draw_mat_pin("Base Color", mat.baseColorTexture, 1, col_color, [&]() {
        ImVec4 col(mat.baseColorFactor.r, mat.baseColorFactor.g,
                   mat.baseColorFactor.b, mat.baseColorFactor.a);
        float btn_w = std::max(
            30.0F, n_size.x - ImGui::CalcTextSize("Base Color").x - 40.0F);
        if (ImGui::ColorButton("##CB", col, 0, ImVec2(btn_w, 0))) {
          activeColorPickerMatIdx_ = idx;
        }
        ImGui::SameLine();
        ImGui::Text("Base Color");
      });

      draw_mat_pin("Metallic / Roughness", mat.metallicRoughnessTexture, 2,
                   col_float, [&]() {
                     ImGui::BeginGroup();
                     ImGui::DragFloat("Metallic", &mat.metallicFactor, 0.01F,
                                      0.F, 1.F);
                     ImGui::DragFloat("Roughness", &mat.roughnessFactor, 0.01F,
                                      0.F, 1.F);
                     ImGui::EndGroup();
                   });

      draw_mat_pin("Normal", mat.normalTexture, 3, col_normal,
                   [&]() { ImGui::Text("Normal"); });
      draw_mat_pin("Occlusion", mat.occlusionTexture, 4, col_float,
                   [&]() { ImGui::Text("Occlusion"); });
      draw_mat_pin("Emissive", mat.emissiveTexture, 5, col_color, [&]() {
        ImGui::DragFloat("Emissive", &mat.emissiveStrength, 0.01F, 0.F, 10.F);
      });

      float current_content_h = ImGui::GetCursorPosY() - start_y;
      if (n_size.y > current_content_h) {
        ImGui::Dummy(ImVec2(0, n_size.y - current_content_h));
      } else {
        ImGui::Dummy(ImVec2(0, 8.0F));
      }

      auto out_pin = MakePinId(NodeType::kMaterial, PinKind::kOutput, 6, idx);
      float offset = n_size.x - ImGui::CalcTextSize("BSDF").x - 8.0F;

      // Anchor correctly using cursor_start
      ImGui::SetCursorPosX(cursor_start.x + offset);

      ImGui::Text("BSDF");
      ImGui::SameLine();
      ed::BeginPin(out_pin, ed::PinKind::Output);
      draw_pin_icon(out_pin, col_shader, true, false);
      ed::EndPin();

      // Pass cursor_start to grip
      draw_resize_grip(n_size, cursor_start);

      ed::EndNode();
      draw_header(node_id, IM_COL32(40, 100, 50, 255));

      if (mat.baseColorTexture &&
          asset.textures.contains(*mat.baseColorTexture))
        push_link(MakePinId(NodeType::kTexture, PinKind::kOutput, 0,
                            *mat.baseColorTexture),
                  MakePinId(NodeType::kMaterial, PinKind::kInput, 1, idx));
      if (mat.metallicRoughnessTexture &&
          asset.textures.contains(*mat.metallicRoughnessTexture))
        push_link(MakePinId(NodeType::kTexture, PinKind::kOutput, 0,
                            *mat.metallicRoughnessTexture),
                  MakePinId(NodeType::kMaterial, PinKind::kInput, 2, idx));
      if (mat.normalTexture && asset.textures.contains(*mat.normalTexture))
        push_link(MakePinId(NodeType::kTexture, PinKind::kOutput, 0,
                            *mat.normalTexture),
                  MakePinId(NodeType::kMaterial, PinKind::kInput, 3, idx));
      if (mat.occlusionTexture &&
          asset.textures.contains(*mat.occlusionTexture))
        push_link(MakePinId(NodeType::kTexture, PinKind::kOutput, 0,
                            *mat.occlusionTexture),
                  MakePinId(NodeType::kMaterial, PinKind::kInput, 4, idx));
      if (mat.emissiveTexture && asset.textures.contains(*mat.emissiveTexture))
        push_link(MakePinId(NodeType::kTexture, PinKind::kOutput, 0,
                            *mat.emissiveTexture),
                  MakePinId(NodeType::kMaterial, PinKind::kInput, 5, idx));

      ImGui::PopID();
    }

    // --- 3. Draw Primitive Nodes ---
    for (auto& [meshIdx, mesh] : asset.meshes) {
      for (size_t p = 0; p < mesh->primitives.size(); p++) {
        ed::NodeId node_id = MakeNodeId(NodeType::kPrimitive, meshIdx, p);
        init_node_pos(node_id, NodeType::kPrimitive, meshIdx + p);

        ed::BeginNode(node_id);

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
        ImGui::Text("  Primitive %zu:%zu", meshIdx, p);
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(140.0F, 6.0F));

        auto in_pin =
            MakePinId(NodeType::kPrimitive, PinKind::kInput, 1, meshIdx, p);
        ed::BeginPin(in_pin, ed::PinKind::Input);

        bool has_material =
            (mesh->primitives[p].materialIdx != kInvalidMaterial) &&
            asset.materials.contains(mesh->primitives[p].materialIdx);
        draw_pin_icon(in_pin, col_shader, has_material, true);
        ed::EndPin();

        ImGui::SameLine();
        ImGui::Text("Surface");
        ed::EndNode();

        draw_header(node_id, IM_COL32(120, 40, 40, 255));

        if (has_material) {
          push_link(MakePinId(NodeType::kMaterial, PinKind::kOutput, 6,
                              mesh->primitives[p].materialIdx),
                    in_pin);
        }
      }
    }

    // --- 4. Handle Link Creation ---
    if (ed::BeginCreate(ImColor(255, 255, 255), 2.0F)) {
      ed::PinId start_pin_id = 0;
      ed::PinId end_pin_id = 0;
      if (ed::QueryNewLink(&start_pin_id, &end_pin_id) && start_pin_id &&
          end_pin_id) {
        uintptr_t s_id = start_pin_id.Get();
        uintptr_t e_id = end_pin_id.Get();

        if (GetPinKind(s_id) == PinKind::kInput &&
            GetPinKind(e_id) == PinKind::kOutput) {
          std::swap(s_id, e_id);
        }

        if (GetPinKind(s_id) == PinKind::kOutput &&
            GetPinKind(e_id) == PinKind::kInput) {
          NodeType s_type = GetNodeType(s_id);
          NodeType e_type = GetNodeType(e_id);

          bool valid =
              (s_type == NodeType::kTexture && e_type == NodeType::kMaterial) ||
              (s_type == NodeType::kMaterial && e_type == NodeType::kPrimitive);

          if (valid) {
            if (ed::AcceptNewItem(ImColor(100, 255, 100), 3.0F)) {
              if (s_type == NodeType::kTexture) {
                uint32_t tex_idx = GetIndex(s_id);
                auto& mat = asset.materials[GetIndex(e_id)];
                uint32_t slot = GetPinSlot(e_id);
                if (slot == 1)
                  mat.baseColorTexture = tex_idx;
                else if (slot == 2)
                  mat.metallicRoughnessTexture = tex_idx;
                else if (slot == 3)
                  mat.normalTexture = tex_idx;
                else if (slot == 4)
                  mat.occlusionTexture = tex_idx;
                else if (slot == 5)
                  mat.emissiveTexture = tex_idx;
              } else {
                asset.meshes[GetIndex(e_id)]
                    ->primitives[GetSubIndex(e_id)]
                    .materialIdx = GetIndex(s_id);
              }
            }
          } else {
            ed::RejectNewItem(ImColor(255, 100, 100), 2.0F);
          }
        } else {
          ed::RejectNewItem(ImColor(255, 100, 100), 2.0F);
        }
      }
    }
    ed::EndCreate();

    // --- 5. Handle Deletions ---
    if (ed::BeginDelete()) {
      ed::LinkId deleted_link_id = 0;
      while (ed::QueryDeletedLink(&deleted_link_id)) {
        if (ed::AcceptDeletedItem() &&
            active_links.contains(deleted_link_id.Get())) {
          uintptr_t in_pin_id =
              active_links[deleted_link_id.Get()].endPin.Get();
          NodeType type = GetNodeType(in_pin_id);

          if (type == NodeType::kMaterial) {
            auto& mat = asset.materials[GetIndex(in_pin_id)];
            uint32_t slot = GetPinSlot(in_pin_id);
            if (slot == 1)
              mat.baseColorTexture = std::nullopt;
            else if (slot == 2)
              mat.metallicRoughnessTexture = std::nullopt;
            else if (slot == 3)
              mat.normalTexture = std::nullopt;
            else if (slot == 4)
              mat.occlusionTexture = std::nullopt;
            else if (slot == 5)
              mat.emissiveTexture = std::nullopt;
          } else if (type == NodeType::kPrimitive) {
            asset.meshes[GetIndex(in_pin_id)]
                ->primitives[GetSubIndex(in_pin_id)]
                .materialIdx = kInvalidMaterial;
          }
        }
      }

      ed::NodeId deleted_node_id = 0;
      while (ed::QueryDeletedNode(&deleted_node_id)) {
        if (ed::AcceptDeletedItem()) {
          uintptr_t n_id = deleted_node_id.Get();
          NodeType type = GetNodeType(n_id);
          uint32_t idx = GetIndex(n_id);

          if (type == NodeType::kTexture) {
            asset.textures.erase(idx);
            for (auto& [mIdx, mat] : asset.materials) {
              if (mat.baseColorTexture == idx)
                mat.baseColorTexture = std::nullopt;
              if (mat.metallicRoughnessTexture == idx)
                mat.metallicRoughnessTexture = std::nullopt;
              if (mat.normalTexture == idx) mat.normalTexture = std::nullopt;
              if (mat.occlusionTexture == idx)
                mat.occlusionTexture = std::nullopt;
              if (mat.emissiveTexture == idx)
                mat.emissiveTexture = std::nullopt;
            }
          } else if (type == NodeType::kMaterial) {
            asset.materials.erase(idx);
            for (auto& [meshIdx, mesh] : asset.meshes) {
              for (auto& prim : mesh->primitives) {
                if (prim.materialIdx == idx)
                  prim.materialIdx = kInvalidMaterial;
              }
            }
          }
        }
      }
    }
    ed::EndDelete();

    // --- 6. Context Menus & Popups ---
    static ImVec2 new_node_position_canvas;

    ed::Suspend();

    if (activeColorPickerMatIdx_ &&
        asset.materials.contains(*activeColorPickerMatIdx_)) {
      ImGui::OpenPopup("MaterialColorPicker");
    }
    if (ImGui::BeginPopup("MaterialColorPicker")) {
      auto& mat = asset.materials[*activeColorPickerMatIdx_];
      ImGui::Text("Edit Base Color");
      ImGui::Separator();
      ImGui::ColorPicker4("##picker", glm::value_ptr(mat.baseColorFactor));
      ImGui::EndPopup();
    } else {
      activeColorPickerMatIdx_ = std::nullopt;
    }

    if (ed::ShowBackgroundContextMenu()) {
      ImGui::SetNextWindowPos(ImGui::GetMousePos());
      ImGui::OpenPopup("AddMenu");
      new_node_position_canvas = ed::ScreenToCanvas(ImGui::GetMousePos());
    }

    if (ImGui::BeginPopup("AddMenu")) {
      if (ImGui::MenuItem("Add New Material")) {
        size_t new_idx = 0;
        for (const auto& [i, mat] : asset.materials) {
          if (i >= new_idx) new_idx = i + 1;
        }

        auto& new_mat = asset.materials[new_idx];
        new_mat.baseColorFactor = {1.0F, 1.0F, 1.0F, 1.0F};
        new_mat.metallicFactor = 0.0F;
        new_mat.roughnessFactor = 0.5F;
        new_mat.alphaMode = fastgltf::AlphaMode::Opaque;

        ed::SetNodePosition(MakeNodeId(NodeType::kMaterial, new_idx),
                            new_node_position_canvas);
        initializedNodes_.insert(
            MakeNodeId(NodeType::kMaterial, new_idx).Get());
      }
      ImGui::EndPopup();
    }
    ed::Resume();

    ed::End();

    ed::PopStyleVar(3);
    ed::PopStyleColor(3);

    ed::SetCurrentEditor(nullptr);
    ImGui::End();
  }

  void uploadTextures(vk::Device device, gltf::Asset& asset) {
    for (auto& [idx, texture] : asset.textures) {
      if (textureDescriptorSets_.contains(idx)) continue;

      try {
        auto alloc_info = vk::DescriptorSetAllocateInfo{}
                              .setDescriptorPool(*descriptorPool)
                              .setDescriptorSetCount(1)
                              .setPSetLayouts(&*imguiTextureLayout_);

        auto set =
            std::move(device.allocateDescriptorSetsUnique(alloc_info)[0]);
        auto image_info = texture->descriptorInfo();
        image_info.setSampler(*previewSampler_);

        auto write =
            vk::WriteDescriptorSet{}
                .setDstSet(*set)
                .setDstBinding(0)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setImageInfo(image_info);

        device.updateDescriptorSets(write, nullptr);
        textureDescriptorSets_[idx] = std::move(set);
      } catch (const std::exception& e) {
        std::cerr << "[Vulkan Error] " << e.what() << "\n";
      }
    }
  }

 private:
  ed::PinId dragging_pin_id_ = 0;
  ed::PinId valid_target_pin_id_ = 0;
  ed::PinId invalid_target_pin_id_ = 0;

  std::optional<size_t> activeColorPickerMatIdx_;

  std::optional<ImGuiIO> io_;
  ed::EditorContext* nodeEditorContext_ = nullptr;

  vk::UniqueDescriptorSetLayout imguiTextureLayout_;
  std::unordered_map<std::uint32_t, vk::UniqueDescriptorSet>
      textureDescriptorSets_;
  vk::UniqueSampler previewSampler_;

  std::unordered_map<uintptr_t, ImVec2> nodeSizes_;
  std::unordered_set<uintptr_t> initializedNodes_;

  void createTextureResources(vk::Device device) {
    auto binding = vk::DescriptorSetLayoutBinding{
        0,
        vk::DescriptorType::eCombinedImageSampler,
        1,
        vk::ShaderStageFlagBits::eFragment,
    };
    auto layout_ci = vk::DescriptorSetLayoutCreateInfo{{}, binding};
    imguiTextureLayout_ = device.createDescriptorSetLayoutUnique(layout_ci);

    previewSampler_ = device.createSamplerUnique(vku::createSamplerCreateInfo(
        vk::SamplerAddressMode::eClampToEdge, vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear));
  }
};

};  // namespace vkit

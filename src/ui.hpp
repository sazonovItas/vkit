#pragma once

#include <utility>

#include "GraphEditor.h"
#include "ImGuizmo.h"
#include "app_types.hpp"
#include "dear_imgui.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "gltf/asset.hpp"
#include "transform.hpp"

namespace vkit {
class UI : public DearImGui {
 public:
  explicit UI(const DearImGuiCreateInfo& imguiCreateInfo)
      : DearImGui(imguiCreateInfo) {
    createTextureResources(imguiCreateInfo.device);
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

          if (new_light_type == 0) {
            l.direction = glm::vec3(0.0F, -1.0F, -0.5F);
          } else if (new_light_type == 1) {
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
                             IM_ARRAYSIZE(light_type_names))) {
              lights[i].type = static_cast<std::uint32_t>(current_type);
            }

            ImGui::ColorEdit3("Color", glm::value_ptr(lights[i].color));
            ImGui::DragFloat("Intensity", &lights[i].intensity, 0.1F, 0.0F,
                             1000.0F);

            if (0 == current_type) {
              ImGui::DragFloat3("Direction",
                                glm::value_ptr(lights[i].direction), 0.05F);
            } else if (current_type == 1) {
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

      if (params.generationMode != 1) {  // Hide colors if Normal Only
        ImGui::SeparatorText("Colors");
        ImGui::ColorEdit4("Tile Color", glm::value_ptr(params.brickColor));
        ImGui::ColorEdit4("Mortar Color", glm::value_ptr(params.mortarColor));
      }

      ImGui::Separator();
      if (ImGui::Button("Generate Texture", ImVec2(-1, 40))) {
        params.triggerGeneration = true;
      }
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
    struct AssetGraphDelegate : public GraphEditor::Delegate {
      gltf::Asset& asset;

      std::vector<GraphEditor::Node> nodes;
      std::vector<GraphEditor::Link> links;
      std::vector<GraphEditor::Template> templates;

      std::unordered_map<size_t, GraphEditor::NodeIndex> textureNodes;
      std::unordered_map<size_t, GraphEditor::NodeIndex> materialNodes;

      std::unordered_map<GraphEditor::NodeIndex, size_t> nodeToTexture;
      std::unordered_map<GraphEditor::NodeIndex, size_t> nodeToMaterial;

      std::unordered_map<GraphEditor::NodeIndex, std::pair<size_t, size_t>>
          nodeToPrimitive;

      std::vector<const char*> texOut = {"Color"};
      std::vector<const char*> matIn = {
          "Base Color", "Metallic/Roughness", "Normal", "Occlusion", "Emissive",
      };
      std::vector<const char*> matOut = {"Material"};
      std::vector<const char*> primIn = {"Material"};

      GraphEditor::NodeIndex contextNode = -1;
      bool openContextMenu = false;
      bool openBackgroundContextMenu = false;

      explicit AssetGraphDelegate(gltf::Asset& a, const UI& uiInstance)
          : asset(a), uiInstance_(uiInstance) {
        // Texture Template
        templates.push_back({
            IM_COL32(204, 102, 51, 255),
            IM_COL32(80, 80, 80, 255),
            IM_COL32(110, 110, 110, 255),
            0,
            nullptr,
            nullptr,
            1,
            texOut.data(),
            nullptr,
        });

        // Material Template
        templates.push_back({
            IM_COL32(76, 153, 102, 255),
            IM_COL32(80, 80, 80, 255),
            IM_COL32(110, 110, 110, 255),
            5,
            matIn.data(),
            nullptr,
            1,
            matOut.data(),
            nullptr,
        });

        // Primitive Template
        templates.push_back({
            IM_COL32(153, 51, 51, 255),
            IM_COL32(80, 80, 80, 255),
            IM_COL32(110, 110, 110, 255),
            1,
            primIn.data(),
            nullptr,
            0,
            nullptr,
            nullptr,
        });

        Rebuild();
      }

      ~AssetGraphDelegate() override {
        for (auto& n : nodes) {
          std::free(const_cast<char*>(n.mName));
        }
      }

      void Sync() {
        if (asset.textures.size() != textureNodes.size() ||
            asset.materials.size() != materialNodes.size()) {
          Rebuild();
        }
      }

      void Rebuild() {
        // 1. Cache existing node positions
        std::unordered_map<size_t, ImRect> old_tex;
        std::unordered_map<size_t, ImRect> old_mat;

        // Custom Hash for std::pair so unordered_map compiles!
        struct PairHash {
          std::size_t operator()(const std::pair<size_t, size_t>& p) const {
            return std::hash<size_t>{}(p.first) ^
                   (std::hash<size_t>{}(p.second) << 1);
          }
        };
        std::unordered_map<std::pair<size_t, size_t>, ImRect, PairHash>
            old_prim;

        for (auto& [n, idx] : nodeToTexture) old_tex[idx] = nodes[n].mRect;
        for (auto& [n, idx] : nodeToMaterial) old_mat[idx] = nodes[n].mRect;
        for (auto& [n, pair] : nodeToPrimitive) old_prim[pair] = nodes[n].mRect;

        // 2. Clear old state completely
        for (auto& n : nodes) std::free(const_cast<char*>(n.mName));
        nodes.clear();
        links.clear();
        textureNodes.clear();
        materialNodes.clear();
        nodeToTexture.clear();
        nodeToMaterial.clear();
        nodeToPrimitive.clear();

        int tex_row = 0;
        int mat_row = 0;
        int prim_row = 0;

        // 3. Rebuild Textures
        for (auto& [idx, tex] : asset.textures) {
          GraphEditor::NodeIndex node_index = nodes.size();
          GraphEditor::Node n{};
          n.mName = strdup(std::format("Texture {}", idx).c_str());
          n.mTemplateIndex = 0;

          if (old_tex.contains(idx)) {
            n.mRect = old_tex[idx];
          } else {
            float y = 50.F + (tex_row * 120.F);
            n.mRect = ImRect(ImVec2(50.F, y), ImVec2(230.F, y + 110.F));
          }

          nodes.push_back(n);
          textureNodes[idx] = node_index;
          nodeToTexture[node_index] = idx;
          tex_row++;
        }

        // 4. Rebuild Materials & Links
        for (auto& [idx, mat] : asset.materials) {
          GraphEditor::NodeIndex node_index = nodes.size();
          GraphEditor::Node n{};
          n.mName = strdup(std::format("Material {}", idx).c_str());
          n.mTemplateIndex = 1;

          if (old_mat.contains(idx)) {
            n.mRect = old_mat[idx];
          } else {
            float y = 50.F + (mat_row * 240.F);
            n.mRect = ImRect(ImVec2(350.F, y), ImVec2(530.F, y + 180.F));
          }

          nodes.push_back(n);
          materialNodes[idx] = node_index;
          nodeToMaterial[node_index] = idx;

          auto add_mat_tex_link = [&](std::optional<std::uint32_t> tex_opt,
                                      int input_slot) {
            if (tex_opt.has_value() && textureNodes.contains(*tex_opt)) {
              GraphEditor::Link l{};
              l.mInputNodeIndex = textureNodes[*tex_opt];
              l.mInputSlotIndex = 0;
              l.mOutputNodeIndex = node_index;
              l.mOutputSlotIndex = input_slot;
              links.push_back(l);
            }
          };

          add_mat_tex_link(mat.baseColorTexture, 0);
          add_mat_tex_link(mat.metallicRoughnessTexture, 1);
          add_mat_tex_link(mat.normalTexture, 2);
          add_mat_tex_link(mat.occlusionTexture, 3);
          add_mat_tex_link(mat.emissiveTexture, 4);

          mat_row++;
        }

        // 5. Rebuild Primitives & Links
        for (auto& [meshIdx, mesh] : asset.meshes) {
          for (size_t p = 0; p < mesh->primitives.size(); p++) {
            GraphEditor::NodeIndex node_index = nodes.size();
            GraphEditor::Node n{};
            n.mName =
                strdup(std::format("Primitive {}:{}", meshIdx, p).c_str());
            n.mTemplateIndex = 2;

            std::pair<size_t, size_t> key = {meshIdx, p};
            if (old_prim.contains(key)) {
              n.mRect = old_prim[key];
            } else {
              float y = 50.F + (prim_row * 120.F);
              n.mRect = ImRect(ImVec2(650.F, y), ImVec2(830.F, y + 110.F));
            }

            nodes.push_back(n);
            nodeToPrimitive[node_index] = key;

            auto& prim = mesh->primitives[p];
            if (materialNodes.contains(prim.materialIdx)) {
              GraphEditor::Link l{};
              l.mInputNodeIndex = materialNodes[prim.materialIdx];
              l.mInputSlotIndex = 0;
              l.mOutputNodeIndex = node_index;
              l.mOutputSlotIndex = 0;
              links.push_back(l);
            }
            prim_row++;
          }
        }
      }

      void AddMaterial() {
        size_t new_idx = 0;

        for (const auto& [idx, mat] : asset.materials) {
          if (idx >= new_idx) {
            new_idx = idx + 1;
          }
        }

        auto& new_mat = asset.materials[new_idx];
        new_mat.baseColorFactor = {1.0F, 1.0F, 1.0F, 1.0F};
        new_mat.metallicFactor = 0.0F;
        new_mat.roughnessFactor = 0.5F;
        new_mat.emissiveStrength = 0.0F;
        new_mat.dissolveStrength = 0.0F;
        new_mat.alphaMode = fastgltf::AlphaMode::Opaque;
        new_mat.alphaCutoff = 0.5F;

        Rebuild();
      }

      void DeleteNode(GraphEditor::NodeIndex index) {
        if (nodeToTexture.contains(index)) {
          size_t tex_idx = nodeToTexture[index];
          asset.textures.erase(tex_idx);

          for (auto& [m_idx, mat] : asset.materials) {
            if (mat.baseColorTexture == tex_idx)
              mat.baseColorTexture = std::nullopt;
            if (mat.metallicRoughnessTexture == tex_idx)
              mat.metallicRoughnessTexture = std::nullopt;
            if (mat.normalTexture == tex_idx) mat.normalTexture = std::nullopt;
            if (mat.occlusionTexture == tex_idx)
              mat.occlusionTexture = std::nullopt;
            if (mat.emissiveTexture == tex_idx)
              mat.emissiveTexture = std::nullopt;
          }
        } else if (nodeToMaterial.contains(index)) {
          size_t mat_idx = nodeToMaterial[index];
          asset.materials.erase(mat_idx);

          for (auto& [meshIdx, mesh] : asset.meshes) {
            for (auto& prim : mesh->primitives) {
              if (prim.materialIdx == mat_idx) {
                prim.materialIdx = 0;
              }
            }
          }
        }
        Rebuild();
      }

      void DeleteSelectedNodes() {
        bool changed = false;
        for (GraphEditor::NodeIndex i = 0; i < nodes.size(); ++i) {
          if (nodes[i].mSelected) {
            if (nodeToTexture.contains(i)) {
              size_t tex_idx = nodeToTexture[i];
              asset.textures.erase(tex_idx);
              for (auto& [m_idx, mat] : asset.materials) {
                if (mat.baseColorTexture == tex_idx)
                  mat.baseColorTexture = std::nullopt;
                if (mat.metallicRoughnessTexture == tex_idx)
                  mat.metallicRoughnessTexture = std::nullopt;
                if (mat.normalTexture == tex_idx)
                  mat.normalTexture = std::nullopt;
                if (mat.occlusionTexture == tex_idx)
                  mat.occlusionTexture = std::nullopt;
                if (mat.emissiveTexture == tex_idx)
                  mat.emissiveTexture = std::nullopt;
              }
              changed = true;
            } else if (nodeToMaterial.contains(i)) {
              size_t mat_idx = nodeToMaterial[i];
              asset.materials.erase(mat_idx);
              for (auto& [meshIdx, mesh] : asset.meshes) {
                for (auto& prim : mesh->primitives) {
                  if (prim.materialIdx == mat_idx) {
                    prim.materialIdx = 0;
                  }
                }
              }
              changed = true;
            }
          }
        }
        if (changed) Rebuild();
      }

      bool AllowedLink(GraphEditor::NodeIndex node1,
                       GraphEditor::NodeIndex node2) override {
        int t1 = nodes[node1].mTemplateIndex;
        int t2 = nodes[node2].mTemplateIndex;

        if ((t1 == 0 && t2 == 1) || (t1 == 1 && t2 == 0)) return true;
        if ((t1 == 1 && t2 == 2) || (t1 == 2 && t2 == 1)) return true;
        return false;
      }

      void SelectNode(GraphEditor::NodeIndex nodeIndex,
                      bool selected) override {
        nodes[nodeIndex].mSelected = selected;
      }

      void MoveSelectedNodes(const ImVec2 delta) override {
        for (auto& n : nodes) {
          if (n.mSelected) {
            n.mRect.Min.x += delta.x;
            n.mRect.Max.x += delta.x;
            n.mRect.Min.y += delta.y;
            n.mRect.Max.y += delta.y;
          }
        }
      }

      void AddLink(GraphEditor::NodeIndex sourceNode,
                   GraphEditor::SlotIndex sourceSlot,
                   GraphEditor::NodeIndex destNode,
                   GraphEditor::SlotIndex destSlot) override {
        GraphEditor::Link l{};
        l.mInputNodeIndex = sourceNode;
        l.mInputSlotIndex = sourceSlot;
        l.mOutputNodeIndex = destNode;
        l.mOutputSlotIndex = destSlot;

        if (nodeToTexture.contains(sourceNode) &&
            nodeToMaterial.contains(destNode)) {
          size_t tex_idx = nodeToTexture[sourceNode];
          size_t mat_idx = nodeToMaterial[destNode];
          switch (destSlot) {
            case 0:
              asset.materials[mat_idx].baseColorTexture = tex_idx;
              break;
            case 1:
              asset.materials[mat_idx].metallicRoughnessTexture = tex_idx;
              break;
            case 2:
              asset.materials[mat_idx].normalTexture = tex_idx;
              break;
            case 3:
              asset.materials[mat_idx].occlusionTexture = tex_idx;
              break;
            case 4:
              asset.materials[mat_idx].emissiveTexture = tex_idx;
              break;
            default:
              std::unreachable();
          }
        } else if (nodeToMaterial.contains(sourceNode) &&
                   nodeToPrimitive.contains(destNode)) {
          size_t mat_idx = nodeToMaterial[sourceNode];
          auto [meshIdx, primIdx] = nodeToPrimitive[destNode];

          asset.meshes[meshIdx]->primitives[primIdx].materialIdx =
              static_cast<uint32_t>(mat_idx);
        }

        links.push_back(l);
      }

      void DelLink(GraphEditor::LinkIndex index) override {
        auto& link = links[index];

        if (nodeToTexture.contains(link.mInputNodeIndex) &&
            nodeToMaterial.contains(link.mOutputNodeIndex)) {
          size_t mat_idx = nodeToMaterial[link.mOutputNodeIndex];
          switch (link.mOutputSlotIndex) {
            case 0:
              asset.materials[mat_idx].baseColorTexture = std::nullopt;
              break;
            case 1:
              asset.materials[mat_idx].metallicRoughnessTexture = std::nullopt;
              break;
            case 2:
              asset.materials[mat_idx].normalTexture = std::nullopt;
              break;
            case 3:
              asset.materials[mat_idx].occlusionTexture = std::nullopt;
              break;
            case 4:
              asset.materials[mat_idx].emissiveTexture = std::nullopt;
              break;
            default:
              std::unreachable();
          }
        } else if (nodeToMaterial.contains(link.mInputNodeIndex) &&
                   nodeToPrimitive.contains(link.mOutputNodeIndex)) {
          auto [meshIdx, primIdx] = nodeToPrimitive[link.mOutputNodeIndex];
          asset.meshes[meshIdx]->primitives[primIdx].materialIdx = 0;
        }

        links.erase(links.begin() + index);
      }

      void CustomDraw(ImDrawList* drawList, ImRect rect,
                      GraphEditor::NodeIndex nodeIndex) override {
        if (nodeToTexture.contains(nodeIndex)) {
          auto tex_idx = static_cast<uint32_t>(nodeToTexture[nodeIndex]);

          auto it = uiInstance_.textureDescriptorSets_.find(tex_idx);
          if (it != uiInstance_.textureDescriptorSets_.end()) {
            float padding = 10.0F;
            ImVec2 img_pos = ImVec2(rect.Min.x + padding, rect.Min.y + 40.0F);
            ImVec2 img_size = ImVec2(rect.GetWidth() - (padding * 2),
                                     rect.GetWidth() - (padding * 2));

            auto tex_id = reinterpret_cast<ImTextureID>(
                static_cast<VkDescriptorSet>(*it->second));

            drawList->AddImage(
                tex_id, img_pos,
                ImVec2(img_pos.x + img_size.x, img_pos.y + img_size.y));
          }
        }
      }

      void RightClick(GraphEditor::NodeIndex nodeIndex, GraphEditor::SlotIndex,
                      GraphEditor::SlotIndex) override {
        if (nodeIndex != static_cast<GraphEditor::NodeIndex>(-1)) {
          contextNode = nodeIndex;
          openContextMenu = true;
        } else {
          openBackgroundContextMenu = true;
        }
      }

      const size_t GetTemplateCount() override { return templates.size(); }
      const GraphEditor::Template GetTemplate(
          GraphEditor::TemplateIndex i) override {
        return templates[i];
      }
      const size_t GetNodeCount() override { return nodes.size(); }
      const GraphEditor::Node GetNode(GraphEditor::NodeIndex i) override {
        return nodes[i];
      }
      const size_t GetLinkCount() override { return links.size(); }
      const GraphEditor::Link GetLink(GraphEditor::LinkIndex i) override {
        return links[i];
      }

     private:
      const UI& uiInstance_;
    };

    static GraphEditor::ViewState view_state;
    static GraphEditor::Options options;
    static bool style_init = false;
    static std::unique_ptr<AssetGraphDelegate> delegate;

    if (!style_init) {
      options.mLineThickness = 3.0F;
      options.mNodeSlotRadius = 5.0F;
      options.mGridSize = 60.F;
      options.mDisplayLinksAsCurves = true;
      options.mRounding = 6.0F;
      style_init = true;
    }

    if (!delegate)
      delegate = std::make_unique<AssetGraphDelegate>(asset, *this);

    // Sync graph with the asset data if anything has changed externally
    delegate->Sync();

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(30, 30, 30, 255));
    ImGui::Begin("Asset Graph");
    GraphEditor::Show(*delegate, options, view_state, true);

    // Keyboard Delete Hook
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) &&
        ImGui::IsKeyPressed(ImGuiKey_Delete)) {
      delegate->DeleteSelectedNodes();
    }

    // Node Context Menu Trigger
    if (delegate->openContextMenu) {
      ImGui::OpenPopup("NodeContext");
      delegate->openContextMenu = false;
    }

    // Background Context Menu Trigger
    if (delegate->openBackgroundContextMenu) {
      ImGui::OpenPopup("BackgroundContext");
      delegate->openBackgroundContextMenu = false;
    }

    // Render Node Right-Click Menu
    if (ImGui::BeginPopup("NodeContext")) {
      if (ImGui::MenuItem("Delete Node")) {
        delegate->DeleteNode(delegate->contextNode);
      }
      ImGui::EndPopup();
    }

    // Render Background Right-Click Menu
    if (ImGui::BeginPopup("BackgroundContext")) {
      if (ImGui::MenuItem("Add New Material")) {
        delegate->AddMaterial();
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Delete Selected Nodes")) {
        delegate->DeleteSelectedNodes();
      }

      ImGui::EndPopup();
    }

    ImGui::End();
    ImGui::PopStyleColor();

    ImGui::Begin("Material Inspector");
    for (auto& [idx, mat] : asset.materials) {
      ImGui::PushID(static_cast<int>(idx));
      if (ImGui::TreeNode(std::format("Material {}", idx).c_str())) {
        ImGui::ColorEdit4("BaseColor", glm::value_ptr(mat.baseColorFactor));
        ImGui::DragFloat("Metallic", &mat.metallicFactor, 0.01F, 0.F, 1.F);
        ImGui::DragFloat("Roughness", &mat.roughnessFactor, 0.01F, 0.F, 1.F);
        ImGui::DragFloat("EmissiveStrength", &mat.emissiveStrength, 0.01F, 0.F,
                         10.F);
        ImGui::DragFloat("DissolveStrength", &mat.dissolveStrength, 0.01F, 0.F,
                         1.F);

        static const char* alpha_modes[] = {"Opaque", "Mask", "Blend"};
        int alpha_mode_index = static_cast<int>(mat.alphaMode);
        if (ImGui::Combo("Alpha Mode", &alpha_mode_index, alpha_modes,
                         IM_ARRAYSIZE(alpha_modes))) {
          mat.alphaMode = static_cast<fastgltf::AlphaMode>(alpha_mode_index);
        }
        if (mat.alphaMode == fastgltf::AlphaMode::Mask) {
          ImGui::DragFloat("Alpha Cutoff", &mat.alphaCutoff, 0.01F, 0.0F, 1.0F);
        }
        ImGui::TreePop();
      }
      ImGui::PopID();
    }
    ImGui::End();
  }

  void uploadTextures(vk::Device device, gltf::Asset& asset) {
    for (auto& [idx, texture] : asset.textures) {
      if (textureDescriptorSets_.contains(idx)) continue;

      auto alloc_info = vk::DescriptorSetAllocateInfo{}
                            .setDescriptorPool(*descriptorPool)
                            .setDescriptorSetCount(1)
                            .setPSetLayouts(&*imguiTextureLayout_);

      auto set = std::move(device.allocateDescriptorSetsUnique(alloc_info)[0]);

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
    }
  }

 private:
  std::optional<ImGuiIO> io_;

  vk::UniqueDescriptorSetLayout imguiTextureLayout_;
  std::unordered_map<std::uint32_t, vk::UniqueDescriptorSet>
      textureDescriptorSets_;
  vk::UniqueSampler previewSampler_;

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

#pragma once

#include <atomic>
#include <filesystem>
#include <optional>
#include <string_view>

#include "vkit/core/events/texture.hpp"
#include "vkit/message_bus/message_bus.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::workflow::node {

class TextureLoadNode : public WorkflowNode {
 public:
  TextureLoadNode(std::string_view name, core::events::TextureLoadBus& loadBus,
                  core::events::TextureReadyBus& readyBus,
                  texture::TextureManager& textureManager);

  ~TextureLoadNode() override;

  void execute() override;

  void setPath(std::filesystem::path path);
  [[nodiscard]] auto getPath() const -> const std::filesystem::path&;
  [[nodiscard]] auto isLoaded() const -> bool;

  std::optional<std::uint32_t> outputTextureId;

 private:
  core::events::TextureLoadBus& loadBus_;
  texture::TextureManager& textureManager_;

  message_bus::Subscription<core::events::TextureReadyEvent> readySub_;

  std::filesystem::path path_;
  std::uint64_t pendingRequestId_{0};
  bool loaded_{false};

  graph::Pin* outColor_{nullptr};

  static std::atomic<std::uint64_t> request_counter_;
};

};  // namespace vkit::workflow::node

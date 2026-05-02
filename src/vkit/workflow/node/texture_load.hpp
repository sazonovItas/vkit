#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>

#include "vkit/core/events/texture.hpp"
#include "vkit/graph/pin.hpp"
#include "vkit/item/storage.hpp"
#include "vkit/texture/texture.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::workflow::node {

class TextureLoadNode : public WorkflowNode {
 public:
  TextureLoadNode(std::string_view name, core::events::TextureLoadBus& loadBus,
                  core::events::TextureReadyBus& readyBus,
                  Storage<texture::Texture>* storage);

  ~TextureLoadNode() override = default;

  void execute() override;

  void setPath(std::filesystem::path path);
  [[nodiscard]] auto getPath() const -> const std::filesystem::path&;
  [[nodiscard]] auto isLoaded() const -> bool;

  std::optional<std::uint32_t> outputTextureId;

 private:
  core::events::TextureLoadBus& loadBus_;
  Storage<texture::Texture>* storage_{nullptr};

  message_bus::Subscription<core::events::TextureReadyEvent> readySub_;

  std::filesystem::path path_;
  std::uint64_t pendingRequestId_{0};
  bool loaded_{false};

  graph::Pin* outColor_{nullptr};

  static std::atomic<std::uint64_t> request_counter_;
};

};  // namespace vkit::workflow::node

#include "vkit/workflow/node/texture_load.hpp"

#include "vkit/texture/loader.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node {

std::atomic<std::uint64_t> TextureLoadNode::request_counter_{0};

TextureLoadNode::TextureLoadNode(std::string_view name,
                                 core::events::TextureLoadBus& loadBus,
                                 core::events::TextureReadyBus& readyBus,
                                 texture::TextureManager& textureManager)
    : WorkflowNode(name),
      loadBus_(loadBus),
      textureManager_(textureManager),
      readySub_{readyBus.subscribe([this](core::events::TextureReadyEvent& ev) {
        if (ev.requestId != pendingRequestId_) return;

        if (!ev.texture) {
          pendingRequestId_ = 0;
          setStatus(NodeStatus::kError);
          return;
        }

        if (auto id = ev.texture->getStorageId()) {
          outputTextureId = id;
          loaded_ = true;
          pendingRequestId_ = 0;
          setStatus(NodeStatus::kReady);
          propagateStale();
        }
      })} {
  outColor_ = addOutputPin(pinKeyType(PinType::kColorTexture2D), "Texture");
}

TextureLoadNode::~TextureLoadNode() {
  if (outputTextureId.has_value()) {
    textureManager_.remove(outputTextureId.value());
  }
}

void TextureLoadNode::execute() {
  if (status() == NodeStatus::kError) return;

  if (loaded_ || path_.empty() || pendingRequestId_ != 0) return;

  pendingRequestId_ = ++request_counter_;
  setStatus(NodeStatus::kExecuting);

  loadBus_.queueMessage(core::events::TextureLoadRequest{
      .path = path_,
      .options = texture::LoadOptions{.useMipmaps = true},
      .requestId = pendingRequestId_,
  });
}

void TextureLoadNode::setPath(std::filesystem::path path) {
  if (path_ == path) return;
  path_ = std::move(path);
  loaded_ = false;
  pendingRequestId_ = 0;

  if (outputTextureId.has_value()) {
    textureManager_.remove(outputTextureId.value());
  }
  outputTextureId = std::nullopt;

  std::error_code ec;
  if (!path_.empty() && !std::filesystem::exists(path_, ec)) {
    setStatus(NodeStatus::kError);
    propagateStale();
    return;
  }

  markStale();
}

auto TextureLoadNode::getPath() const -> const std::filesystem::path& {
  return path_;
}

auto TextureLoadNode::isLoaded() const -> bool { return loaded_; }

};  // namespace vkit::workflow::node

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

        if (!ev.color || !ev.image) {
          pendingRequestId_ = 0;
          setStatus(NodeStatus::kError);
          return;
        }

        outputColorId = textureManager_.add(ev.color);
        outputF32Id = textureManager_.add(ev.image);

        if (outputColorId) outColor_->setData<std::uint32_t>(*outputColorId);
        if (outputF32Id) outImageF32_->setData<std::uint32_t>(*outputF32Id);

        loaded_ = true;
        pendingRequestId_ = 0;
        setStatus(NodeStatus::kReady);
        propagateStale();
      })} {
  outColor_ = addOutputPin(pinKeyType(PinType::kColorTexture2D), "Color");
  outImageF32_ = addOutputPin(pinKeyType(PinType::kFloatTexture2D), "Image");
}

TextureLoadNode::~TextureLoadNode() {
  if (outputColorId.has_value()) textureManager_.remove(outputColorId.value());
  if (outputF32Id.has_value()) textureManager_.remove(outputF32Id.value());
}

void TextureLoadNode::execute() {
  if (status() == NodeStatus::kError) return;

  if (loaded_ || path_.empty() || pendingRequestId_ != 0) return;

  pendingRequestId_ = ++request_counter_;
  setStatus(NodeStatus::kExecuting);

  loadBus_.queueMessage(core::events::TextureLoadRequest{
      .path = path_,
      .options = texture::LoadOptions{.useMipmaps = useMipmaps_},
      .requestId = pendingRequestId_,
  });
}

void TextureLoadNode::setPath(std::filesystem::path path) {
  if (path_ == path) return;
  path_ = std::move(path);
  loaded_ = false;
  pendingRequestId_ = 0;

  if (outputColorId.has_value()) textureManager_.remove(outputColorId.value());
  if (outputF32Id.has_value()) textureManager_.remove(outputF32Id.value());

  outputColorId = std::nullopt;
  outputF32Id = std::nullopt;

  // Wipe pins clean
  if (outColor_) outColor_->clearData();
  if (outImageF32_) outImageF32_->clearData();

  std::error_code ec;
  if (!path_.empty() && !std::filesystem::exists(path_, ec)) {
    setStatus(NodeStatus::kError);
    propagateStale();
    return;
  }

  markStale();
}

void TextureLoadNode::setUseMipmaps(bool useMipmaps) {
  if (useMipmaps_ == useMipmaps) return;
  useMipmaps_ = useMipmaps;

  loaded_ = false;
  pendingRequestId_ = 0;

  if (outputColorId.has_value()) textureManager_.remove(outputColorId.value());
  if (outputF32Id.has_value()) textureManager_.remove(outputF32Id.value());

  outputColorId = std::nullopt;
  outputF32Id = std::nullopt;

  // Wipe pins clean
  if (outColor_) outColor_->clearData();
  if (outImageF32_) outImageF32_->clearData();

  markStale();
}

auto TextureLoadNode::getPath() const -> const std::filesystem::path& {
  return path_;
}

auto TextureLoadNode::getUseMipmaps() const -> bool { return useMipmaps_; }

auto TextureLoadNode::isLoaded() const -> bool { return loaded_; }

};  // namespace vkit::workflow::node

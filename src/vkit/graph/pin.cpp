#include "vkit/graph/pin.hpp"

#include <algorithm>

#include "vkit/graph/graph.hpp"
#include "vkit/graph/link.hpp"

namespace vkit::graph {

Pin::Pin(Node* ownerNode, std::size_t slot, bool isSrc, std::size_t key,
         std::string_view name)
    : id_{makeGraphId()},
      key_{key},
      ownerNode_{ownerNode},
      slot_{slot},
      isSrc_{isSrc},
      name_{name} {}

Pin::~Pin() noexcept = default;

auto Pin::isSrc() const -> bool { return isSrc_; }

auto Pin::isSink() const -> bool { return !isSrc_; }

auto Pin::getId() const -> int { return id_; }

auto Pin::getKind() const -> PinKind {
  return isSrc_ ? PinKind::kOutput : PinKind::kInput;
}

auto Pin::getKey() const -> std::size_t { return key_; }

auto Pin::getName() const -> std::string_view { return name_; }

void Pin::addLink(Link* link) { links_.push_back(link); }

void Pin::removeLink(Link* link) {
  auto i = std::ranges::find_if(links_,
                                [link](Link* entry) { return entry == link; });
  if (i != links_.end()) {
    links_.erase(i);
  }
}

auto Pin::getLinks() const -> const std::vector<Link*>& { return links_; }

auto Pin::getLinks() -> std::vector<Link*>& { return links_; }

auto Pin::getOwnerNode() const -> Node* { return ownerNode_; }

auto Pin::getSlot() const -> std::size_t { return slot_; }

void Pin::setOwnerNode(Node* node) { ownerNode_ = node; }

};  // namespace vkit::graph

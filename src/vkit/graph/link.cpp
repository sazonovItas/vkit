#include "vkit/graph/link.hpp"

#include "vkit/graph/graph.hpp"
#include "vkit/graph/pin.hpp"

namespace vkit::graph {

Link::Link() : id_{makeGraphId()} {}

Link::Link(Link&& old) noexcept = default;

Link& Link::operator=(Link&& old) noexcept = default;

Link::Link(Pin* src, Pin* sink) : id_{makeGraphId()}, src_{src}, sink_{sink} {}

Link::~Link() noexcept = default;

auto Link::getId() const -> int { return id_; }

auto Link::getSrc() const -> Pin* { return src_; }

auto Link::getSink() const -> Pin* { return sink_; }

auto Link::isConnected() const -> bool { return (src_ != nullptr) && (sink_ != nullptr); }

void Link::disconnect() {
  src_->removeLink(this);
  sink_->removeLink(this);
  src_ = nullptr;
  sink_ = nullptr;
}

};  // namespace vkit::graph

#include "vkit/graph/graph.hpp"

#include <algorithm>

#include "vkit/graph/link.hpp"
#include "vkit/graph/node.hpp"
#include "vkit/graph/pin.hpp"

namespace vkit::graph {

void Graph::registerNode(Node* node) {
  if (node != nullptr) {
    nodes.push_back(node);
  }
}

void Graph::unregisterNode(Node* node) {
  auto it = std::ranges::find(nodes, node);
  if (it != nodes.end()) {
    nodes.erase(it);
  }
}

auto Graph::connect(Pin* src, Pin* sink) -> Link* {
  if (src == nullptr || sink == nullptr) {
    return nullptr;
  }

  auto link = std::make_unique<Link>(src, sink);
  Link* link_ptr = link.get();

  src->addLink(link_ptr);
  sink->addLink(link_ptr);

  links.push_back(std::move(link));

  return link_ptr;
}

void Graph::disconnect(Link* link) {
  if (link == nullptr) return;

  link->disconnect();

  auto it = std::ranges::find_if(links, [link](const std::unique_ptr<Link>& l) {
    return l.get() == link;
  });

  if (it != links.end()) {
    links.erase(it);
  }
}

auto Graph::getNodes() const -> const std::vector<Node*>& { return nodes; }

auto Graph::getNodes() -> std::vector<Node*>& { return nodes; }

auto Graph::getLinks() -> std::vector<std::unique_ptr<Link>>& { return links; }

};  // namespace vkit::graph

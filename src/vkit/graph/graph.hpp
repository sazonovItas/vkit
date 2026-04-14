#pragma once

#include <atomic>
#include <memory>
#include <vector>

namespace vkit::graph {

class Node;
class Pin;
class Link;
class Graph;

[[nodiscard]] inline auto makeGraphId() -> int {
  static std::atomic<int> counter{1};
  return counter++;
}

class Graph {
 public:
  void registerNode(Node* node);
  void unregisterNode(Node* node);
  auto connect(Pin* src, Pin* sink) -> Link*;
  void disconnect(Link* link);

  [[nodiscard]] auto getNodes() const -> const std::vector<Node*>&;
  [[nodiscard]] auto getNodes() -> std::vector<Node*>&;
  [[nodiscard]] auto getLinks() -> std::vector<std::unique_ptr<Link>>&;

  std::vector<Node*> nodes;
  std::vector<std::unique_ptr<Link>> links;
};

};  // namespace vkit::graph

#pragma once

#include <atomic>
#include <memory>
#include <vector>

namespace vkit::graph {

class Node;
class Pin;
class Link;

[[nodiscard]] inline auto makeGraphId() -> int {
  static std::atomic<int> counter{1};
  return counter++;
}

class Graph {
 public:
  virtual void registerNode(Node* node);
  virtual void unregisterNode(Node* node);
  virtual auto connect(Pin* src, Pin* sink) -> Link*;
  virtual void disconnect(Link* link);

  [[nodiscard]] auto getNodes() const -> const std::vector<Node*>&;
  [[nodiscard]] auto getNodes() -> std::vector<Node*>&;
  [[nodiscard]] auto getLinks() -> std::vector<std::unique_ptr<Link>>&;

  std::vector<Node*> nodes;
  std::vector<std::unique_ptr<Link>> links;
};

};  // namespace vkit::graph

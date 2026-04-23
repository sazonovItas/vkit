#pragma once

#include <string_view>
#include <vector>

#include "vkit/graph/pin.hpp"

namespace vkit::graph {

class Node {
 public:
  Node();
  explicit Node(std::string_view name);
  Node(const Node&);
  Node& operator=(const Node&);
  virtual ~Node() = default;

 protected:
  int graphNodeId_;
  std::string name_;
  std::vector<Pin> inputPins_;
  std::vector<Pin> outputPins_;
};

}  // namespace vkit::graph

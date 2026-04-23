#include "vkit/graph/node.hpp"

#include "vkit/graph/graph.hpp"

namespace vkit::graph {

Node::Node() : graphNodeId_{makeGraphId()} {}

Node::Node(std::string_view name) : graphNodeId_{makeGraphId()}, name_{name} {}

Node::Node(const Node& other)
    : graphNodeId_{makeGraphId()},
      inputPins_{other.inputPins_},
      outputPins_{other.outputPins_} {
  for (auto& pin : inputPins_) {
    pin.setOwnerNode(this);
  }
  for (auto& pin : outputPins_) {
    pin.setOwnerNode(this);
  }
}

Node& Node::operator=(const Node& other) {
  if (this != &other) {
    inputPins_ = other.inputPins_;
    outputPins_ = other.outputPins_;

    for (auto& pin : inputPins_) {
      pin.setOwnerNode(this);
    }
    for (auto& pin : outputPins_) {
      pin.setOwnerNode(this);
    }
  }
  return *this;
}

}  // namespace vkit::graph

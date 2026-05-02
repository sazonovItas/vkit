#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "vkit/graph/pin.hpp"

namespace vkit::graph {

class Graph;

class Node {
 public:
  Node();
  explicit Node(std::string_view name);

  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;

  virtual ~Node() = default;

  [[nodiscard]] auto getId() const -> int { return graphNodeId_; }
  [[nodiscard]] auto getName() const -> std::string_view { return name_; }

  [[nodiscard]] auto getInputs() -> std::vector<std::unique_ptr<Pin>>& {
    return inputPins_;
  }
  [[nodiscard]] auto getOutputs() -> std::vector<std::unique_ptr<Pin>>& {
    return outputPins_;
  }

 protected:
  int graphNodeId_;
  std::string name_;

  std::vector<std::unique_ptr<Pin>> inputPins_;
  std::vector<std::unique_ptr<Pin>> outputPins_;

  auto addInputPin(std::size_t key, std::string_view name) -> Pin* {
    std::size_t slot = inputPins_.size();
    inputPins_.push_back(
        std::unique_ptr<Pin>(new Pin(this, slot, false, key, name)));
    return inputPins_.back().get();
  }

  auto addOutputPin(std::size_t key, std::string_view name) -> Pin* {
    std::size_t slot = outputPins_.size();
    outputPins_.push_back(
        std::unique_ptr<Pin>(new Pin(this, slot, true, key, name)));
    return outputPins_.back().get();
  }
};

};  // namespace vkit::graph

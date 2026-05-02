#include "vkit/graph/node.hpp"

#include "vkit/graph/graph.hpp"

namespace vkit::graph {

Node::Node() : graphNodeId_{makeGraphId()} {}

Node::Node(std::string_view name) : graphNodeId_{makeGraphId()}, name_{name} {}

};  // namespace vkit::graph

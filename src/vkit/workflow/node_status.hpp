#pragma once

namespace vkit::workflow {

enum class NodeStatus {
  kStale,
  kExecuting,
  kReady,
};

};  // namespace vkit::workflow

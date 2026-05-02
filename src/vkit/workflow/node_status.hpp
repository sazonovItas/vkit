#pragma once

namespace vkit::workflow {

enum class NodeStatus {
  kStale,
  kExecuting,
  kReady,
  kError,
};

};  // namespace vkit::workflow

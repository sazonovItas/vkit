#pragma once

#include <filesystem>
#include <vector>

#include "vkit/controller/workflow.hpp"
#include "vkit/workflow/workflow.hpp"

namespace vkit::workflow {

class WorkflowSerializer {
 public:
  static bool exportToFile(Workflow* workflow,
                           const std::vector<controller::NodePosition>& positions,
                           const std::filesystem::path& filePath);

  static bool importFromFile(const std::filesystem::path& filePath,
                             controller::WorkflowController* controller,
                             std::vector<controller::NodePosition>& outPositions);
};

}  // namespace vkit::workflow

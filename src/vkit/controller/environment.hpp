#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "vkit/environment/environment.hpp"
#include "vkit/environment/manager.hpp"

namespace vkit::controller {

class EnvironmentController {
 public:
  explicit EnvironmentController(env::EnvironmentManager* envManager);

  auto loadEnvironmentFromFile() -> std::optional<std::uint32_t>;

  void setCurrentEnvironment(std::uint32_t id);
  void clearCurrentEnvironment();

  [[nodiscard]] auto getCurrentEnvironmentId() const
      -> std::optional<std::uint32_t>;
  [[nodiscard]] auto getCurrentEnvironment() const
      -> std::shared_ptr<env::Environment>;

  [[nodiscard]] auto getLoadedEnvironments() const
      -> std::vector<std::shared_ptr<env::Environment>>;

  void removeEnvironment(std::uint32_t id);

 private:
  env::EnvironmentManager* envManager_{nullptr};
  std::optional<std::uint32_t> currentEnvironmentId_{std::nullopt};
};

};  // namespace vkit::controller

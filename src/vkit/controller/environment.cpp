#include "vkit/controller/environment.hpp"

namespace vkit::controller {

EnvironmentController::EnvironmentController(
    env::EnvironmentManager* envManager)
    : envManager_{envManager} {}

auto EnvironmentController::loadEnvironmentFromFile()
    -> std::optional<std::uint32_t> {
  if (!envManager_) return std::nullopt;

  auto env_id = envManager_->promptAndLoadEnvironment();
  if (env_id.has_value()) {
    setCurrentEnvironment(env_id.value());
    return env_id;
  }

  return std::nullopt;
}

void EnvironmentController::setCurrentEnvironment(std::uint32_t id) {
  if (!envManager_) return;

  if (envManager_->getEnvironment(id)) {
    currentEnvironmentId_ = id;
  }
}

void EnvironmentController::clearCurrentEnvironment() {
  currentEnvironmentId_ = std::nullopt;
}

auto EnvironmentController::getCurrentEnvironmentId() const
    -> std::optional<std::uint32_t> {
  return currentEnvironmentId_;
}

auto EnvironmentController::getCurrentEnvironment() const
    -> std::shared_ptr<env::Environment> {
  if (!envManager_ || !currentEnvironmentId_.has_value()) return nullptr;
  return envManager_->getEnvironment(currentEnvironmentId_.value());
}

auto EnvironmentController::getLoadedEnvironments() const
    -> std::vector<std::shared_ptr<env::Environment>> {
  if (!envManager_) return {};
  return envManager_->getEnvironments();
}

void EnvironmentController::removeEnvironment(std::uint32_t id) {
  if (!envManager_) return;

  if (currentEnvironmentId_ == id) {
    clearCurrentEnvironment();
  }

  envManager_->removeEnvironment(id);
}

};  // namespace vkit::controller

#pragma once

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "vkit/scene/camera.hpp"

namespace vkit::controller {

class OrbitalCameraController {
 public:
  glm::vec3 target{0.0F, 0.0F, 0.0F};
  float distance{10.0F};
  float yaw{-glm::half_pi<float>()};
  float pitch{0.0F};

  float zoomSpeed{0.1F};
  float mouseSensitivity{0.005F};

  void setZoomSpeed(float speed) { zoomSpeed = speed; }
  void setMouseSensitivity(float sensitivity) {
    mouseSensitivity = sensitivity;
  }

  void processMouseMovement(float xOffset, float yOffset) {
    yaw += xOffset * mouseSensitivity;
    pitch -= yOffset * mouseSensitivity;

    const float limit = glm::half_pi<float>() - 0.01F;
    pitch = std::clamp(pitch, -limit, limit);
  }

  void processScroll(float yOffset) {
    distance -= yOffset * zoomSpeed;
    distance = std::max(distance, 0.1F);
  }

  void update(scene::Camera& camera) const {
    glm::vec3 front;
    front.x = std::cos(pitch) * std::cos(yaw);
    front.y = std::sin(pitch);
    front.z = std::cos(pitch) * std::sin(yaw);
    front = glm::normalize(front);

    glm::vec3 position = target - (front * distance);
    camera.lookAt(position, target);
  }
};

class FreeCameraController {
 public:
  glm::vec3 position{0.0F, 0.0F, 5.0F};
  float yaw{-glm::half_pi<float>()};
  float pitch{0.0F};

  float movementSpeed{1.0F};
  float mouseSensitivity{0.005F};

  void setMovementSpeed(float speed) { movementSpeed = speed; }
  void setMouseSensitivity(float sensitivity) {
    mouseSensitivity = sensitivity;
  }

  void processMouseMovement(float xOffset, float yOffset) {
    yaw += xOffset * mouseSensitivity;
    pitch -= yOffset * mouseSensitivity;

    const float limit = glm::half_pi<float>() - 0.01F;
    pitch = std::clamp(pitch, -limit, limit);
  }

  void move(glm::vec3 localDirection, float deltaTime) {
    float velocity = movementSpeed * deltaTime;

    glm::vec3 front;
    front.x = std::cos(pitch) * std::cos(yaw);
    front.y = std::sin(pitch);
    front.z = std::cos(pitch) * std::sin(yaw);
    front = glm::normalize(front);

    glm::vec3 right =
        glm::normalize(glm::cross(front, glm::vec3(0.0F, 1.0F, 0.0F)));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    position += right * localDirection.x * velocity;
    position += up * localDirection.y * velocity;
    position += front * localDirection.z * velocity;
  }

  void update(scene::Camera& camera) const {
    glm::vec3 front;
    front.x = std::cos(pitch) * std::cos(yaw);
    front.y = std::sin(pitch);
    front.z = std::cos(pitch) * std::sin(yaw);
    front = glm::normalize(front);

    camera.lookAt(position, position + front);
  }
};

class FirstPersonCameraController {
 public:
  glm::vec3 position{0.0F, 1.8F, 5.0F};
  float yaw{-glm::half_pi<float>()};
  float pitch{0.0F};

  float movementSpeed{5.0F};
  float mouseSensitivity{0.005F};

  void setMovementSpeed(float speed) { movementSpeed = speed; }
  void setMouseSensitivity(float sensitivity) {
    mouseSensitivity = sensitivity;
  }

  void processMouseMovement(float xOffset, float yOffset) {
    yaw += xOffset * mouseSensitivity;
    pitch -= yOffset * mouseSensitivity;

    const float limit = glm::half_pi<float>() - 0.01F;
    pitch = std::clamp(pitch, -limit, limit);
  }

  void moveGrounded(glm::vec3 localDirection, float deltaTime) {
    float velocity = movementSpeed * deltaTime;

    glm::vec3 grounded_front;
    grounded_front.x = std::cos(yaw);
    grounded_front.y = 0.0F;
    grounded_front.z = std::sin(yaw);
    grounded_front = glm::normalize(grounded_front);

    glm::vec3 right =
        glm::normalize(glm::cross(grounded_front, glm::vec3(0.0F, 1.0F, 0.0F)));

    position += right * localDirection.x * velocity;
    position.y += localDirection.y * velocity;
    position += grounded_front * localDirection.z * velocity;
  }

  void update(scene::Camera& camera) const {
    glm::vec3 front;
    front.x = std::cos(pitch) * std::cos(yaw);
    front.y = std::sin(pitch);
    front.z = std::cos(pitch) * std::sin(yaw);
    front = glm::normalize(front);

    camera.lookAt(position, position + front);
  }
};

};  // namespace vkit::controller

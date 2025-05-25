// Camera.cpp

#include "Camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up)
    : position(position), target(target), up(up) {
    viewMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::mat4(1.0f);
}

void Camera::setPosition(float x, float y, float z) {
    position = glm::vec3(x, y, z);
}

void Camera::setTarget(float x, float y, float z) {
    target = glm::vec3(x, y, z);
}

void Camera::setUp(float x, float y, float z) {
    up = glm::vec3(x, y, z);
}

void Camera::updateViewMatrix() {
    viewMatrix = glm::lookAt(position, target, up);
}

void Camera::setOrthographic(float left, float right, float bottom, float top, float near, float far) {
    projectionMatrix = glm::ortho(left, right, bottom, top, near, far);
}

void Camera::setPerspective(float fovy, float aspect, float near, float far) {
    projectionMatrix = glm::perspective(fovy, aspect, near, far);
}

glm::mat4 Camera::getViewMatrix() {
    updateViewMatrix();
    return viewMatrix;
}

glm::mat4 Camera::getProjectionMatrix() {
    return projectionMatrix;
}

glm::vec3 Camera::getPosition() const {
    return position;
}
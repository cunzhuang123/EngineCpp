// Camera.h

#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 1000.0f),
           glm::vec3 target = glm::vec3(0.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f));

    void setPosition(float x, float y, float z);
    void setTarget(float x, float y, float z);
    void setUp(float x, float y, float z);

    void updateViewMatrix();

    void setOrthographic(float left, float right, float bottom, float top, float near, float far);
    void setPerspective(float fovy, float aspect, float near, float far);

    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();

    glm::vec3 getPosition() const; // 添加 getPosition()

private:
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};

#endif // CAMERA_H
#include "rendering/Camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 target, float fov, float aspect, float near, float far, glm::vec3 front, glm::vec3 up) : position(position), _front(front), _up(up), _fov(fov), _aspect(aspect), _near(near), _far(far)
{
    glm::vec3 dir = glm::normalize(target - position);
    _yaw = glm::degrees(std::atan2(dir.z, dir.x));
    float dxz = sqrt(dir.x * dir.x + dir.z * dir.z);
    _pitch = glm::degrees(std::atan2(dir.y, dxz));
    _front = UpdateDirection();
};

Camera::~Camera() {}

void Camera::Rotate(float deltaX, float deltaY)
{
    _yaw += deltaX;
    _pitch = glm::clamp(_pitch - deltaY, -89.0f, 89.0f);
    _front = UpdateDirection();
}

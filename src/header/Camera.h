#pragma once

#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

class Camera
{
public:
    Camera(glm::vec3 position, glm::vec3 front, glm::vec3 up, float fov, float aspect, float near, float far);
    ~Camera();

    Camera(Camera&& other) noexcept;
    Camera& operator=(Camera&& other) noexcept;

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

    inline glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(position, position + _front, _up);
    }

    inline glm::mat4 GetProjectionMatrix() const
    {
        return glm::perspective(glm::radians(_fov), _aspect, _near, _far);
    }

    inline glm::vec3 UpdateDirection() const
    {
        glm::vec3 direction;
        direction.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
        direction.y = sin(glm::radians(_pitch));
        direction.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
        return glm::normalize(direction);
    }

    void Rotate(float deltaX, float deltaY);

    glm::vec3 Front() const { return _front; }
    glm::vec3 Up() const { return _up; }

    float speed = 2.5f;
    glm::vec3 position;
    float _fov;
    float _aspect;
    float _near;
    float _far;

    private:
    float _yaw = -90.0f;
    float _pitch = 0.0f;
    glm::vec3 _front;
    glm::vec3 _up;
};

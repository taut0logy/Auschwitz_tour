#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

enum Camera_Movement {
    CAM_FORWARD,
    CAM_BACKWARD,
    CAM_LEFT,
    CAM_RIGHT,
    CAM_UP,
    CAM_DOWN
};

class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;

    float speed;
    float sprintMultiplier;
    float sensitivity;
    float fov;

    // Boundary limits (v2 spec)
    float minX = -138.0f, maxX = 138.0f;
    float minY = 1.5f,    maxY = 12.0f;
    float minZ = -98.0f,  maxZ = 98.0f;

    Camera(glm::vec3 pos = glm::vec3(150.0f, 1.7f, 0.0f),
           float yawVal = 180.0f, float pitchVal = 0.0f)
    {
        position = pos;
        worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        yaw = yawVal;       // 180 = facing -X (west, into camp)
        pitch = pitchVal;
        speed = 7.0f;
        sprintMultiplier = 3.5f;
        sensitivity = 0.08f;
        fov = 65.0f;
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    void processKeyboard(Camera_Movement direction, float deltaTime, bool sprint = false) {
        float velocity = speed * deltaTime * (sprint ? sprintMultiplier : 1.0f);

        switch (direction) {
            case CAM_FORWARD:  position += front * velocity; break;
            case CAM_BACKWARD: position -= front * velocity; break;
            case CAM_LEFT:     position -= right * velocity; break;
            case CAM_RIGHT:    position += right * velocity; break;
            case CAM_UP:       position.y += velocity;           break;
            case CAM_DOWN:     position.y -= velocity;           break;
        }

        // Clamp to scene bounds
        position.x = glm::clamp(position.x, minX, maxX);
        position.y = glm::clamp(position.y, minY, maxY);
        position.z = glm::clamp(position.z, minZ, maxZ);
    }

    void processMouseMovement(float xoffset, float yoffset) {
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw   += xoffset;
        pitch += yoffset;

        if (pitch >  88.0f) pitch =  88.0f;
        if (pitch < -88.0f) pitch = -88.0f;

        updateCameraVectors();
    }

    void processMouseScroll(float yoffset) {
        fov -= yoffset;
        if (fov < 20.0f) fov = 20.0f;
        if (fov > 90.0f) fov = 90.0f;
    }

private:
    void updateCameraVectors() {
        glm::vec3 dir;
        dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        dir.y = sin(glm::radians(pitch));
        dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(dir);
        right = glm::normalize(glm::cross(front, worldUp));
        up    = glm::normalize(glm::cross(right, front));
    }
};

#endif

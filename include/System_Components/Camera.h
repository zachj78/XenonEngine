#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include "Utils/config.h"


//Inject this class into UniformBufferManager, figure out how this math shit works
class Camera {
public:
    glm::vec3 position;
    float yaw, pitch;
    float fov = 30.0f;

    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    Camera(glm::vec3 startPos, glm::vec3 upDir, float startYaw, float startPitch)
        : position(startPos), worldUp(upDir), yaw(startYaw), pitch(startPitch) {
        std::cout << "Initializing Camera" << std::endl;
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 getProjectionMatrix(float aspect) const {
        return glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f);
    }

    glm::vec3 getPosition() const {
        return position;
    }

    void move(int key, float delta) {
        switch (key) {
            case (GLFW_KEY_W): 
                position += front * delta;
                break;
            case(GLFW_KEY_S): 
                position -= front * delta;
                break;
            case(GLFW_KEY_D):
                position += right * delta;
                break;
            case(GLFW_KEY_A):
                position -= right * delta;
                break;
            case(GLFW_KEY_SPACE): 
                position += worldUp * delta; 
                break; 
            case(GLFW_KEY_LEFT_CONTROL): 
                position -= worldUp * delta; 
                break;
        }
    }

    void processMouse(float xOffset, float yOffset) {
        yaw += xOffset; 
        pitch -= yOffset; 

        if (pitch > 85.0f) pitch = 85.0f;
        if (pitch < -85.0f) pitch = -85.0f;

        updateCameraVectors();
    }

    void updateCameraVectors() {
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        front = glm::normalize(direction);
        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }

private:


};


#endif
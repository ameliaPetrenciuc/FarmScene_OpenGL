#include "Camera.hpp"

namespace gps {

    float yaw, pitch;
    // Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;

        // Calculăm direcțiile frontale și dreapta ale camerei
        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
        yaw = glm::degrees(atan2(cameraFrontDirection.z, cameraFrontDirection.x));
        pitch = glm::degrees(asin(cameraFrontDirection.y)); // Vertical angle (pitch)
        this->cameraTarget = cameraPosition + cameraFrontDirection;
    }
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraTarget, cameraUpDirection);
    }

    void Camera::move(MOVE_DIRECTION direction, float speed) {
        if (direction == MOVE_FORWARD) {
            cameraPosition += cameraFrontDirection * speed;
            cameraTarget += cameraFrontDirection * speed;
        }
        if (direction == MOVE_BACKWARD) {
            cameraPosition -= cameraFrontDirection * speed;
            cameraTarget -= cameraFrontDirection * speed;
        }
        if (direction == MOVE_RIGHT) {
            cameraPosition += cameraRightDirection * speed;
            cameraTarget += cameraRightDirection * speed;
        }
        if (direction == MOVE_LEFT) {
            cameraPosition -= cameraRightDirection * speed;
            cameraTarget -= cameraRightDirection * speed;
        }
    }
    void Camera::rotate(float pitch_Change, float yaw_Change) {

        yaw += yaw_Change;
        pitch += pitch_Change;

        // Clamp the pitch to avoid gimbal lock
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
        glm::vec3 front;
        front.x = cos(glm::radians(yaw));
        front.y = sin(glm::radians(pitch));                         
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        cameraFrontDirection = glm::normalize(front);
        cameraTarget = cameraPosition + cameraFrontDirection;
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }

    glm::vec3 Camera::getPosition() const {
        return cameraPosition;
    }

    void Camera::setPosition(float x, float y, float z) {
        // Update camera position
        cameraPosition = glm::vec3(x, y, z);

        // Recalculate target and front direction based on yaw and pitch
        glm::vec3 front;
        front.x = cos(glm::radians(yaw));

        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        cameraFrontDirection = glm::normalize(front);
        cameraTarget = cameraPosition + cameraFrontDirection;

        // Update the right direction
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }

    void Camera::setDirection(const glm::vec3& target) {
        // Calculează noua direcție frontală
        cameraFrontDirection = glm::normalize(target - cameraPosition);

        // Actualizează yaw și pitch pe baza noii direcții
        yaw = glm::degrees(atan2(cameraFrontDirection.z, cameraFrontDirection.x)); // Unghi orizontal
        pitch = glm::degrees(asin(cameraFrontDirection.y)); // Unghi vertical

        // Recalculează target și direcția dreaptă
        cameraTarget = cameraPosition + cameraFrontDirection;
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }



}
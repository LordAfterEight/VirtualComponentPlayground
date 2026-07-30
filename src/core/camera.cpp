#include "core/camera.hpp"

#include <algorithm>
#include <cmath>

float Camera::world_x(float window_x) const {
    return this->x + window_x / this->zoom;
}

float Camera::world_y(float window_y) const {
    return this->y + window_y / this->zoom;
}

void Camera::zoom_at(float scroll, float window_x, float window_y) {
    float focused_world_x = this->world_x(window_x);
    float focused_world_y = this->world_y(window_y);
    this->zoom *= std::pow(1.05f, scroll);
    this->zoom = std::clamp(this->zoom, 0.1f, 10.0f);
    this->x = focused_world_x - window_x / this->zoom;
    this->y = focused_world_y - window_y / this->zoom;
}

void Camera::pan_by(float window_x_delta, float window_y_delta) {
    this->x -= window_x_delta / this->zoom;
    this->y -= window_y_delta / this->zoom;
}

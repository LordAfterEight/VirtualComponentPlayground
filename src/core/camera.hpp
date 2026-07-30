#pragma once

struct Camera {
    float x = 0.0f;
    float y = 0.0f;
    float zoom = 1.0f;

    float world_x(float window_x) const;
    float world_y(float window_y) const;
    void zoom_at(float scroll, float window_x, float window_y);
    void pan_by(float window_x_delta, float window_y_delta);
};

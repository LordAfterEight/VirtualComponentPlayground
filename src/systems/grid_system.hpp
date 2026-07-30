#pragma once

#include <SDL3/SDL_render.h>

#include "core/camera.hpp"

namespace GridSystem {
    void render(SDL_Renderer* renderer, const Camera& camera, float canvas_width, float canvas_height,
        int window_width, int window_height, float detail_scale);
}

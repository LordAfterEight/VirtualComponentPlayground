#pragma once

#include <SDL3/SDL_events.h>

#include "core/camera.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"

class InteractionSystem {
    private:
        Entity dragged_entity{0, 0};
        bool is_dragging = false;
        bool is_panning = false;
        float drag_offset_x = 0.0f;
        float drag_offset_y = 0.0f;

        void begin_drag(World& world, const Camera& camera, float mouse_x, float mouse_y);
        void update_drag(World& world, const Camera& camera, float mouse_x, float mouse_y, float canvas_width, float canvas_height);

    public:
        void handle_event(const SDL_Event& event, World& world, Camera& camera, float canvas_width, float canvas_height);
};

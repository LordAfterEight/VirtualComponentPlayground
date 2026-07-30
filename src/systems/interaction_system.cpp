#include "systems/interaction_system.hpp"

#include <cmath>

#include <SDL3/SDL_mouse.h>

#include "geometry/package_geometry.hpp"

void InteractionSystem::begin_drag(World& world, const Camera& camera, float mouse_x, float mouse_y) {
    float world_x = camera.world_x(mouse_x);
    float world_y = camera.world_y(mouse_y);
    const std::vector<Entity>& entities = world.package_entities();

    for (int index = static_cast<int>(entities.size()) - 1; index >= 0; index--) {
        Entity entity = entities[index];
        const Transform& transform = world.transform(entity);
        const Package& package = world.package(entity);
        if (PackageGeometry::contains(transform, package, world_x, world_y)) {
            this->dragged_entity = entity;
            this->is_dragging = true;
            this->drag_offset_x = world_x - transform.x;
            this->drag_offset_y = world_y - transform.y;
            return;
        }
    }
}

void InteractionSystem::update_drag(World& world, const Camera& camera, float mouse_x, float mouse_y,
    float canvas_width, float canvas_height) {
    if (!this->is_dragging || !world.is_alive(this->dragged_entity)) {
        return;
    }

    float candidate_x = std::round((camera.world_x(mouse_x) - this->drag_offset_x) / PackageGeometry::grid_spacing) * PackageGeometry::grid_spacing;
    float candidate_y = std::round((camera.world_y(mouse_y) - this->drag_offset_y) / PackageGeometry::grid_spacing) * PackageGeometry::grid_spacing;
    if (PackageGeometry::can_place(world, this->dragged_entity, candidate_x, candidate_y, canvas_width, canvas_height)) {
        Transform& transform = world.transform(this->dragged_entity);
        transform.x = candidate_x;
        transform.y = candidate_y;
    }
}

void InteractionSystem::handle_event(const SDL_Event& event, World& world, Camera& camera,
    float canvas_width, float canvas_height) {
    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        float scroll = event.wheel.y;
        if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
            scroll = -scroll;
        }
        camera.zoom_at(scroll, event.wheel.mouse_x, event.wheel.mouse_y);
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        this->begin_drag(world, camera, event.button.x, event.button.y);
    }
    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
        this->is_dragging = false;
    }
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_MIDDLE) {
        this->is_panning = true;
    }
    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_MIDDLE) {
        this->is_panning = false;
    }
    if (event.type == SDL_EVENT_MOUSE_MOTION && this->is_panning) {
        camera.pan_by(event.motion.xrel, event.motion.yrel);
    }
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        this->update_drag(world, camera, event.motion.x, event.motion.y, canvas_width, canvas_height);
    }
}

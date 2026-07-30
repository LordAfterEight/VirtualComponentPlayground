#pragma once

#include "ecs/components.hpp"
#include "ecs/world.hpp"

namespace PackageGeometry {
    constexpr float grid_spacing = 5.0f;

    void body_size(const Package& package, float& width, float& height);
    bool contains(const Transform& transform, const Package& package, float world_x, float world_y);
    bool can_place(const World& world, Entity moving_entity, float x, float y, float canvas_width, float canvas_height);
}

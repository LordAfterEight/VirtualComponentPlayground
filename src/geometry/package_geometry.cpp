#include "geometry/package_geometry.hpp"

#include <algorithm>

namespace PackageGeometry {
    void body_size(const Package& package, float& width, float& height) {
        uint8_t larger_horizontal = std::max(package.pins_left, package.pins_right);
        uint8_t larger_vertical = std::max(package.pins_top, package.pins_bottom);
        width = std::max(20.0f, (static_cast<float>(larger_vertical) + 1.0f) * grid_spacing);
        height = std::max(20.0f, (static_cast<float>(larger_horizontal) + 1.0f) * grid_spacing);
    }

    bool contains(const Transform& transform, const Package& package, float world_x, float world_y) {
        float width = 0.0f;
        float height = 0.0f;
        body_size(package, width, height);
        return world_x >= transform.x && world_x <= transform.x + width &&
            world_y >= transform.y && world_y <= transform.y + height;
    }

    bool can_place(const World& world, Entity moving_entity, float x, float y, float canvas_width, float canvas_height) {
        const Package& moving_package = world.package(moving_entity);
        float moving_width = 0.0f;
        float moving_height = 0.0f;
        body_size(moving_package, moving_width, moving_height);
        if (x < 0.0f || y < 0.0f || x + moving_width > canvas_width || y + moving_height > canvas_height) {
            return false;
        }

        for (Entity entity : world.package_entities()) {
            if (entity == moving_entity) {
                continue;
            }

            const Transform& other_transform = world.transform(entity);
            const Package& other_package = world.package(entity);
            float other_width = 0.0f;
            float other_height = 0.0f;
            body_size(other_package, other_width, other_height);
            bool bodies_overlap =
                x < other_transform.x + other_width && x + moving_width > other_transform.x &&
                y < other_transform.y + other_height && y + moving_height > other_transform.y;
            if (bodies_overlap) {
                return false;
            }
        }
        return true;
    }
}

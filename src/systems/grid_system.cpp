#include "systems/grid_system.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "geometry/package_geometry.hpp"

namespace GridSystem {
    void render(SDL_Renderer* renderer, const Camera& camera, float canvas_width, float canvas_height,
        int window_width, int window_height, float detail_scale) {
        constexpr float minimum_screen_spacing = 8.0f;
        float rendered_grid_spacing = PackageGeometry::grid_spacing;
        while (rendered_grid_spacing * camera.zoom < minimum_screen_spacing) {
            rendered_grid_spacing *= 2.0f;
        }

        float visible_left = std::max(0.0f, camera.x);
        float visible_top = std::max(0.0f, camera.y);
        float visible_right = std::min(canvas_width, camera.x + window_width / camera.zoom);
        float visible_bottom = std::min(canvas_height, camera.y + window_height / camera.zoom);
        if (visible_left > visible_right || visible_top > visible_bottom) {
            return;
        }

        int first_column = static_cast<int>(std::ceil(visible_left / rendered_grid_spacing));
        int last_column = static_cast<int>(std::floor(visible_right / rendered_grid_spacing));
        int first_row = static_cast<int>(std::ceil(visible_top / rendered_grid_spacing));
        int last_row = static_cast<int>(std::floor(visible_bottom / rendered_grid_spacing));
        std::vector<SDL_FPoint> dots;
        dots.reserve(static_cast<size_t>(last_column - first_column + 1) *
            static_cast<size_t>(last_row - first_row + 1));

        for (int column = first_column; column <= last_column; column++) {
            float x = (static_cast<float>(column) * rendered_grid_spacing - camera.x) * detail_scale;
            for (int row = first_row; row <= last_row; row++) {
                float y = (static_cast<float>(row) * rendered_grid_spacing - camera.y) * detail_scale;
                dots.push_back(SDL_FPoint{x, y});
            }
        }

        if (dots.empty()) {
            return;
        }

        SDL_SetRenderDrawColor(renderer, 62, 72, 94, 255);
        SDL_RenderPoints(renderer, dots.data(), static_cast<int>(dots.size()));
    }
}

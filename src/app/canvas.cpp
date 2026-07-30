#include "app/canvas.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>
#include <string>

#include <SDL3_ttf/SDL_ttf.h>

#include "systems/grid_system.hpp"
#include "systems/interaction_system.hpp"
#include "render/package_renderer.hpp"

Canvas::Canvas(uint16_t width, uint16_t height) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(display);
    this->monitor_width = mode ? mode->w : width;
    this->monitor_height = mode ? mode->h : height;
    this->canvas_width = this->monitor_width * 2.0f;
    this->canvas_height = this->monitor_height * 2.0f;
    this->window_width = static_cast<int>(this->monitor_width * 0.5f);
    this->window_height = static_cast<int>(this->monitor_height * 0.5f);
    this->window = SDL_CreateWindow("Virtual Component Playground", this->window_width, this->window_height, SDL_WINDOW_RESIZABLE);
    this->renderer = SDL_CreateRenderer(this->window, nullptr);
    SDL_SetRenderVSync(this->renderer, 1);

    TTF_Init();
    this->interaction_system = std::make_unique<InteractionSystem>();
    this->package_renderer = std::make_unique<PackageRenderer>(this->renderer);
    this->fps_sample_start = SDL_GetTicks();
}

Canvas::~Canvas() {
    this->package_renderer.reset();
    this->interaction_system.reset();
    TTF_Quit();
    SDL_DestroyRenderer(this->renderer);
    SDL_DestroyWindow(this->window);
    SDL_Quit();
    std::cout << "Exiting..." << std::endl;
}

World& Canvas::world() {
    return this->world_state;
}

float Canvas::detail_scale() const {
    return std::max(4.0f, std::ceil(this->camera.zoom));
}

void Canvas::render_canvas_direction_indicator() {
    float canvas_left = -this->camera.x * this->camera.zoom;
    float canvas_top = -this->camera.y * this->camera.zoom;
    float canvas_right = canvas_left + this->canvas_width * this->camera.zoom;
    float canvas_bottom = canvas_top + this->canvas_height * this->camera.zoom;
    bool canvas_is_visible = canvas_right >= 0.0f && canvas_left <= this->window_width &&
        canvas_bottom >= 0.0f && canvas_top <= this->window_height;
    if (canvas_is_visible) {
        return;
    }

    float screen_center_x = this->window_width * 0.5f;
    float screen_center_y = this->window_height * 0.5f;
    float canvas_center_x = (this->canvas_width * 0.5f - this->camera.x) * this->camera.zoom;
    float canvas_center_y = (this->canvas_height * 0.5f - this->camera.y) * this->camera.zoom;
    float direction_x = canvas_center_x - screen_center_x;
    float direction_y = canvas_center_y - screen_center_y;
    float direction_length = std::sqrt(direction_x * direction_x + direction_y * direction_y);
    if (direction_length == 0.0f) {
        return;
    }

    float unit_x = direction_x / direction_length;
    float unit_y = direction_y / direction_length;
    float edge_x = screen_center_x - 16.0f;
    float edge_y = screen_center_y - 16.0f;
    float distance_x = direction_x == 0.0f ? 1.0e30f : edge_x / std::abs(direction_x);
    float distance_y = direction_y == 0.0f ? 1.0e30f : edge_y / std::abs(direction_y);
    float distance = std::min(distance_x, distance_y);
    float arrow_x = screen_center_x + direction_x * distance;
    float arrow_y = screen_center_y + direction_y * distance;
    float tail_x = arrow_x - unit_x * 24.0f;
    float tail_y = arrow_y - unit_y * 24.0f;
    float perpendicular_x = -unit_y;
    float perpendicular_y = unit_x;

    SDL_SetRenderDrawColor(this->renderer, 255, 180, 80, 255);
    SDL_RenderLine(this->renderer, tail_x, tail_y, arrow_x, arrow_y);
    SDL_RenderLine(this->renderer, arrow_x, arrow_y, arrow_x - unit_x * 10.0f + perpendicular_x * 6.0f,
        arrow_y - unit_y * 10.0f + perpendicular_y * 6.0f);
    SDL_RenderLine(this->renderer, arrow_x, arrow_y, arrow_x - unit_x * 10.0f - perpendicular_x * 6.0f,
        arrow_y - unit_y * 10.0f - perpendicular_y * 6.0f);
}

void Canvas::render_debug_info() {
    if (!this->show_debug_info) {
        return;
    }

    SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 100);
    SDL_RenderDebugText(this->renderer, 10.0f, 10.0f,
        std::format("Window  Resolution: {}x{}", this->window_width, this->window_height).c_str());
    SDL_RenderDebugText(this->renderer, 10.0f, 20.0f,
        std::format("Canvas  Resolution: {}x{}", this->canvas_width, this->canvas_height).c_str());
    SDL_RenderDebugText(this->renderer, 10.0f, 30.0f,
        std::format("Monitor Resolution: {}x{}", this->monitor_width, this->monitor_height).c_str());
    std::string fps_text = std::format("FPS: {:.1f}", this->fps);
    std::string zoom_text = std::format("Zoom: {:.2f}", this->camera.zoom);
    SDL_RenderDebugText(this->renderer, this->window_width - static_cast<float>(fps_text.length() * 8) - 10.0f, 10.0f, fps_text.c_str());
    SDL_RenderDebugText(this->renderer, this->window_width - static_cast<float>(zoom_text.length() * 8) - 10.0f, 20.0f, zoom_text.c_str());
}

void Canvas::start() {
    while (this->running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                this->running = false;
            }
            this->interaction_system->handle_event(event, this->world_state, this->camera,
                this->canvas_width, this->canvas_height);
        }

        SDL_GetRenderOutputSize(this->renderer, &this->window_width, &this->window_height);
        SDL_SetRenderDrawColor(this->renderer, 10, 10, 20, 255);
        SDL_RenderClear(this->renderer);

        float detail = this->detail_scale();
        SDL_SetRenderScale(this->renderer, this->camera.zoom / detail, this->camera.zoom / detail);
        SDL_FRect canvas_rect{-this->camera.x * detail, -this->camera.y * detail,
            this->canvas_width * detail, this->canvas_height * detail};
        SDL_SetRenderDrawColor(this->renderer, 15, 15, 25, 255);
        SDL_RenderFillRect(this->renderer, &canvas_rect);
        GridSystem::render(this->renderer, this->camera, this->canvas_width, this->canvas_height,
            this->window_width, this->window_height, detail);
        this->package_renderer->render(this->world_state, this->camera, detail);

        SDL_SetRenderScale(this->renderer, 1.0f, 1.0f);
        this->render_canvas_direction_indicator();
        this->render_debug_info();
        SDL_RenderPresent(this->renderer);

        this->frame_count++;
        uint64_t now = SDL_GetTicks();
        uint64_t elapsed = now - this->fps_sample_start;
        if (elapsed >= 100) {
            this->fps = this->frame_count * 1000.0f / elapsed;
            this->frame_count = 0;
            this->fps_sample_start = now;
        }
    }
}

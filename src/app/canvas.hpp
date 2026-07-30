#pragma once

#include <memory>

#include <SDL3/SDL.h>

#include "core/camera.hpp"
#include "ecs/world.hpp"

class InteractionSystem;
class PackageRenderer;

class Canvas {
    private:
        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        int monitor_width = 0;
        int monitor_height = 0;
        int window_width = 1280;
        int window_height = 720;
        float canvas_width = 2560.0f;
        float canvas_height = 1440.0f;
        bool running = true;
        bool show_debug_info = true;
        float fps = 0.0f;
        uint64_t frame_count = 0;
        uint64_t fps_sample_start = 0;

        World world_state;
        Camera camera;
        std::unique_ptr<InteractionSystem> interaction_system;
        std::unique_ptr<PackageRenderer> package_renderer;

        float detail_scale() const;
        void render_canvas_direction_indicator();
        void render_debug_info();

    public:
        Canvas(uint16_t width, uint16_t height);
        ~Canvas();

        Canvas(const Canvas&) = delete;
        Canvas& operator=(const Canvas&) = delete;

        World& world();
        void start();
};

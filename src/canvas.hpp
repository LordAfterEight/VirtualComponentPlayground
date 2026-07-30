#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <stdint.h>
#include <vector>
#include <iostream>
#include <format>
#include <algorithm>
#include <cmath>

#include "error.hpp"
#include "component.hpp"

enum RuntimeEvent {
    Closed,
};

class Canvas {
    private:
        SDL_Color last_color;
        SDL_Color curr_color;
        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_Event event;
        TTF_Font* monofont_light;
        TTF_Font* monofont_regular;
        TTF_Font* monofont_bold;
        TTF_TextEngine* textengine;

        int monitor_width;
        int monitor_height;
        uint64_t frame_count = 0;
        uint64_t fps_sample_start = 0;

        float zoom = 1.0f;
        float camera_x = 0.0f;
        float camera_y = 0.0f;
        bool is_panning = false;

        void render_canvas_direction_indicator() {
            float canvas_left = -this->camera_x * this->zoom;
            float canvas_top = -this->camera_y * this->zoom;
            float canvas_right = canvas_left + this->canvas_width * this->zoom;
            float canvas_bottom = canvas_top + this->canvas_height * this->zoom;

            bool canvas_is_visible =
                canvas_right >= 0.0f && canvas_left <= this->window_width &&
                canvas_bottom >= 0.0f && canvas_top <= this->window_height;

            if (canvas_is_visible) {
                return;
            }

            float screen_center_x = this->window_width * 0.5f;
            float screen_center_y = this->window_height * 0.5f;
            float canvas_center_x = (this->canvas_width * 0.5f - this->camera_x) * this->zoom;
            float canvas_center_y = (this->canvas_height * 0.5f - this->camera_y) * this->zoom;
            float direction_x = canvas_center_x - screen_center_x;
            float direction_y = canvas_center_y - screen_center_y;
            float direction_length = std::sqrt(direction_x * direction_x + direction_y * direction_y);

            if (direction_length == 0.0f) {
                return;
            }

            float unit_x = direction_x / direction_length;
            float unit_y = direction_y / direction_length;
            float edge_x = this->window_width * 0.5f - 16.0f;
            float edge_y = this->window_height * 0.5f - 16.0f;
            float distance_to_edge_x = direction_x == 0.0f ? 1.0e30f : edge_x / std::abs(direction_x);
            float distance_to_edge_y = direction_y == 0.0f ? 1.0e30f : edge_y / std::abs(direction_y);
            float distance_to_edge = std::min(distance_to_edge_x, distance_to_edge_y);

            float arrow_x = screen_center_x + direction_x * distance_to_edge;
            float arrow_y = screen_center_y + direction_y * distance_to_edge;
            float tail_x = arrow_x - unit_x * 24.0f;
            float tail_y = arrow_y - unit_y * 24.0f;
            float perpendicular_x = -unit_y;
            float perpendicular_y = unit_x;

            SDL_SetRenderDrawColor(this->renderer, 255, 180, 80, 255);
            SDL_RenderLine(this->renderer, tail_x, tail_y, arrow_x, arrow_y);
            SDL_RenderLine(this->renderer, arrow_x, arrow_y,
                arrow_x - unit_x * 10.0f + perpendicular_x * 6.0f,
                arrow_y - unit_y * 10.0f + perpendicular_y * 6.0f);
            SDL_RenderLine(this->renderer, arrow_x, arrow_y,
                arrow_x - unit_x * 10.0f - perpendicular_x * 6.0f,
                arrow_y - unit_y * 10.0f - perpendicular_y * 6.0f);
        }

        void render_components() {
            this->change_drawing_color(200, 200, 200, 255);
            for (Component component : this->components) {
                float x = component.pos_x - this->camera_x;
                float y = component.pos_y - this->camera_y;

                uint8_t larger_horizontal;
                uint8_t larger_vertical;

                if (component.pins_left > component.pins_right) larger_horizontal = component.pins_left; else larger_horizontal = component.pins_right;
                if (component.pins_top > component.pins_bottom) larger_vertical = component.pins_top; else larger_vertical = component.pins_bottom;

                TTF_Text* label = TTF_CreateText(this->textengine, this->monofont_regular, component.name.c_str(), 0);
                TTF_SetTextColor(label, 255, 200, 230, 200);
                TTF_DrawRendererText(label, x, y - 10.0f);

                SDL_FRect rect = SDL_FRect{x, y, 20.0f + static_cast<float>(larger_horizontal) * 4.0f, 20.0f + larger_vertical * 4.0f};
                SDL_RenderRect(this->renderer, &rect);
            }
            this->revert_to_last_color();
        }

    public:
        bool show_debug_info = true;
        int window_width = 1280;
        int window_height = 720;
        float canvas_width = 1280 * 2;
        float canvas_height = 720 * 2;
        bool running;
        float fps = 0.0f;
        std::vector<Component> components;

        Canvas(uint16_t width, uint16_t height) {
            SDL_Init(SDL_INIT_VIDEO);
            SDL_DisplayID display = SDL_GetPrimaryDisplay();
            const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(display);
            this->fps_sample_start = SDL_GetTicks();
            this->monitor_width = mode->w;
            this->monitor_height = mode->h;
            this->canvas_width = mode->w * 2;
            this->canvas_height = mode->h * 2;
            this->window_width = mode->w * 0.5;
            this->window_height = mode->h * 0.5;
            this->running = true;
            this->window = SDL_CreateWindow(
                "Virtual Component Playground", this->window_width, this->window_height, SDL_WINDOW_RESIZABLE
            );
            this->renderer = SDL_CreateRenderer(this->window, nullptr);
            SDL_SetRenderVSync(this->renderer, 1);

            TTF_Init();
            this->textengine = TTF_CreateRendererTextEngine(this->renderer);
            this->monofont_light = TTF_OpenFont("assets/fonts/IBMPlexMono-Light.ttf", 24.0f);
            this->monofont_regular = TTF_OpenFont("assets/fonts/IBMPlexMono-Regular.ttf", 24.0f);
            this->monofont_bold = TTF_OpenFont("assets/fonts/IBMPlexMono-Bold.ttf", 24.0f);
        }

        ~Canvas() {
            SDL_DestroyRenderer(this->renderer);
            SDL_DestroyWindow(this->window);
            SDL_Quit();
            std::cout << "Exiting..." << std::endl;
        }

        void change_drawing_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
            this->last_color = this->curr_color;
            this->curr_color = SDL_Color{r, g, b, a};
            SDL_SetRenderDrawColor(this->renderer, r, g, b, a);
        };

        void revert_to_last_color() {
            SDL_Color temp = this->last_color;
            this->last_color = this->curr_color;
            this->curr_color = temp;
        }

        void add_component(Component component) {
            this->components.push_back(component);
        }

        RuntimeEvent start() {
            while (this->running) {
                while (SDL_PollEvent(&this->event)) {
                    if (this->event.type == SDL_EVENT_QUIT) {
                        this->running = false;
                    }
                    if (this->event.type == SDL_EVENT_MOUSE_WHEEL) {
                        float scroll = this->event.wheel.y;
                        float mouse_x = this->event.wheel.mouse_x;
                        float mouse_y = this->event.wheel.mouse_y;

                        float canvas_x = this->camera_x + mouse_x / this->zoom;
                        float canvas_y = this->camera_y + mouse_y / this->zoom;

                        if (this->event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                            scroll = -scroll;
                        }

                        this->zoom *= std::pow(1.05f, scroll);
                        this->zoom = std::clamp(this->zoom, 0.1f, 10.0f);

                        this->camera_x = canvas_x - mouse_x / this->zoom;
                        this->camera_y = canvas_y - mouse_y / this->zoom;
                    }
                    if (this->event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                        this->event.button.button == SDL_BUTTON_MIDDLE) {
                        this->is_panning = true;
                    }
                    if (this->event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                        this->event.button.button == SDL_BUTTON_MIDDLE) {
                        this->is_panning = false;
                    }
                    if (this->event.type == SDL_EVENT_MOUSE_MOTION && this->is_panning) {
                        this->camera_x -= this->event.motion.xrel / this->zoom;
                        this->camera_y -= this->event.motion.yrel / this->zoom;
                    }
                }
                SDL_GetRenderOutputSize(this->renderer, &this->window_width, &this->window_height);

                SDL_SetRenderDrawColor(this->renderer, 10, 10, 20, 255);
                SDL_RenderClear(this->renderer);

                SDL_SetRenderScale(this->renderer, this->zoom, this->zoom);  // === Scaled drawing starts here ===

                SDL_FRect rect = SDL_FRect{-this->camera_x, -this->camera_y, this->canvas_width, this->canvas_height};
                this->change_drawing_color(15, 15, 25, 255);
                SDL_RenderFillRect(this->renderer, &rect);
                this->render_components();

                SDL_SetRenderScale(this->renderer, 1.0f, 1.0f);             // === Scaled drawing ends here ===
                this->render_canvas_direction_indicator();

                if (this->show_debug_info) {
                    SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 100);
                    SDL_RenderDebugText(this->renderer, 10.0, 10.0, std::format("Window  Resolution: {}x{}", this->window_width, this->window_height).c_str());
                    SDL_RenderDebugText(this->renderer, 10.0, 20.0, std::format("Canvas  Resolution: {}x{}", this->canvas_width, this->canvas_height).c_str());
                    SDL_RenderDebugText(this->renderer, 10.0, 30.0, std::format("Monitor Resolution: {}x{}", this->monitor_width, this->monitor_height).c_str());
                    std::string fps_text = std::format("FPS: {:.1f}", this->fps);
                    std::string zoom_text = std::format("Zoom: {:.2f}", this->zoom);
                    SDL_RenderDebugText(this->renderer, this->window_width - strlen(fps_text.c_str()) * 8 - 10, 10.0, fps_text.c_str());
                    SDL_RenderDebugText(this->renderer, this->window_width - strlen(zoom_text.c_str()) * 8 - 10, 20.0, zoom_text.c_str());
                }

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
            return RuntimeEvent::Closed;
        }
};

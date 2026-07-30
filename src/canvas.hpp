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
        uint16_t canvas_width = 1280 * 2;
        uint16_t canvas_height = 720 * 2;
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

                        if (this->event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                            scroll = -scroll;
                        }

                        this->zoom *= std::pow(1.15f, scroll);
                        this->zoom = std::clamp(this->zoom, 0.1f, 10.0f);
                    }
                }
                SDL_GetRenderOutputSize(this->renderer, &this->window_width, &this->window_height);

                SDL_SetRenderDrawColor(this->renderer, 10, 10, 20, 255);
                SDL_RenderClear(this->renderer);

                if (this->show_debug_info) {
                    SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 100);
                    SDL_RenderDebugText(this->renderer, 10.0, 10.0, std::format("Window  Resolution: {}x{}", this->window_width, this->window_height).c_str());
                    SDL_RenderDebugText(this->renderer, 10.0, 20.0, std::format("Canvas  Resolution: {}x{}", this->canvas_width, this->canvas_height).c_str());
                    SDL_RenderDebugText(this->renderer, 10.0, 30.0, std::format("Monitor Resolution: {}x{}", this->monitor_width, this->monitor_height).c_str());
                    std::string fps_text = std::format("FPS: {:.1f}", this->fps);
                    SDL_RenderDebugText(this->renderer, this->window_width - strlen(fps_text.c_str()) * 8 - 10, 10.0, fps_text.c_str());
                }

                SDL_SetRenderScale(this->renderer, this->zoom, this->zoom);
                this->render_components();
                SDL_SetRenderScale(this->renderer, 1.0f, 1.0f);

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

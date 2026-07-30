#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL.h>
#include <stdint.h>
#include <format>
#include <vector>
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
        int monitor_width;
        int monitor_height;
        uint64_t frame_count = 0;
        uint64_t fps_sample_start = 0;
    public:
        bool show_debug_info = true;
        int window_width = 1280;
        int window_height = 720;
        uint16_t canvas_width;
        uint16_t canvas_height;
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
        }

        ~Canvas() {
            SDL_DestroyRenderer(this->renderer);
            SDL_DestroyWindow(this->window);
            SDL_Quit();
        }

        void change_drawing_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
            this->last_color = this->curr_color;
            this->curr_color = SDL_Color{r, g, b, a};
            SDL_SetRenderDrawColor(this->renderer, r, g, b, a);
        };

        RuntimeEvent start() {
            while (this->running) {
                while (SDL_PollEvent(&this->event)) {
                    if (this->event.type == SDL_EVENT_QUIT) {
                        this->running = false;
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
                    const char* fps_text = std::format("FPS: {:.1f}", this->fps).c_str();
                    SDL_RenderDebugText(this->renderer, this->window_width - strlen(fps_text) * 8 - 10, 10.0, fps_text);
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

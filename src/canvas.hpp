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
#include <string>
#include <unordered_map>

#include "error.hpp"
#include "component.hpp"

enum RuntimeEvent {
    Closed,
};

class Canvas {
    private:
        struct SilkscreenTexture {
            SDL_Texture* texture;
            int width;
            int height;
        };

        SDL_Color last_color;
        SDL_Color curr_color;
        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_Event event;
        TTF_Font* monofont_light;
        TTF_Font* monofont_regular;
        TTF_Font* monofont_bold;
        TTF_TextEngine* textengine;
        std::unordered_map<int, TTF_Font*> silkscreen_fonts;
        std::unordered_map<std::string, SilkscreenTexture> silkscreen_textures;

        int monitor_width;
        int monitor_height;
        uint64_t frame_count = 0;
        uint64_t fps_sample_start = 0;

        float zoom = 1.0f;
        float camera_x = 0.0f;
        float camera_y = 0.0f;
        bool is_panning = false;
        float regular_font_detail_scale = 0.0f;

        static constexpr float minimum_detail_scale = 4.0f;
        static constexpr float silkscreen_minimum_screen_height = 12.0f;

        float get_detail_scale() const {
            return std::max(minimum_detail_scale, std::ceil(this->zoom));
        }

        TTF_Font* get_silkscreen_font(int world_point_size, float detail_scale) {
            int rendered_point_size = std::max(1, static_cast<int>(std::ceil(world_point_size * detail_scale)));
            auto existing_font = this->silkscreen_fonts.find(rendered_point_size);
            if (existing_font != this->silkscreen_fonts.end()) {
                return existing_font->second;
            }

            TTF_Font* font = TTF_OpenFont("assets/fonts/IBMPlexMono-Regular.ttf", static_cast<float>(rendered_point_size));
            if (font) {
                this->silkscreen_fonts.emplace(rendered_point_size, font);
            }
            return font;
        }

        const SilkscreenTexture* get_silkscreen_texture(TTF_Font* font, int rendered_point_size, const std::string& text) {
            std::string key = std::to_string(rendered_point_size) + "\n" + text;
            auto existing_texture = this->silkscreen_textures.find(key);
            if (existing_texture != this->silkscreen_textures.end()) {
                return &existing_texture->second;
            }

            SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), 0, SDL_Color{235, 235, 225, 255});
            if (!surface) {
                return nullptr;
            }

            SilkscreenTexture cached_texture{
                SDL_CreateTextureFromSurface(this->renderer, surface),
                surface->w,
                surface->h
            };
            SDL_DestroySurface(surface);
            if (!cached_texture.texture) {
                return nullptr;
            }

            auto inserted_texture = this->silkscreen_textures.emplace(std::move(key), cached_texture).first;
            return &inserted_texture->second;
        }

        void set_regular_font_detail_scale(float detail_scale) {
            if (this->regular_font_detail_scale == detail_scale) {
                return;
            }

            TTF_SetFontSize(this->monofont_regular, 24.0f * detail_scale);
            this->regular_font_detail_scale = detail_scale;
        }

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

        void render_component_pins(const Component& component, float x, float y, float width, float height, float detail_scale) {
            float pin_length = 8.0f * detail_scale;
            float pin_thickness = 2.0f * detail_scale;

            for (uint8_t pin = 0; pin < component.pins_left; pin++) {
                float pin_y = y + height * (static_cast<float>(pin) + 1.0f) /
                    (static_cast<float>(component.pins_left) + 1.0f);
                SDL_FRect pin_rect{x - pin_length, pin_y - pin_thickness * 0.5f, pin_length, pin_thickness};
                SDL_RenderFillRect(this->renderer, &pin_rect);
            }

            for (uint8_t pin = 0; pin < component.pins_right; pin++) {
                float pin_y = y + height * (static_cast<float>(component.pins_right) - static_cast<float>(pin)) /
                    (static_cast<float>(component.pins_right) + 1.0f);
                SDL_FRect pin_rect{x + width, pin_y - pin_thickness * 0.5f, pin_length, pin_thickness};
                SDL_RenderFillRect(this->renderer, &pin_rect);
            }

            for (uint8_t pin = 0; pin < component.pins_top; pin++) {
                float pin_x = x + width * (static_cast<float>(component.pins_top) - static_cast<float>(pin)) /
                    (static_cast<float>(component.pins_top) + 1.0f);
                SDL_FRect pin_rect{pin_x - pin_thickness * 0.5f, y - pin_length, pin_thickness, pin_length};
                SDL_RenderFillRect(this->renderer, &pin_rect);
            }

            for (uint8_t pin = 0; pin < component.pins_bottom; pin++) {
                float pin_x = x + width * (static_cast<float>(pin) + 1.0f) /
                    (static_cast<float>(component.pins_bottom) + 1.0f);
                SDL_FRect pin_rect{pin_x - pin_thickness * 0.5f, y + height, pin_thickness, pin_length};
                SDL_RenderFillRect(this->renderer, &pin_rect);
            }
        }

        void render_pin_one_indicator(float x, float y, float detail_scale) {
            int radius = static_cast<int>(2.0f * detail_scale);
            float center_offset = 3.5f * detail_scale;

            SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 255);
            for (int offset_y = -radius; offset_y <= radius; offset_y++) {
                for (int offset_x = -radius; offset_x <= radius; offset_x++) {
                    if (offset_x * offset_x + offset_y * offset_y <= radius * radius) {
                        SDL_RenderPoint(this->renderer,
                            x + center_offset + static_cast<float>(offset_x),
                            y + center_offset + static_cast<float>(offset_y));
                    }
                }
            }
        }

        bool render_silkscreen(const Component& component, float x, float y, float width, float height, float detail_scale) {
            if (component.silkscreen.empty()) {
                return false;
            }

            float padding = 4.0f * detail_scale;
            float available_width = width - padding * 2.0f;
            float available_height = height - padding * 2.0f;
            if (available_width <= 0.0f || available_height <= 0.0f) {
                return false;
            }

            bool vertical = height > width;
            float maximum_font_height = vertical ? available_width : available_height;
            int largest_point_size = std::max(1, static_cast<int>(std::floor(maximum_font_height / detail_scale)));
            int smallest_point_size = 1;
            int chosen_rendered_point_size = 0;
            TTF_Font* chosen_font = nullptr;

            while (smallest_point_size <= largest_point_size) {
                int point_size = (smallest_point_size + largest_point_size) / 2;
                TTF_Font* font = this->get_silkscreen_font(point_size, detail_scale);
                int text_width = 0;
                int text_height = 0;

                if (!font || !TTF_GetStringSize(font, component.silkscreen.c_str(), 0, &text_width, &text_height)) {
                    return false;
                }

                bool text_fits = vertical
                    ? text_width <= available_height && text_height <= available_width
                    : text_width <= available_width && text_height <= available_height;

                if (text_fits) {
                    chosen_font = font;
                    chosen_rendered_point_size = static_cast<int>(std::ceil(point_size * detail_scale));
                    smallest_point_size = point_size + 1;
                } else {
                    largest_point_size = point_size - 1;
                }
            }

            if (!chosen_font) {
                return false;
            }

            const SilkscreenTexture* texture = this->get_silkscreen_texture(
                chosen_font,
                chosen_rendered_point_size,
                component.silkscreen
            );
            if (!texture) {
                return false;
            }

            if (static_cast<float>(texture->height) * this->zoom / detail_scale < silkscreen_minimum_screen_height) {
                return false;
            }

            float text_width = static_cast<float>(texture->width);
            float text_height = static_cast<float>(texture->height);
            SDL_FRect text_rect{
                x + width * 0.5f - text_width * 0.5f,
                y + height * 0.5f - text_height * 0.5f,
                text_width,
                text_height
            };

            if (vertical) {
                SDL_RenderTextureRotated(this->renderer, texture->texture, nullptr, &text_rect, 90.0, nullptr, SDL_FLIP_NONE);
            } else {
                SDL_RenderTexture(this->renderer, texture->texture, nullptr, &text_rect);
            }
            return true;
        }

        void render_component_label(const Component& component, float x, float y, float width, float detail_scale, bool silkscreen_is_visible) {
            if (silkscreen_is_visible) {
                return;
            }

            TTF_Text* label = TTF_CreateText(this->textengine, this->monofont_regular, component.name.c_str(), 0);
            if (!label) {
                return;
            }

            TTF_SetTextColor(label, 255, 200, 230, 200);
            int label_width = 0;
            int label_height = 0;
            if (TTF_GetTextSize(label, &label_width, &label_height)) {
                float label_x = x + (width - static_cast<float>(label_width)) * 0.5f;
                float label_y = y - static_cast<float>(label_height) - 12.0f * detail_scale;
                TTF_DrawRendererText(label, label_x, label_y);
            }
            TTF_DestroyText(label);
        }

        void render_components(float detail_scale) {
            this->set_regular_font_detail_scale(detail_scale);

            for (const Component& component : this->components) {
                float x = (component.pos_x - this->camera_x) * detail_scale;
                float y = (component.pos_y - this->camera_y) * detail_scale;

                uint8_t larger_horizontal = std::max(component.pins_left, component.pins_right);
                uint8_t larger_vertical = std::max(component.pins_top, component.pins_bottom);
                float component_width = (20.0f + static_cast<float>(larger_vertical) * 4.0f) * detail_scale;
                float component_height = (20.0f + static_cast<float>(larger_horizontal) * 4.0f) * detail_scale;

                SDL_FRect rect = SDL_FRect{x, y, component_width, component_height};
                SDL_SetRenderDrawColor(this->renderer, 68, 72, 82, 255);
                SDL_RenderFillRect(this->renderer, &rect);
                SDL_SetRenderDrawColor(this->renderer, 205, 210, 220, 255);
                SDL_RenderRect(this->renderer, &rect);
                this->render_component_pins(component, x, y, component_width, component_height, detail_scale);
                this->render_pin_one_indicator(x, y, detail_scale);
                bool silkscreen_is_visible = this->render_silkscreen(component, x, y, component_width, component_height, detail_scale);
                this->render_component_label(component, x, y, component_width, detail_scale, silkscreen_is_visible);
            }
        }

        void render_debug_info() {
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
            for (const auto& entry : this->silkscreen_textures) {
                SDL_DestroyTexture(entry.second.texture);
            }
            for (const auto& entry : this->silkscreen_fonts) {
                TTF_CloseFont(entry.second);
            }
            TTF_CloseFont(this->monofont_light);
            TTF_CloseFont(this->monofont_regular);
            TTF_CloseFont(this->monofont_bold);
            TTF_DestroyRendererTextEngine(this->textengine);
            TTF_Quit();
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

                float detail_scale = this->get_detail_scale();
                SDL_SetRenderScale(this->renderer, this->zoom / detail_scale, this->zoom / detail_scale);

                SDL_FRect rect = SDL_FRect{
                    -this->camera_x * detail_scale,
                    -this->camera_y * detail_scale,
                    this->canvas_width * detail_scale,
                    this->canvas_height * detail_scale
                };
                SDL_SetRenderDrawColor(this->renderer, 15, 15, 25, 255);
                SDL_RenderFillRect(this->renderer, &rect);
                this->render_components(detail_scale);

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
            return RuntimeEvent::Closed;
        }
};

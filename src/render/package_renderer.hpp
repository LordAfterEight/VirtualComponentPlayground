#pragma once

#include <string>
#include <unordered_map>

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "core/camera.hpp"
#include "ecs/world.hpp"

class PackageRenderer {
    private:
        struct SilkscreenTexture {
            SDL_Texture* texture;
            int width;
            int height;
        };

        SDL_Renderer* renderer;
        std::unordered_map<int, TTF_Font*> silkscreen_fonts;
        std::unordered_map<std::string, SilkscreenTexture> silkscreen_textures;

        TTF_Font* silkscreen_font(int world_point_size, float detail_scale);
        const SilkscreenTexture* silkscreen_texture(TTF_Font* font, int rendered_point_size, const std::string& text);
        void render_pins(const Package& package, float x, float y, float width, float height, float detail_scale);
        void render_pin_one_indicator(float x, float y, float detail_scale);
        void render_outline(float x, float y, float width, float height, float detail_scale);
        bool render_silkscreen(const std::string& text, float x, float y, float width, float height,
            const Camera& camera, float detail_scale);
        void render_label(const Package& package, float x, float y, float width, float detail_scale,
            bool silkscreen_is_visible);

    public:
        explicit PackageRenderer(SDL_Renderer* renderer);
        ~PackageRenderer();

        void render(const World& world, const Camera& camera, float detail_scale);
};

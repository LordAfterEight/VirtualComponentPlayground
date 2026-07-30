#include "render/package_renderer.hpp"

#include <algorithm>
#include <cmath>

#include "geometry/package_geometry.hpp"

PackageRenderer::PackageRenderer(SDL_Renderer* renderer) : renderer(renderer) {
}

PackageRenderer::~PackageRenderer() {
    for (const auto& entry : this->silkscreen_textures) {
        SDL_DestroyTexture(entry.second.texture);
    }
    for (const auto& entry : this->silkscreen_fonts) {
        TTF_CloseFont(entry.second);
    }
}

TTF_Font* PackageRenderer::silkscreen_font(int world_point_size, float detail_scale) {
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

const PackageRenderer::SilkscreenTexture* PackageRenderer::silkscreen_texture(TTF_Font* font,
    int rendered_point_size, const std::string& text) {
    std::string key = std::to_string(rendered_point_size) + "\n" + text;
    auto existing_texture = this->silkscreen_textures.find(key);
    if (existing_texture != this->silkscreen_textures.end()) {
        return &existing_texture->second;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), 0, SDL_Color{235, 235, 225, 255});
    if (!surface) {
        return nullptr;
    }

    SilkscreenTexture texture{SDL_CreateTextureFromSurface(this->renderer, surface), surface->w, surface->h};
    SDL_DestroySurface(surface);
    if (!texture.texture) {
        return nullptr;
    }

    auto inserted_texture = this->silkscreen_textures.emplace(std::move(key), texture).first;
    return &inserted_texture->second;
}

void PackageRenderer::render_pins(const Package& package, float x, float y, float width, float height, float detail_scale) {
    float pin_length = 8.0f * detail_scale;
    float pin_thickness = 2.0f * detail_scale;
    float grid = PackageGeometry::grid_spacing * detail_scale;

    for (uint8_t pin = 0; pin < package.pins_left; pin++) {
        float pin_y = y + (static_cast<float>(pin) + 1.0f) * grid;
        SDL_FRect rect{x - pin_length, pin_y - pin_thickness * 0.5f, pin_length, pin_thickness};
        SDL_RenderFillRect(this->renderer, &rect);
    }
    for (uint8_t pin = 0; pin < package.pins_right; pin++) {
        float pin_y = y + height - (static_cast<float>(pin) + 1.0f) * grid;
        SDL_FRect rect{x + width, pin_y - pin_thickness * 0.5f, pin_length, pin_thickness};
        SDL_RenderFillRect(this->renderer, &rect);
    }
    for (uint8_t pin = 0; pin < package.pins_top; pin++) {
        float pin_x = x + width - (static_cast<float>(pin) + 1.0f) * grid;
        SDL_FRect rect{pin_x - pin_thickness * 0.5f, y - pin_length, pin_thickness, pin_length};
        SDL_RenderFillRect(this->renderer, &rect);
    }
    for (uint8_t pin = 0; pin < package.pins_bottom; pin++) {
        float pin_x = x + (static_cast<float>(pin) + 1.0f) * grid;
        SDL_FRect rect{pin_x - pin_thickness * 0.5f, y + height, pin_thickness, pin_length};
        SDL_RenderFillRect(this->renderer, &rect);
    }
}

void PackageRenderer::render_pin_one_indicator(float x, float y, float detail_scale) {
    int radius = static_cast<int>(2.0f * detail_scale);
    float center_offset = 5.0f * detail_scale;
    SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 255);
    for (int offset_y = -radius; offset_y <= radius; offset_y++) {
        for (int offset_x = -radius; offset_x <= radius; offset_x++) {
            if (offset_x * offset_x + offset_y * offset_y <= radius * radius) {
                SDL_RenderPoint(this->renderer, x + center_offset + offset_x, y + center_offset + offset_y);
            }
        }
    }
}

void PackageRenderer::render_outline(float x, float y, float width, float height, float detail_scale) {
    float thickness = 2.0f * detail_scale;
    SDL_FRect top{x, y, width, thickness};
    SDL_FRect bottom{x, y + height - thickness, width, thickness};
    SDL_FRect left{x, y, thickness, height};
    SDL_FRect right{x + width - thickness, y, thickness, height};
    SDL_RenderFillRect(this->renderer, &top);
    SDL_RenderFillRect(this->renderer, &bottom);
    SDL_RenderFillRect(this->renderer, &left);
    SDL_RenderFillRect(this->renderer, &right);
}

bool PackageRenderer::render_silkscreen(const std::string& text, float x, float y, float width, float height,
    const Camera& camera, float detail_scale) {
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
        TTF_Font* font = this->silkscreen_font(point_size, detail_scale);
        int text_width = 0;
        int text_height = 0;
        if (!font || !TTF_GetStringSize(font, text.c_str(), 0, &text_width, &text_height)) {
            return false;
        }

        bool fits = vertical ? text_width <= available_height && text_height <= available_width :
            text_width <= available_width && text_height <= available_height;
        if (fits) {
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

    const SilkscreenTexture* texture = this->silkscreen_texture(chosen_font, chosen_rendered_point_size, text);
    if (!texture || static_cast<float>(texture->height) * camera.zoom / detail_scale < 12.0f) {
        return false;
    }

    SDL_FRect text_rect{x + width * 0.5f - texture->width * 0.5f, y + height * 0.5f - texture->height * 0.5f,
        static_cast<float>(texture->width), static_cast<float>(texture->height)};
    if (vertical) {
        SDL_RenderTextureRotated(this->renderer, texture->texture, nullptr, &text_rect, 90.0, nullptr, SDL_FLIP_NONE);
    } else {
        SDL_RenderTexture(this->renderer, texture->texture, nullptr, &text_rect);
    }
    return true;
}

void PackageRenderer::render_label(const Package& package, float x, float y, float width, float detail_scale,
    bool silkscreen_is_visible) {
    if (silkscreen_is_visible) {
        return;
    }

    TTF_Font* font = this->silkscreen_font(24, detail_scale);
    if (!font) {
        return;
    }
    SDL_Surface* surface = TTF_RenderText_Blended(font, package.name.c_str(), 0, SDL_Color{255, 200, 230, 200});
    if (!surface) {
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(this->renderer, surface);
    int label_width = surface->w;
    int label_height = surface->h;
    SDL_DestroySurface(surface);
    if (!texture) {
        return;
    }

    SDL_FRect label_rect{x + (width - label_width) * 0.5f,
        y - label_height - 12.0f * detail_scale,
        static_cast<float>(label_width), static_cast<float>(label_height)};
    SDL_RenderTexture(this->renderer, texture, nullptr, &label_rect);
    SDL_DestroyTexture(texture);
}

void PackageRenderer::render(const World& world, const Camera& camera, float detail_scale) {
    for (Entity entity : world.package_entities()) {
        const Transform& transform = world.transform(entity);
        const Package& package = world.package(entity);
        float world_width = 0.0f;
        float world_height = 0.0f;
        PackageGeometry::body_size(package, world_width, world_height);
        float x = (transform.x - camera.x) * detail_scale;
        float y = (transform.y - camera.y) * detail_scale;
        float width = world_width * detail_scale;
        float height = world_height * detail_scale;

        SDL_FRect body{x, y, width, height};
        SDL_SetRenderDrawColor(this->renderer, 68, 72, 82, 255);
        SDL_RenderFillRect(this->renderer, &body);
        SDL_SetRenderDrawColor(this->renderer, 205, 210, 220, 255);
        this->render_outline(x, y, width, height, detail_scale);
        this->render_pins(package, x, y, width, height, detail_scale);
        this->render_pin_one_indicator(x, y, detail_scale);

        bool silkscreen_is_visible = world.has_silkscreen(entity) &&
            this->render_silkscreen(world.silkscreen(entity).text, x, y, width, height, camera, detail_scale);
        this->render_label(package, x, y, width, detail_scale, silkscreen_is_visible);
    }
}

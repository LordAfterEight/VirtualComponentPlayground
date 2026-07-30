#pragma once

#include <stdint.h>
#include <string>
#include <SDL3_ttf/SDL_ttf.h>

class Component {
    public:
        std::string name;
        float pos_x;
        float pos_y;
        uint8_t pins_top = 0;
        uint8_t pins_bottom = 0;
        uint8_t pins_left = 0;
        uint8_t pins_right = 0;

        Component(float x, float y, std::string name) {
            this->pos_x = x;
            this->pos_y = y;
            this->name = name;
        }
};

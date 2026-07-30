#pragma once

#include <cstdint>
#include <string>

struct Transform {
    float x;
    float y;
};

struct Package {
    std::string name;
    uint8_t pins_top = 0;
    uint8_t pins_bottom = 0;
    uint8_t pins_left = 0;
    uint8_t pins_right = 0;
};

struct Silkscreen {
    std::string text;
};

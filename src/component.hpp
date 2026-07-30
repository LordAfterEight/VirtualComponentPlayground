#include <stdint.h>
#include <string>

class Component {
    std::string name;
    int16_t pos_x;
    int16_t pos_y;
    uint8_t pins_top;
    uint8_t pins_bottom;
    uint8_t pins_left;
    uint8_t pins_right;
};

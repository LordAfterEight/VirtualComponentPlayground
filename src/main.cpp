#include <SDL3/SDL.h>
#include "canvas.hpp"
#include "component.hpp"

int main() {
    Canvas canvas = Canvas(1920, 1080);

    Component testcomponent = Component(100.0, 100.0, "STM32H745BIT6 MCU");
    testcomponent.silkscreen = "STM32H745BIT6";
    testcomponent.pins_right = 54;
    testcomponent.pins_left = 54;
    testcomponent.pins_top = 54;
    testcomponent.pins_bottom = 54;

    canvas.add_component(testcomponent);

    canvas.start();
    return 0;
}

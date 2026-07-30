#include <SDL3/SDL.h>
#include "canvas.hpp"
#include "component.hpp"

int main() {
    Canvas canvas = Canvas(1920, 1080);

    Component testcomponent = Component(100.0, 100.0, "TestComponent");
    testcomponent.silkscreen = "TC-001";
    testcomponent.pins_right = 5;
    testcomponent.pins_left = 5;
    testcomponent.pins_top = 2;
    testcomponent.pins_bottom = 3;

    canvas.add_component(testcomponent);

    canvas.start();
    return 0;
}

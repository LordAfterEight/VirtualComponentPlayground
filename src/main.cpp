#include "app/canvas.hpp"

int main() {
    Canvas canvas = Canvas(1920, 1080);

    World& world = canvas.world();
    Entity test_component = world.create_package(100.0f, 100.0f, "STM32H745BIT6 MCU");
    Package& package = world.package(test_component);
    package.pins_right = 54;
    package.pins_left = 54;
    package.pins_top = 54;
    package.pins_bottom = 54;
    world.add_silkscreen(test_component, "STM32H745BIT6");

    canvas.start();
    return 0;
}

#include <SDL3/SDL.h>
#include "canvas.hpp"
#include "component.hpp"

int main() {
    Canvas canvas = Canvas(1920, 1080);

    Component testcomponent = Component(100.0, 100.0, "TestComponent");

    canvas.add_component(testcomponent);

    canvas.start();
    return 0;
}

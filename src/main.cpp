#include <SDL3/SDL.h>
#include "canvas.hpp"

int main() {
    Canvas canvas = Canvas(1920, 1080);
    canvas.start();
    return 0;
}

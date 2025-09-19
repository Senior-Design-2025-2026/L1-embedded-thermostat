#include <unistd.h>
#include "rpi1306i2c.hpp"

int main(void) {
    ssd1306::Display128x32 screen(1, 0x3C);

    screen.clear();
    screen.drawString(0, 0, "Sensor 1: 23 (C)");
    screen.drawString(0, 16, "Sensor 2: 23 (C)");

    sleep(5);
    screen.clear();
    return 0;
}
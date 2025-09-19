#include <unistd.h>
#include <string>
#include "rpi1306i2c.hpp"

int main() {
    // Initialize the OLED display (I2C bus 1, address 0x3C)
    ssd1306::Display128x32 screen(1, 0x3C);

    // Optional: clear the screen first
    screen.clear();

    // Draw first line: "Sensor 1: 23°C"
    screen.drawStringDouble(0, 0, "Sensor 1: 23 C");

    // Draw second line: "Sensor 2: 25°C"
    screen.drawStringDouble(0, 8, "Sensor 2: 25 C");
    
    display.bufferFlush();

    sleep(10);

    // Clear screen

    screen.clear();

    return 0;
}

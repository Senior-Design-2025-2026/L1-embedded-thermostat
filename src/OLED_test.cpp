#include <unistd.h>
#include <string>
#include "rpi1306i2c.hpp"

int main() {
    // Initialize the OLED display (I2C bus 1, address 0x3C)
    ssd1306::Display128x32 screen(1, 0x3C);

    // Optional: clear the screen first
    screen.clear();

    // Draw first line: "Sensor 1: 23°C"
    screen.drawString(0, 0, "Sensor 1: 23");
    screen.drawChar(6*12, 0, '°'); // 12th character position
    screen.drawChar(6*13, 0, 'C');

    // Draw second line: "Sensor 2: 25°C"
    screen.drawString(0, 8, "Sensor 2: 25");
    screen.drawChar(6*12, 8, '°'); // same x offset
    screen.drawChar(6*13, 8, 'C');

    // Keep it displayed for 5 seconds
    sleep(10);

    // Clear screen
    screen.clear();

    return 0;
}

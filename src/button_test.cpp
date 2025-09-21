#include <iostream>
#include <pigpio.h>

constexpr int BUTTON_PIN = 27; // Use one of your button pins for testing

// Callback function to print a message when the button is pressed
void buttonCallback(int gpio, int level, uint32_t tick) {
    if (level == 1) { // Rising edge
        std::cout << "Button pressed on GPIO " << gpio << std::endl;
    }
}

int main() {
    // Initialize pigpio
    if (gpioInitialise() < 0) {
        std::cerr << "pigpio initialization failed" << std::endl;
        return 1;
    }

    // Set the GPIO pin as an input with a pull-down resistor
    gpioSetMode(BUTTON_PIN, PI_INPUT);
    gpioSetPullUpDown(BUTTON_PIN, PI_PUD_DOWN);

    // Set up the interrupt to call our function on a rising edge
    gpioSetISRFunc(BUTTON_PIN, RISING_EDGE, 0, buttonCallback);

    std::cout << "Listening for button presses on GPIO " << BUTTON_PIN << ". Press Ctrl+C to exit." << std::endl;

    // Keep the program running indefinitely
    while (true) {
        gpioDelay(1000000); // 1-second delay
    }

    // Clean up
    gpioTerminate();
    return 0;
}
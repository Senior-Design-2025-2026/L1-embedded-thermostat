#include <iostream>
#include <wiringPi.h>

constexpr int BUTTON_PIN = 27;

// This function will be called by WiringPi's interrupt handler
void buttonCallback() {
    std::cout << "Button pressed on GPIO " << BUTTON_PIN << std::endl;
}

int main() {
    // Initialize WiringPi
    if (wiringPiSetupGpio() == -1) {
        std::cerr << "WiringPi initialization failed. Please check your installation." << std::endl;
        return 1;
    }

    // Set the button pin as an input with a pull-down resistor
    pinMode(BUTTON_PIN, INPUT);
    pullUpDnControl(BUTTON_PIN, PUD_DOWN);

    // Register the interrupt service routine (ISR)
    // The ISR will trigger on a rising edge (when the button is pressed)
    if (wiringPiISR(BUTTON_PIN, INT_EDGE_RISING, &buttonCallback) < 0) {
        std::cerr << "Unable to set up ISR. Check your permissions." << std::endl;
        return 1;
    }

    std::cout << "Listening for button presses on GPIO " << BUTTON_PIN << ". Press Ctrl+C to exit." << std::endl;

    // A loop to keep the program running
    while (true) {
        delay(1000); // Wait for 1 second
    }

    return 0;
}
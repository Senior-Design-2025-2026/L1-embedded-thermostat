#include <iostream>
#include <wiringPi.h>

constexpr int BUTTON_PIN1 = 27;
constexpr int BUTTON_PIN2 = 22;

const unsigned int DEBOUNCE_DELAY = 100; // milliseconds
unsigned int lastPressTime1 = 0;
unsigned int lastPressTime2 = 0;

void buttonCallback(int buttonPin, unsigned int& lastPressTime) {
    unsigned int currentTime = millis();
    if (currentTime - lastPressTime > DEBOUNCE_DELAY) {
        std::cout << "Button pressed on GPIO " << buttonPin << std::endl;
        lastPressTime = currentTime;
    }
}

void buttonCallback1() {
    buttonCallback(BUTTON_PIN1, lastPressTime1);
}

void buttonCallback2() {
    buttonCallback(BUTTON_PIN2, lastPressTime2);
}

int main() {
    if (wiringPiSetupGpio() == -1) {
        std::cerr << "WiringPi initialization failed." << std::endl;
        return 1;
    }

    // Setup for BUTTON 1 (GPIO 27)
    pinMode(BUTTON_PIN1, INPUT);
    // Corrected to PUD_UP to match your wiring
    pullUpDnControl(BUTTON_PIN1, PUD_UP); 
    // Corrected to INT_EDGE_FALLING to match your wiring
    if (wiringPiISR(BUTTON_PIN1, INT_EDGE_FALLING, &buttonCallback1) < 0) {
        std::cerr << "Unable to set up ISR for BUTTON 1." << std::endl;
        return 1;
    }

    // Setup for BUTTON 2 (GPIO 22)
    pinMode(BUTTON_PIN2, INPUT);
    // Corrected to PUD_UP to match your wiring
    pullUpDnControl(BUTTON_PIN2, PUD_UP);
    // Corrected to INT_EDGE_FALLING to match your wiring
    if (wiringPiISR(BUTTON_PIN2, INT_EDGE_FALLING, &buttonCallback2) < 0) {
        std::cerr << "Unable to set up ISR for BUTTON 2." << std::endl;
        return 1;
    }

    std::cout << "Listening for button presses on GPIOs " << BUTTON_PIN1 << " and " << BUTTON_PIN2 << "." << std::endl;

    while (true) {
        delay(1000);
    }

    return 0;
}
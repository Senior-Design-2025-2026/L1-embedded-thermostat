#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include "rpi1306i2c.hpp"
#include <wiringPi.h>

constexpr int BUTTON_SENSOR1 = 27;
constexpr int BUTTON_SENSOR2 = 22;

const unsigned int DEBOUNCE_DELAY = 100; // milliseconds
volatile unsigned int lastPressTime1 = 0;
volatile unsigned int lastPressTime2 = 0;

volatile bool sensor1Enabled = true;
volatile bool sensor2Enabled = true;

// Callback function for button presses
void buttonCallback(int buttonPin, volatile unsigned int& lastPressTime, volatile bool& sensorEnabled) {
    unsigned int currentTime = millis();
    if (currentTime - lastPressTime > DEBOUNCE_DELAY) {
        sensorEnabled = !sensorEnabled;
        std::cout << "Sensor " << (buttonPin == BUTTON_SENSOR1 ? "1" : "2") << " toggled to " << (sensorEnabled ? "ON" : "OFF") << std::endl;
        lastPressTime = currentTime;
    }
}

void buttonCallback1() {
    buttonCallback(BUTTON_SENSOR1, lastPressTime1, sensor1Enabled);
}

void buttonCallback2() {
    buttonCallback(BUTTON_SENSOR2, lastPressTime2, sensor2Enabled);
}

double readTemperature(const std::string &devicePath) {
    std::ifstream file(devicePath + "/w1_slave");
    std::string line1, line2;

    if (!std::getline(file, line1) || !std::getline(file, line2)) {
        throw std::runtime_error("Failed to read sensor file");
    }

    if (line1.find("YES") == std::string::npos) {
        throw std::runtime_error("CRC check failed");
    }

    auto pos = line2.find("t=");
    if (pos == std::string::npos) {
        throw std::runtime_error("Temperature not found");
    }

    int tempMilliC = std::stoi(line2.substr(pos + 2));
    return tempMilliC / 1000.0;
}

int main() {
    // WiringPi initialization
    if (wiringPiSetupGpio() == -1) {
        std::cerr << "WiringPi initialization failed." << std::endl;
        return 1;
    }

    // Set up buttons for falling edge interrupt with pull-up resistor
    pinMode(BUTTON_SENSOR1, INPUT);
    pullUpDnControl(BUTTON_SENSOR1, PUD_UP);
    if (wiringPiISR(BUTTON_SENSOR1, INT_EDGE_FALLING, &buttonCallback1) < 0) {
        std::cerr << "Unable to set up ISR for BUTTON 1." << std::endl;
        return 1;
    }

    pinMode(BUTTON_SENSOR2, INPUT);
    pullUpDnControl(BUTTON_SENSOR2, PUD_UP);
    if (wiringPiISR(BUTTON_SENSOR2, INT_EDGE_FALLING, &buttonCallback2) < 0) {
        std::cerr << "Unable to set up ISR for BUTTON 2." << std::endl;
        return 1;
    }

    ssd1306::Display128x32 screen(1, 0x3C);
    
    // Timer variables
    unsigned int lastReadTime = 0;
    const unsigned int READ_INTERVAL = 1000; // 1000 milliseconds = 1 second

    // Variables to track display state
    bool lastSensor1Enabled = true;
    bool lastSensor2Enabled = true;

    while (true) {
        unsigned int currentTime = millis();

        // Check if a second has passed since the last measurement
        if (currentTime - lastReadTime >= READ_INTERVAL) {
            // ---- Sensor 1 Logic ----
            if (sensor1Enabled) {
                try {
                    std::string devicePath = "/sys/bus/w1/devices/28-000010eb7a80";
                    double temperature1 = readTemperature(devicePath);
                    std::ostringstream ss1;
                    ss1 << "Sensor 1: " << std::fixed << std::setprecision(2) << temperature1 << " C";
                    screen.drawString(0, 0, ss1.str());
                } catch (const std::exception &e) {
                    screen.drawString(0, 0, "Sensor 1: Unplugged");
                }
            }
            
            // ---- Sensor 2 Logic ----
            if (sensor2Enabled) {
                try {
                    std::string devicePath = "/sys/bus/w1/devices/28-000007292a49";
                    double temperature2 = readTemperature(devicePath);
                    std::ostringstream ss2;
                    ss2 << "Sensor 2: " << std::fixed << std::setprecision(2) << temperature2 << " C";
                    screen.drawString(0, 8, ss2.str());
                } catch (const std::exception &e) {
                    screen.drawString(0, 8, "Sensor 2: Unplugged");
                }
            }
            lastReadTime = currentTime;
        }

        // Check for state changes and update the display immediately
        if (lastSensor1Enabled != sensor1Enabled) {
            screen.clear(); // Clear only when a state change happens
            if (!sensor1Enabled) {
                screen.drawString(0, 0, "Sensor 1: OFF");
            }
            lastSensor1Enabled = sensor1Enabled;
        }
        
        if (lastSensor2Enabled != sensor2Enabled) {
            screen.clear(); // Clear only when a state change happens
            if (!sensor2Enabled) {
                screen.drawString(0, 8, "Sensor 2: OFF");
            }
            lastSensor2Enabled = sensor2Enabled;
        }
    }
    screen.clear();
    return 0;
}
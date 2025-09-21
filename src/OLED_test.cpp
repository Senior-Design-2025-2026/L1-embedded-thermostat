#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include "rpi1306i2c.hpp"
#include <pigpio.h>

constexpr int BUTTON_SENSOR1 = 27;
constexpr int BUTTON_SENSOR2 = 22;

volatile bool sensor1Enabled = true;
volatile bool sensor2Enabled = true;

// Interrupt callbacks
void sensor1Callback(int gpio, int level, uint32_t tick) {
    if (level == 1) {  // Rising edge
        sensor1Enabled = !sensor1Enabled;
        std::cout << "Sensor 1 toggled " << (sensor1Enabled ? "ON" : "OFF") << std::endl;
    }
}

void sensor2Callback(int gpio, int level, uint32_t tick) {
    if (level == 1) {  // Rising edge
        sensor2Enabled = !sensor2Enabled;
        std::cout << "Sensor 2 toggled " << (sensor2Enabled ? "ON" : "OFF") << std::endl;
    }
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
    if (gpioInitialise() < 0) {
        std::cerr << "pigpio initialization failed" << std::endl;
        return 1;
    }

    // Set up buttons with pull-down and attach interrupts
    gpioSetMode(BUTTON_SENSOR1, PI_INPUT);
    gpioSetPullUpDown(BUTTON_SENSOR1, PI_PUD_DOWN);
    gpioSetISRFunc(BUTTON_SENSOR1, RISING_EDGE, 0, sensor1Callback);

    gpioSetMode(BUTTON_SENSOR2, PI_INPUT);
    gpioSetPullUpDown(BUTTON_SENSOR2, PI_PUD_DOWN);
    gpioSetISRFunc(BUTTON_SENSOR2, RISING_EDGE, 0, sensor2Callback);

    ssd1306::Display128x32 screen(1, 0x3C);

    while (true) {
        double temperature1, temperature2;
        bool temperature1Null = false, temperature2Null = false;
        std::string temp1Str, temp2Str;

        if (sensor1Enabled) {
            try {
                std::string devicePath = "/sys/bus/w1/devices/28-000010eb7a80";
                temperature1 = readTemperature(devicePath);
                std::ostringstream ss1;
                ss1 << "Sensor 1: " << std::fixed << std::setprecision(2) << temperature1 << " C";
                temp1Str = ss1.str();
                std::cout << temp1Str << "\n";
            } catch (const std::exception &e) {
                temperature1Null = true;
                temp1Str = "Sensor 1: Unplugged";
                std::cerr << "Error: " << e.what() << "\n";
            }
        } else {
            temp1Str = "Sensor 1: OFF";
        }
        screen.drawString(0, 0, temp1Str);

        if (sensor2Enabled) {
            try {
                std::string devicePath = "/sys/bus/w1/devices/28-000007292a49";
                temperature2 = readTemperature(devicePath);
                std::ostringstream ss2;
                ss2 << "Sensor 2: " << std::fixed << std::setprecision(2) << temperature2 << " C";
                temp2Str = ss2.str();
                std::cout << temp2Str << "\n";
            } catch (const std::exception &e) {
                temperature2Null = true;
                temp2Str = "Sensor 2: Unplugged";
                std::cerr << "Error: " << e.what() << "\n";
            }
        } else {
            temp2Str = "Sensor 2: OFF";
        }
        screen.drawString(0, 8, temp2Str);

        sleep(1);
    }

    screen.clear();
    gpioTerminate();
    return 0;
}

#include <fstream>
#include <iostream>
#include <string>
#include <httplib.h>
#include <json.hpp>
#include <unistd.h>
#include "rpi1306i2c.hpp"
#include <sstream>
#include <iomanip>
#include <wiringPi.h>

using json = nlohmann::json;

constexpr int BUTTON_SENSOR1 = 27;
constexpr int BUTTON_SENSOR2 = 22;

const unsigned int DEBOUNCE_DELAY = 100;
volatile unsigned int lastPressTime1 = 0;
volatile unsigned int lastPressTime2 = 0;

volatile bool sensor1Enabled = false;
volatile bool sensor2Enabled = false;
volatile bool temperature1Null = false;
volatile bool temperature2Null = false;

volatile double lastTemp1 = 0.0;
volatile double lastTemp2 = 0.0;

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
    return tempMilliC / 1000.0;  // convert to °C
}

int main() {
    httplib::Client client("http://localhost:8050");
    ssd1306::Display128x32 screen(1, 0x3C);

    if (wiringPiSetupGpio() == -1) {
        std::cerr << "WiringPi initialization failed." << std::endl;
        return 1;
    }

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
    // Perform initial readings to populate lastTemp variables
    try {
        lastTemp1 = readTemperature("/sys/bus/w1/devices/28-000010eb7a80");
    } catch (const std::exception &e) {
        lastTemp1 = 0.0;
    }
    
    try {
        lastTemp2 = readTemperature("/sys/bus/w1/devices/28-000007292a49");
    } catch (const std::exception &e) {
        lastTemp2 = 0.0;
    }

    unsigned int lastReadTime = 0;
    const unsigned int READ_INTERVAL = 1000;
    bool lastSensor1Enabled = false;
    bool lastSensor2Enabled = false;

    // Display "OFF" with added spaces to clear any leftover characters
    screen.drawString(0, 0, "Sensor 1: OFF     ");
    screen.drawString(0, 8, "Sensor 2: OFF     ");

    while (true) {
        double temperature1;
        double temperature2;
        std::string temp1Str;
        std::string temp2Str;

        unsigned int currentTime = millis();

        if (lastSensor1Enabled != sensor1Enabled) {
            if (!sensor1Enabled) {
                screen.drawString(0, 0, "Sensor 1: OFF       "); // Added spaces
            } else {
                std::ostringstream ss1;
                ss1 << "Sensor 1: " << std::fixed << std::setprecision(2) << lastTemp1 << " C    "; // Added spaces
                screen.drawString(0, 0, ss1.str());
            }
            lastSensor1Enabled = sensor1Enabled;
        }

        if (lastSensor2Enabled != sensor2Enabled) {
            if (!sensor2Enabled) {
                screen.drawString(0, 8, "Sensor 2: OFF        "); // Added spaces
            } else {
                std::ostringstream ss2;
                ss2 << "Sensor 2: " << std::fixed << std::setprecision(2) << lastTemp2 << " C    "; // Added spaces
                screen.drawString(0, 8, ss2.str());
            }
            lastSensor2Enabled = sensor2Enabled;
        }

        if (currentTime - lastReadTime >= READ_INTERVAL) {
            if (sensor1Enabled) {
                try {
                    double temperature1 = readTemperature("/sys/bus/w1/devices/28-000010eb7a80");
                    std::ostringstream ss1;
                    ss1 << "Sensor 1: " << std::fixed << std::setprecision(2) << temperature1 << " C    "; // Added spaces
                    screen.drawString(0, 0, ss1.str());
                    lastTemp1 = temperature1;
                    temperature1Null = false;
                } catch (const std::exception &e) {
                    screen.drawString(0, 0, "Sensor 1: Unplugged "); // Added spaces
                    temperature1Null = true;
                }
            } else {
                temperature1Null = true;
            }
            if (sensor2Enabled) {
                try {
                    double temperature2 = readTemperature("/sys/bus/w1/devices/28-000007292a49");
                    std::ostringstream ss2;
                    ss2 << "Sensor 2: " << std::fixed << std::setprecision(2) << temperature2 << " C    "; // Added spaces
                    screen.drawString(0, 8, ss2.str());
                    lastTemp2 = temperature2;
                    temperature2Null = false;
                } catch (const std::exception &e) {
                    screen.drawString(0, 8, "Sensor 2: Unplugged "); // Added spaces
                    temperature2Null = true;
                }
            } else {
                temperature2Null = true;
            }
            lastReadTime = currentTime;

            json json_data;
            if (temperature1Null) {
                json_data["sensor1Temperature"] = nullptr;
            } else {
                json_data["sensor1Temperature"] = temperature1;
            }

            if (temperature2Null) {
                json_data["sensor2Temperature"] = nullptr;
            } else {
                json_data["sensor2Temperature"] = temperature2;
            }

            auto res = client.Post("/temperatureData", json_data.dump(), "application/json");

            if (res) {
                std::cout << "Response Status: " << res->status << std::endl;
                std::cout << "Response Body: " << res->body << std::endl;
            } else {
                std::cout << "Error: " << res.error() << std::endl;
            }

            json data = json::parse(res->body);

            for (auto &val : data) {
                std::cout << val << std::endl;
            }
        }
    }
}
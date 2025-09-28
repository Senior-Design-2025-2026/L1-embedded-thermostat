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

const int BUTTON_SENSOR1 = 27;
const int BUTTON_SENSOR2 = 22;
const int POWER_SWITCH = 23;

// Need volatile variables to support interrupts
volatile bool sensor1Enabled = false;
volatile bool sensor2Enabled = false;

bool systemActive = false;
bool lastSystemActive = false;

const unsigned int DEBOUNCE_DELAY = 100;
volatile unsigned int lastPressTime1 = 0;
volatile unsigned int lastPressTime2 = 0;

// Change the sensor variable
void buttonCallback(int buttonPin, volatile unsigned int& lastPressTime, volatile bool& sensorEnabled) {
    // If the switch is off, ignore
    if (!systemActive) return; 
    unsigned int currentTime = millis();
    if (currentTime - lastPressTime > DEBOUNCE_DELAY) {
        sensorEnabled = !sensorEnabled;
        std::cout << "Sensor " << (buttonPin == BUTTON_SENSOR1 ? "1" : "2") << " toggled to " << (sensorEnabled ? "ON" : "OFF") << std::endl;
        lastPressTime = currentTime;
    }
}

// ISR for button 1
void button1Interrupt() {
    buttonCallback(BUTTON_SENSOR1, lastPressTime1, sensor1Enabled);
}

// ISR for button 2
void button2Interrupt() {
    buttonCallback(BUTTON_SENSOR2, lastPressTime2, sensor2Enabled);
}

// Simple helper to change the unit of measurement
std::string changeUnits(std::string unit) {
    if (unit == "C") {
        return "F";
    } else {
        return "C";
    }
}

// Reads the temperature from the given device
double readTemperature(const std::string &devicePath) {
    std::ifstream file(devicePath + "/w1_slave");
    std::string line1, line2;

    // Error checking the read
    if (!std::getline(file, line1) || !std::getline(file, line2)) {
        throw std::runtime_error("Failed to read sensor file");
    }

    // CRC check
    if (line1.find("YES") == std::string::npos) {
        throw std::runtime_error("CRC check failed");
    }

    // Check that the temperature exists
    size_t tEqualsPosition = line2.find("t=");
    if (tEqualsPosition == std::string::npos) {
        throw std::runtime_error("Temperature not found");
    }

    int milliCelsius = std::stoi(line2.substr(tEqualsPosition + 2));
    // Convert to degrees Celsius
    return milliCelsius / 1000.0;  
}

int main() {
    // Listen on local port 8050
    httplib::Client client("http://localhost:8050");

    // Screen initialization
    ssd1306::Display128x32 screen(1, 0x3C);
    screen.clear();

    if (wiringPiSetupGpio() == -1) {
        std::cerr << "WiringPi initialization failed." << std::endl;
        return 1;
    }

    // Set GPIO pin to input
    pinMode(BUTTON_SENSOR1, INPUT);
    pullUpDnControl(BUTTON_SENSOR1, PUD_UP);

    // Configure falling edge interrupt for pushbutton 1
    if (wiringPiISR(BUTTON_SENSOR1, INT_EDGE_FALLING, &button1Interrupt) < 0) {
        std::cerr << "Unable to set up ISR for BUTTON 1." << std::endl;
        return 1;
    }

    // Set GPIO pin to input
    pinMode(BUTTON_SENSOR2, INPUT);
    pullUpDnControl(BUTTON_SENSOR2, PUD_UP);

    // Configure falling edge interrupt for pushbutton 2
    if (wiringPiISR(BUTTON_SENSOR2, INT_EDGE_FALLING, &button2Interrupt) < 0) {
        std::cerr << "Unable to set up ISR for BUTTON 2." << std::endl;
        return 1;
    }

    unsigned int lastReadTime = 0;
    const unsigned int READ_INTERVAL = 1000;
    bool lastSensor1Enabled = false;
    bool lastSensor2Enabled = false;

    // Keeping track of the units to display
    std::string unit = "C";

    // Display "OFF" with added spaces to clear any leftover characters
    // Sensors are assumed to start off
    screen.drawString(0, 0, "Sensor 1: OFF     ");
    screen.drawString(0, 8, "Sensor 2: OFF     ");

    // Set Switch GPIO pin to input - low is off
    pinMode(POWER_SWITCH, INPUT);
    pullUpDnControl(POWER_SWITCH, PUD_DOWN);

    systemActive = (digitalRead(POWER_SWITCH) == HIGH);
    lastSystemActive = systemActive;

    // If switch begins in active state, display sensors in OFF position (Mode they are in when system starts up)
    if (systemActive) {
        screen.drawString(0, 0, "Sensor 1: OFF     ");
        screen.drawString(0, 8, "Sensor 2: OFF     ");
    }   
    // If switch begins in off state, do not display on OLED
    else {
        screen.clear();
    }

    while (true) {
        // Get the current time (used to only read once per second)
        unsigned int currentTime = millis();

        // poll the switch every loop to check state
        systemActive = (digitalRead(POWER_SWITCH) == HIGH);

        if (!systemActive) {
            // If just switched OFF, blank the screen once
            if (lastSystemActive) {
                screen.clear();
                std::cout << "System OFF" << std::endl;
            }

            // Set previous state to off so next cycle system is active
            lastSystemActive = false;
            usleep(100000);

            // If one second has elapsed
            if (currentTime - lastReadTime >= READ_INTERVAL) {
                // Create JSON object to send to server
                json json_data;
                
                json_data["sensor1Temperature"] = nullptr;
                json_data["sensor2Temperature"] = nullptr;

                auto res = client.Post("/temperatureData", json_data.dump(), "application/json");

                // Update lastReadTime
                lastReadTime = currentTime;
            }

            continue;       // skip the rest of the loop
        }

        // Set last system active to true
        lastSystemActive = true;

        double temperature1 = 0.0;
        double temperature2 = 0.0;
        bool temperature1Null;
        bool temperature2Null;

        // In case this has been changed from the interrupt (tough to implement at the interrupt level)
        if (lastSensor1Enabled != sensor1Enabled) {
            if (!sensor1Enabled) {
                screen.drawString(0, 0, "Sensor 1: OFF       ");
            } else {
                std::ostringstream ss1;
                ss1 << "Sensor 1: " << std::fixed << std::setprecision(2) << temperature1 << " C    ";
                screen.drawString(0, 0, ss1.str());
            }
            lastSensor1Enabled = sensor1Enabled;
        }

        if (lastSensor2Enabled != sensor2Enabled) {
            if (!sensor2Enabled) {
                screen.drawString(0, 8, "Sensor 2: OFF        "); 
            } else {
                std::ostringstream ss2;
                ss2 << "Sensor 2: " << std::fixed << std::setprecision(2) << temperature1 << " C    ";
                screen.drawString(0, 8, ss2.str());
            }
            lastSensor2Enabled = sensor2Enabled;
        }

        // If one second has elapsed
        if (currentTime - lastReadTime >= READ_INTERVAL) {
            // If the sensor is on, get a reading
            if (sensor1Enabled) {
                try {
                    temperature1 = readTemperature("/sys/bus/w1/devices/28-000010eb7a80");

                    // Set upper and lower bounds
                    if (temperature1 > 63) {
                        temperature1 = 63;
                    } else if (temperature1 < -10) {
                        temperature1 = -10;
                    }
                    // Need a different variable to handle the conversion, because celsius value must be preserved
                    double tempTemp1 = temperature1;
                    // Stream for the screen
                    std::ostringstream ss1;

                    // Potential conversion to Fahrenheit
                    if (unit == "F") {
                        tempTemp1 = tempTemp1 * 9 / 5.0 + 32;
                    }

                    ss1 << "Sensor 1: " << std::fixed << std::setprecision(2) << tempTemp1 << " " << unit << "    ";
                    screen.drawString(0, 0, ss1.str());
                    temperature1Null = false;
                } catch (const std::exception &e) {
                    // If the sensor is supposed to be on, but no reading is found, the sensor has been unplugged
                    screen.drawString(0, 0, "Sensor 1: Unplugged ");
                    temperature1Null = true;
                }
            } else {
                screen.drawString(0, 0, "Sensor 1: OFF       ");
                temperature1Null = true;
            }
            if (sensor2Enabled) {
                try {
                    temperature2 = readTemperature("/sys/bus/w1/devices/28-000007292a49");
                    // Set upper and lower bounds
                    if (temperature2 > 63) {
                        temperature2 = 63;
                    } else if (temperature2 < -10) {
                        temperature2 = -10;
                    }

                    double tempTemp2 = temperature2;

                    if (unit == "F") {
                        tempTemp2 = tempTemp2 * 9 / 5.0 + 32;
                    }

                    std::ostringstream ss2;
                    ss2 << "Sensor 2: " << std::fixed << std::setprecision(2) << tempTemp2 << " " << unit << "    ";
                    screen.drawString(0, 8, ss2.str());
                    temperature2Null = false;
                } catch (const std::exception &e) {
                    screen.drawString(0, 8, "Sensor 2: Unplugged ");
                    temperature2Null = true;
                }
            } else {
                screen.drawString(0, 8, "Sensor 2: OFF       ");
                temperature2Null = true;
            }

            // Update lastReadTime
            lastReadTime = currentTime;

            // Create JSON object to send to server
            json json_data;
            
            // ALWAYS SEND TEMPERATURE IN CELSIUS- THE SERVER WILL HANDLE CONVERSIONS
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
                // Parse the JSON array
                json j = json::parse(res->body);

                // bool sensor1Enabled = j[1].get<bool>();
                // bool sensor2Enabled = j[2].get<bool>();
                std::cout << "Response Status: " << res->status << std::endl;
                std::cout << "Response Body: " << res->body << std::endl;

                // Check for change in units
                if (j[0].get<std::string>() != unit) {
                    unit = changeUnits(unit);
                }

                // Check for change in sensor 1 status
                if (j[1].get<bool>()) {
                    buttonCallback(BUTTON_SENSOR1, lastPressTime1, sensor1Enabled);
                }

                // Check for change in sensor 2 status
                if (j[2].get<bool>()) {
                    buttonCallback(BUTTON_SENSOR2, lastPressTime2, sensor2Enabled);
                }
            } else {
                std::cout << "Error: " << res.error() << std::endl;
            }
        }
    }
}
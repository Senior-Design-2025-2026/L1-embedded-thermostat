#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include "rpi1306i2c.hpp"
#include <gpiod.h>

constexpr int BUTTON_SENSOR1 = 27;
constexpr int BUTTON_SENSOR2 = 22;

bool sensor1Enabled = true;
bool sensor2Enabled = true;

double readTemperature(const std::string &devicePath) {
    std::ifstream file(devicePath + "/w1_slave");
    std::string line1, line2;

    if (!std::getline(file, line1) || !std::getline(file, line2)) {
        throw std::runtime_error("Failed to read sensor file");
    }

    // Check CRC line
    if (line1.find("YES") == std::string::npos) {
        throw std::runtime_error("CRC check failed");
    }

    // Extract "t=..."
    auto pos = line2.find("t=");
    if (pos == std::string::npos) {
        throw std::runtime_error("Temperature not found");
    }

    int tempMilliC = std::stoi(line2.substr(pos + 2));
    return tempMilliC / 1000.0;  // convert to °C
}

gpiod_line* setupButton(struct gpiod_chip* chip, int gpio) {
    gpiod_line* line = gpiod_chip_get_line(chip, gpio);
    if (!line) throw std::runtime_error("Failed to get GPIO line");

    if (gpiod_line_request_both_edges_events(line, "sensorButton") < 0)
        throw std::runtime_error("Failed to request edge events");

    return line;
}

void checkButton(gpiod_line* line, bool &sensorEnabled, const std::string &sensorName, int timeoutMs) {
    struct timespec timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_nsec = (timeoutMs % 1000) * 1000000;

    struct gpiod_line_event event;
    int ret = gpiod_line_event_wait(line, &timeout);
    if (ret > 0) {
        gpiod_line_event_read(line, &event);
        if (event.event_type == GPIOD_LINE_EVENT_RISING || event.event_type == GPIOD_LINE_EVENT_FALLING) {
            sensorEnabled = !sensorEnabled;
            std::cout << sensorName << " toggled " << (sensorEnabled ? "ON" : "OFF") << std::endl;
        }
    }
}

int main() {
    ssd1306::Display128x32 screen(1, 0x3C);
    struct gpiod_chip* chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) {
        std::cerr << "Failed to open GPIO chip" << std::endl;
        return 1;
    }

    gpiod_line* button1 = setupButton(chip, BUTTON_SENSOR1);
    gpiod_line* button2 = setupButton(chip, BUTTON_SENSOR2);

    const int POLL_INTERVAL_MS = 50;   // check buttons every 50ms
    const int READ_INTERVAL_MS = 1000; // read sensors every 1 second
    int elapsed = 0;

    while (true) {
        double temperature1;
        double temperature2;
        bool temperature1Null = false;
        bool temperature2Null = false;
        std::string temp1Str;
        std::string temp2Str;

        // poll buttons
        checkButton(button1, sensor1Enabled, "Sensor 1", POLL_INTERVAL_MS);
        checkButton(button2, sensor2Enabled, "Sensor 2", POLL_INTERVAL_MS);

        usleep(POLL_INTERVAL_MS * 1000);
        elapsed += POLL_INTERVAL_MS;

        // read sensors every 1 second
        if (elapsed >= READ_INTERVAL_MS) {
            elapsed = 0;

            screen.clear();

            if (sensor1Enabled) {
                try {
                    std::string devicePath = "/sys/bus/w1/devices/28-000010eb7a80"; 
                    temperature1 = readTemperature(devicePath);
                    std::ostringstream ss1;
                    ss1 << "Sensor 1: " << std::fixed << std::setprecision(2) << temperature1 << " C   ";
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
                    ss2 << "Sensor 2: " << std::fixed << std::setprecision(2) << temperature2 << " C   ";
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
        }
    }

    gpiod_line_release(button1);
    gpiod_line_release(button2);
    gpiod_chip_close(chip);
    screen.clear();
}

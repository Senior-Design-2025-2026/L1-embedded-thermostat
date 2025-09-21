#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include "rpi1306i2c.hpp"

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

int main() {
    ssd1306::Display128x32 screen(1, 0x3C);

    while (true) {
        double temperature1;
        double temperature2;
        bool temperature1Null = false;
        bool temperature2Null = false;
        std::string temp1Str;
        std::string temp2Str;

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

        screen.drawString(0, 0, temp1Str);

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

        screen.drawString(0, 8, temp2Str);
    }
    screen.clear()
}

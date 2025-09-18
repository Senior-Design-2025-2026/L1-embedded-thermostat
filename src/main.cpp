#include <fstream>
#include <iostream>
#include <string>
#include <httplib.h>
#include <json.hpp>
#include <wiringPiI2C.h>

// Include the C++ wrapper for the u8g2 library
#include "u8g2/cppsrc/U8g2lib.h"

// The display object
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE); // The U8G2 constructor

// Function to initialize the OLED
void init_oled() {
    u8g2.begin();
    u8g2.setFont(u8g2_font_unifont_tf);
}

// Function to display text on the OLED
void display_on_oled(const std::string &text, int line) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_unifont_tf);
    u8g2.drawStr(0, 10 + line * 10, text.c_str());
    u8g2.sendBuffer();
}

using json = nlohmann::json;

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
    return tempMilliC / 1000.0; // convert to °C
}

int main() {
    httplib::Client client("http://localhost:8050");

    // Initialize the OLED display
    init_oled();

    while (true) {
        double temperature1 = -999.0;
        double temperature2 = -999.0;
        
        try {
            std::string devicePath = "/sys/bus/w1/devices/28-000010eb7a80";
            temperature1 = readTemperature(devicePath);
            std::cout << "Temperature 1: " << temperature1 << " °C\n";
        } catch (const std::exception &e) {
            std::cerr << "Error reading sensor 1: " << e.what() << "\n";
        }

        try {
            std::string devicePath = "/sys/bus/w1/devices/28-000007292a49";
            temperature2 = readTemperature(devicePath);
            std::cout << "Temperature 2: " << temperature2 << " °C\n";
        } catch (const std::exception &e) {
            std::cerr << "Error reading sensor 2: " << e.what() << "\n";
        }

        json json_data;
        json_data["sensor1Temperature"] = temperature1;
        json_data["sensor2Temperature"] = temperature2;

        auto res = client.Post("/temperatureData", json_data.dump(), "application/json");

        if (res) {
            std::cout << "Response Status: " << res->status << std::endl;
            std::cout << "Response Body: " << res->body << std::endl;
        } else {
            std::cout << "Error: " << res.error() << std::endl;
        }

        // Clear the OLED and display new temperature values
        u8g2.clearBuffer();
        
        std::string temp1_str = "Temp 1: " + std::to_string(temperature1).substr(0, 5) + " C";
        std::string temp2_str = "Temp 2: " + std::to_string(temperature2).substr(0, 5) + " C";

        u8g2.setFont(u8g2_font_unifont_tf);
        u8g2.drawStr(0, 10, temp1_str.c_str());
        u8g2.drawStr(0, 20, temp2_str.c_str());
        u8g2.sendBuffer();
        
        sleep(1);
    }
}
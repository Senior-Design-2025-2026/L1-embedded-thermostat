#include <fstream>
#include <iostream>
#include <string>
#include <httplib.h>
#include <json.hpp>
#include <wiringPiI2C.h>
#include <ArduiPi_OLED.h> // Include the new library

// I2C address for the OLED
#define I2C_ADDR 0x3c

// Function to initialize the OLED
void init_oled(ArduiPi_OLED &oled) {
    oled.begin();
    oled.clearDisplay();
    oled.display();
}

// Function to display text on the OLED
void display_on_oled(ArduiPi_OLED &oled, const std::string &text, int line) {
    oled.setTextSize(1);
    oled.setTextColor(WHITE);
    oled.setCursor(0, line * 10);
    oled.print(text.c_str());
    oled.display();
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
    ArduiPi_OLED oled;
    init_oled(oled);

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
        oled.clearDisplay();
        
        std::string temp1_str = "Temp 1: " + std::to_string(temperature1).substr(0, 5) + " C";
        std::string temp2_str = "Temp 2: " + std::to_string(temperature2).substr(0, 5) + " C";

        display_on_oled(oled, temp1_str, 0); // Display on line 0
        display_on_oled(oled, temp2_str, 1); // Display on line 1
        
        sleep(1);
    }
}
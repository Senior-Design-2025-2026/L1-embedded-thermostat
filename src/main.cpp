#include <fstream>
#include <iostream>
#include <string>
#include <httplib.h>
#include <json.hpp>


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
    return tempMilliC / 1000.0;  // convert to °C
}

int main() {
    httplib::Client client("http://localhost:8050");

    while (true) {
        double temperature1;
        double temperature2;
        bool temperature1Null = false;
        bool temperature2Null = false;

        try {
            std::string devicePath = "/sys/bus/w1/devices/28-000010eb7a80"; 
            temperature1 = readTemperature(devicePath);
            std::cout << "Temperature: " << temperature1 << " °C\n";
        } catch (const std::exception &e) {
            temperature1Null = false;
            std::cerr << "Error: " << e.what() << "\n";
        }

        try {
            std::string devicePath = "/sys/bus/w1/devices/28-000007292a49"; 
            temperature2 = readTemperature(devicePath);
            std::cout << "Temperature: " << temperature2 << " °C\n";
        } catch (const std::exception &e) {
            temperature2Null = false;
            std::cerr << "Error: " << e.what() << "\n";
        }

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

        sleep(1);
    }
}
#include <fstream>
#include <iostream>
#include <string>
#include <httplib.h>
#include <cstdlib>
#include <ctime>

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
    srand(time(0));

    httplib::Client client("http://localhost:8050");

    while (true) {
        int temperature1 = rand() % 41 + 10;
        int temperature2 = rand() % 41 + 10;

        try {
            std::string devicePath = "/sys/bus/w1/devices/28-000007292a49"; 
            double tempC = readTemperature(devicePath);
            std::cout << "Temperature: " << tempC << " °C\n";
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << "\n";
        }

        std::string json_data = "{\"sensor1Temperature\": \"" + std::to_string(temperature1) + "\", \"sensor2Temperature\": \"" + std::to_string(temperature2) + "\"}";

        auto res = client.Post("/temperatureData", json_data, "application/json");

        if (res) {
            std::cout << "Response Status: " << res->status << std::endl;
            std::cout << "Response Body: " << res->body << std::endl;
        } else {
            std::cout << "Error: " << res.error() << std::endl;
        }

        sleep(1);
    }
}
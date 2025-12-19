# ESP32 Automated Sprinkler System

A comprehensive, web-based irrigation controller for the ESP32, featuring automated scheduling, weather-based rain delays, water usage monitoring, and cloud integration via Firebase.

See the video Series here at https://www.youtube.com/playlist?list=PLK_zQ6_j4TF2hAYxrCdtLyZ5UvhFppnFG

## Features

*   **Web-Based Dashboard:** Responsive UI for status monitoring, manual controls, and configuration.
*   **Data Visualization:** Dedicated Consumption page with usage charts.
*   **Utilization Management:** Dedicated Utilization page to view history, export data to CSV, and clear records.
*   **Zone Control:** Supports up to 16 zones using a PF575 I2C GPIO Expander.
*   **Flexible Scheduling:** 3 independent programs (A, B, C) with customizable start times, active days, and per-zone durations.
*   **Smart Rain Delay:**
    *   Physical Rain Sensor support.
    *   **OpenWeatherMap Integration:** Automatically skips runs if the chance of rain (POP) exceeds a user-defined threshold (default 75%).
*   **Water Metering:** Tracks water consumption (Gallons/Litres) using pulse-based flow meters.
*   **Cloud History:** Logs run history and water usage to Google Firebase Realtime Database.
*   **Hardware Integration:**
    *   Motorized Ball Valve control (Open/Close/Status).
    *   I2C LCD Display (20x4) for local status.
    *   Rotary Encoder for local menu navigation.
*   **Local Logging:** Event logging to SPIFFS with auto-rotation to prevent storage exhaustion.

## Hardware Requirements

*   **ESP32 Development Board**
*   **PF575 I2C GPIO Expander** (for zone solenoids)
*   **Relay Board** (driven by PF575)
*   **Motorized Ball Valve** (5-wire type supported)
*   **Flow Meter** (Pulse output)
*   **I2C LCD Display** (20x4 recommended)
*   **Rotary Encoder** (for local control)
*   **Rain Sensor** (Optional)

## Software Dependencies

This project is built using **PlatformIO**. Key libraries include:

*   `ESPAsyncWebServer` & `AsyncTCP`
*   `ArduinoJson`
*   `Firebase ESP Client`
*   `LiquidCrystal_PCF8574`
*   `Ai Esp32 Rotary Encoder`
*   `NTPClient`

## Installation

1.  **Clone the Repository** and open in VS Code with PlatformIO.
2.  **Upload Filesystem:** Run the `Upload Filesystem Image` task to flash the `data` folder (HTML/CSS/JS) to the ESP32 SPIFFS.
3.  **Upload Firmware:** Build and Upload the firmware to the ESP32.

## Configuration

1.  **Initial WiFi Setup:** Connect to the `sprinklernet` AP and browse to `192.168.4.1` to configure local WiFi.
2.  **System Config:** Access `http://sprinkler32.local`.
    *   **Main Config:** Setup GPIOs, Firebase URL/Secret, OpenWeatherMap API Key, Rain Cutoff %, and Zones.
    *   **Programmes:** Configure Schedules A, B, and C.

## License

Copyright © 2023-2026 Jordan Rubin.

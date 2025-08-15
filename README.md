# RC Car with Raspberry Pi Pico W

This project is an internet-controlled RC car powered by the Raspberry Pi Pico W. It drives a DC motor and a servo, and is controlled remotely via a simple web server running directly on the Pico W.

## Features

- **DC Motor Control:** Uses a TB6612FNG H-Bridge driver for forward, reverse, and brake functions.
- **Servo Control:** (Stubbed in code, ready for expansion.)
- **WiFi Connectivity:** Connects to your WiFi network using credentials in `wifi.h`.
- **Web Server:** Runs a simple server on the Pico W for remote control.
- **Status LED:** Blinks to indicate connection and activity.

## File Overview

- `picow_wifi_scan.cpp`: Main application. Initializes WiFi, sets up the motor and servo, and runs the main control loop.
- `include/SparkFun_TB6612FNG/SparkFun_TB6612.h`: Motor driver class and function declarations.
- `include/SparkFun_TB6612FNG/SparkFun_TB6612.cpp`: Motor driver implementation using Pico SDK GPIO and PWM.
- `wifi.h`: Stores your WiFi SSID and password (not included for security).
- `CMakeLists.txt`: Build configuration for the Pico SDK and project sources.

## Hardware

- **Raspberry Pi Pico W**
- **TB6612FNG H-Bridge Motor Driver**
- **DC Motor**
- **Servo Motor**
- **RC Car chassis and power supply**

## Getting Started

1. **Clone the repository** and add your WiFi credentials to `wifi.h`:
    ```cpp
    char ssid[] = "YOUR_SSID";
    char pass[] = "YOUR_PASSWORD";
    ```

2. **Build the project** using CMake and Ninja:
    ```powershell
    cd build
    cmake -S .. -B .
    ninja
    ```

3. **Flash the Pico W** with the generated `.uf2` file.

4. **Connect the hardware**:
    - Motor and servo pins are defined in `picow_wifi_scan.cpp` (e.g., motor uses GPIO 26, 27, 4, 5).
    - Ensure correct wiring to the TB6612FNG and servo.

5. **Control the car** via the web server running on the Pico W’s IP address.

## Code Structure

- **Motor Control:** The `Motor` class wraps GPIO and PWM functions for easy motor control.
- **Main Loop:** Handles WiFi connection, blinks the status LED, and calls motor/servo control functions.
- **Server Logic:** (Stubbed, ready for expansion to handle HTTP requests for remote control.)

## Notes

- The servo control loop is currently a stub; expand as needed for steering.
- Use an oscilloscope for accurate PWM signal measurement.
- Ensure your power supply can handle the motor and servo current.

## License

BSD-3-Clause (see source headers for details)

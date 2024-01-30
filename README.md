# TNET - Greenhouse Monitor

This repository contains firmware for a custom ESP32 IoT system for monitoring greenhouse conditions over MQTT, submitted as part of my final computing project.

The hardware itself consists of a custom PCB containing a temperature, pressure, humidity and Co2 sensor. Measuring and sending values over a pre-configured Wi-Fi connection using TLS for security.

## Installation

This project was designed using the Arduino framework for the [ESP32](https://github.com/espressif/arduino-esp32), incorperating [PlatformIO](https://github.com/platformio/platformio-core) for dependency and build management.

To generate a binary run `pio run` from the root directory. 
Alternatively you can run `pio run -t upload` (specifying your upload port) to flash the device directly.

## Formatting

This project was formatted using [Clang Format](https://clang.llvm.org/docs/ClangFormat.html). Please see the included `.clang-format` configuration file for futher details.
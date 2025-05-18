# ESP32 Web Server for LED Control

This project creates a simple Wi-Fi web server using the **ESP32** (with ESP-IDF) to control two LEDs (Green and Red) connected to GPIO16 and GPIO17. The interface is styled using HTML and CSS, and the ESP32 runs in SoftAP mode so users can connect directly to it.

---

## Project Preview

- ESP32 acts as a Wi-Fi hotspot
- Web interface to control Green and Red LEDs
- LED states displayed and toggleable
- Modern styled interface with responsive design

---

## Hardware Setup

| Component       | Connection             |
|----------------|------------------------|
| ESP32 Pin 16   | → 220Ω → Green LED → GND |
| ESP32 Pin 17   | → 220Ω → Red LED → GND   |

**Power:** Use a USB cable or battery to power the ESP32.

---

## Software Overview

### Features:

- SoftAP mode (ESP32 creates its own Wi-Fi)
- HTML page served from ESP32
- Two buttons to control LEDs
- LED status shown on the page
- Non-volatile storage of LED state 

### File Structure

![image](https://github.com/user-attachments/assets/7b7d40bc-d9d2-4a19-ab71-8f4ae952e32e)

## How to Build and Flash

### Prerequisites

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- ESP32 board
- USB cable

### Build Instructions

```bash
idf.py set-target esp32
idf.py menuconfig   # (Optional) configure project
idf.py build
idf.py -p /dev/ttyUSB0 flash
idf.py monitor

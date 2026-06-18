# Smart Glove — Gesture-Controlled Slide/Photo Navigation

A wearable glove that detects hand tilt gestures to navigate slides and photos on a laptop wirelessly.

## Project Overview

A wrist-mounted MPU6050 accelerometer detects hand tilt (LEFT / CENTER / RIGHT / FORWARD). A PIC16F877A microcontroller reads the sensor, displays the state on a 16×2 LCD, lights up a traffic light module, and sends a UART command to an ESP32-CAM. The ESP32-CAM forwards the command over WiFi to a Python script running on a laptop, which simulates keyboard presses to scroll through slides and photos.

## Gesture Mapping

| Gesture | LCD Display | Traffic Light | Key Press | Action |
|---|---|---|---|---|
| Hand flat | CENTER | Green | — | No action |
| Roll wrist left | LEFT <<< | Yellow | ← Left arrow | Previous slide/photo |
| Roll wrist right | RIGHT >>> | Red | → Right arrow | Next slide/photo |
| Tilt fingers down | EXIT vvv | All three on | Escape | Exit slideshow |

## Hardware Components

- **PIC16F877A** — Main microcontroller (20 MHz crystal, XC8 compiler, MPLAB X IDE)
- **MPU6050** — Accelerometer/gyroscope (using X-axis and Y-axis accelerometer data)
- **16×2 LCD** — Parallel 4-bit mode on PORTD
- **Traffic Light Module** — 3 LEDs (Red, Yellow, Green) on PORTB
- **ESP32-CAM** — WiFi bridge (AI Thinker board, Arduino IDE 2.x, ESP32 board package 2.0.17)
- **20 MHz Crystal** + 2× 22pF capacitors
- **10kΩ resistors** — I2C pull-ups and UART voltage divider
- **Potentiometer** — LCD contrast adjustment

## System Architecture

```
MPU6050 --[I2C]--> PIC16F877A --[UART 9600]--> Voltage Divider ---> ESP32-CAM --[WiFi]--> Python Script ---> Keyboard Action
                       |                                                                      |
                   LCD + LEDs                                                             Laptop (Slides/Photos)
```

## Pin Connections

### PIC16F877A
| Pin | Function | Connection |
|---|---|---|
| RC3 (Pin 18) | SCL | MPU6050 SCL + 10kΩ pull-up to 3.3V |
| RC4 (Pin 23) | SDA | MPU6050 SDA + 10kΩ pull-up to 3.3V |
| RC6 (Pin 25) | UART TX | 10kΩ → voltage divider → ESP32-CAM GPIO 13 |
| RD0 (Pin 19) | LCD RS | LCD pin 4 |
| RD1 (Pin 20) | LCD EN | LCD pin 6 |
| RD4-RD7 (Pins 27-30) | LCD D4-D7 | LCD pins 11-14 |
| RB0 (Pin 33) | Red LED | Traffic light Red |
| RB1 (Pin 34) | Yellow LED | Traffic light Yellow |
| RB2 (Pin 35) | Green LED | Traffic light Green |

### UART Voltage Divider (5V → 3.3V)
```
PIC TX (RC6, 5V) ---[10kΩ]--- Junction ---[10kΩ]---[10kΩ]--- GND
                                  |
                            ESP32-CAM GPIO 13
```

## Power Supplies

- **5V / 2A** → PIC, LCD, Traffic Light, ESP32-CAM
- **3.3V / 0.18A** → MPU6050 only
- Common GND rail shared between both supplies

## Repository Structure

```
SmartGlove/
├── README.md
├── PIC/
│   └── main_v6.c              # PIC16F877A firmware
├── ESP32/
│   └── SmartGlove_WiFi.ino    # ESP32-CAM WiFi bridge
├── Python/
│   └── smartglove.py          # Laptop-side keyboard controller
└── Docs/
    └── wiring_notes.txt       # Breadboard wiring reference
```

## Setup Instructions

### 1. Program the ESP32-CAM
1. Mount ESP32-CAM on the ESP32-CAM-MB (USB programming board)
2. Open `ESP32/SmartGlove_WiFi.ino` in Arduino IDE 2.x
3. Set Board: "AI Thinker ESP32-CAM", ESP32 board package version 2.0.17
4. Change `ssid` and `password` to your phone hotspot credentials
5. Upload, open Serial Monitor at 9600 baud, press RST
6. Note the IP address printed

### 2. Program the PIC16F877A
1. Open `PIC/main_v6.c` in MPLAB X IDE
2. Compiler: XC8 v1.41, Device: PIC16F877A
3. Build and program via PICkit

### 3. Run the Python Script
1. Install Python 3.x and run: `pip install pynput`
2. Edit `Python/smartglove.py` — set `HOST` to the ESP32's IP address
3. Connect laptop to the same phone hotspot
4. Run: `python Python/smartglove.py`
5. Open a slide presentation or photo viewer and tilt your hand to navigate

## Configuration Notes

- **WiFi:** Both ESP32-CAM and laptop must be on the same network (phone hotspot recommended, public WiFi has client isolation)
- **Y-axis direction:** If forward tilt triggers the wrong direction, change `AXIS_SIGN_Y` from `1` to `-1` in `main_v6.c`
- **Thresholds:** Adjust `RIGHT_THRESHOLD`, `LEFT_THRESHOLD`, `FORWARD_THRESHOLD` in `main_v6.c` if gestures are too sensitive or not sensitive enough

## Authors

- Student 1 — PIC firmware, hardware wiring, MPU6050 integration
- Student 2 — ESP32-CAM WiFi bridge, Python laptop script

## Course

Microprocessors Project — Istanbul Medipol University
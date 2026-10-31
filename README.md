# Dynamic Radio Spectrum Allocation Using Cognitive Radio

This project explores how a wireless system can observe nearby Wi-Fi networks, compare available channels and choose a suitable channel for communication. The prototype brings together two NodeMCU ESP8266 boards, ESP-NOW communication, a DHT11 sensor, an I²C LCD and a MATLAB live dashboard.

The main idea is simple: the transmitter checks Wi-Fi channels 1, 6 and 11, evaluates the networks it can see and selects a preferred channel. When a change is needed, the transmitter and receiver coordinate the transition so that communication can continue on the selected channel.

## Project at a glance

- **Project:** Dynamic Radio Spectrum Allocation Using Cognitive Radio
- **Platform:** Two NodeMCU ESP8266 boards
- **Wireless link:** ESP-NOW
- **Sensing input:** Nearby Wi-Fi access points and their RSSI values
- **Additional sensor:** DHT11 temperature and humidity sensor
- **Display:** 16×2 I²C LCD
- **Monitoring:** MATLAB live dashboard
- **Simulation:** MATLAB Simulink channel-selection model

## How the system works

1. The transmitter scans nearby Wi-Fi networks on channels 1, 6 and 11.
2. It records the number of detected access points and their average RSSI on each channel.
3. A score is calculated for every candidate channel. The channel with the lowest score becomes the preferred choice.
4. A 20% hysteresis condition helps keep the selection steady when score differences are small.
5. If a channel change is selected, both ESP8266 boards coordinate it through ESP-NOW messages: **REQUEST, READY, COMMIT and SWITCHED**.
6. The transmitter sends temperature and channel information to the receiver.
7. The receiver updates the LCD and forwards the telemetry to MATLAB over USB serial.

## Hardware used

| Component | Role |
|---|---|
| 2 × NodeMCU ESP8266 | Transmitter and receiver |
| DHT11 | Temperature and humidity sensing |
| 16×2 I²C LCD | Displays channel and communication information |
| USB cable | Programming, power and serial connection to the computer |

## Connections

### Transmitter — DHT11

| DHT11 pin | NodeMCU pin |
|---|---|
| VCC | 3V3 |
| DATA | D5 / GPIO14 |
| GND | GND |

### Receiver — 16×2 I²C LCD

| LCD pin | NodeMCU pin |
|---|---|
| SDA | D2 / GPIO4 |
| SCL | D1 / GPIO5 |
| GND | GND |
| VCC | Supply supported by the LCD backpack |

The receiver firmware uses I²C address `0x27). Some LCD backpacks use `0x3F`. ESP8266 GPIO operates at 3.3 V, so check the LCD backpack's I²C signal voltage and use level shifting when required.

## Channel-selection method

For each candidate channel (j), the transmitter calculates a score using the detected access-point count and average RSSI:

`signalPenalty(j) = (R(j) + 100) / 100`

`S(j) = N(j) + 0.25 × signalPenalty(j)`

Here, `N(j)` is the number of detected access points and `R(j)` is their average RSSI in dBm. When a scan finds no access point on a channel, the firmware uses −100 dBm as the default RSSI and a zero signal penalty. The lowest score is preferred, with the 20% hysteresis condition guiding when the system changes from its current channel.

## MATLAB live dashboard

The receiver sends structured serial records beginning with `DATA_CSV` at 115200 baud. MATLAB reads the records and updates six live plots:

- Temperature over time
- Received packet count
- Average RSSI for channels 1, 6 and 11
- Detected access-point count
- Channel scores
- Active channel and estimated packet loss

The Simulink model represents the channel-selection workflow using input values, decision logic and Scope blocks. The ESP8266 firmware performs the live wireless communication and channel switching.

## Repository files

| File | Description |
|---|---|
| `transmitter_code.ino` | Transmitter firmware for Wi-Fi scanning, channel scoring, DHT11 readings and coordinated switching |
| `receiver_code.ino` | Receiver firmware for ESP-NOW messages, LCD display, packet tracking and serial telemetry |
| `matlab_live_dashboard.m` | MATLAB script for serial data collection and live visualisation |
| `docs/PROJECT_OVERVIEW.md` | System architecture, working process and hardware details |
| `docs/EXPERIMENTAL_RESULTS.md` | Recorded channel observations and active-channel changes |

## Getting started

1. Open the receiver sketch in Arduino IDE, select the NodeMCU ESP8266 board and upload it.
2. Open the receiver Serial Monitor and note the MAC address printed during startup.
3. Enter the receiver MAC address in `receiverMAC[]` in the transmitter sketch.
4. Upload the transmitter sketch and note its MAC address.
5. Enter the transmitter MAC address in `transmitterMAC[]` in the receiver sketch, then upload the receiver sketch again.
6. Start both boards and confirm that ESP-NOW communication is established.
7. Connect the receiver to the computer through USB.
8. In `matlab_live_dashboard.m`, set the serial port to the port assigned to the receiver. The script uses COM5 by default.
9. Close Arduino Serial Monitor before starting the MATLAB dashboard so the serial port is available.

## Libraries and software

- Arduino IDE
- ESP8266 Arduino Core
- DHT sensor library
- LiquidCrystal_I2C library
- MATLAB
- Simulink

## Project documentation

The project report covers the background of Cognitive Radio, channel scoring, hardware design, firmware flow, MATLAB monitoring, Simulink modelling and experimental observations. The documentation in this repository provides a practical guide to the implemented prototype and its workflow.

## Team

- Deepak Kumar
- Neeraj Kumar Verma
- Sreyan Bhar

**Project guide:** Dr. Amit Kumar, Assistant Professor, EECE Department, DIT University

**Course:** Principles of Communication (ECN302)  
**Department:** Electrical and Electronics & Communication Engineering, DIT University, Dehradun


# Dynamic Radio Spectrum Allocation Using Cognitive Radio

An ESP8266-based educational prototype for Wi-Fi-aware adaptive channel selection, ESP-NOW communication, DHT11 telemetry, receiver LCD monitoring and MATLAB visualisation.

## Repository structure

- `transmitter_code.ino` — NodeMCU transmitter firmware with DHT11, Wi-Fi scanning on channels 1, 6 and 11, RSSI-based heuristic scoring, hysteresis and coordinated channel-switch requests.
- `receiver_code.ino` — NodeMCU receiver firmware with ESP-NOW control/data reception, packet-gap estimation, LCD status pages and CSV serial telemetry.
- `matlab_live_dashboard.m` — MATLAB live dashboard for serial telemetry and six plots.
- `docs/PROJECT_OVERVIEW.md` — system architecture, theory, wiring, algorithm, protocol, limitations and validation guidance.

## Hardware connections

### Transmitter — DHT11
- VCC → 3V3
- DATA → D5 / GPIO14
- GND → GND

### Receiver — 16×2 I²C LCD
- SDA → D2 / GPIO4
- SCL → D1 / GPIO5
- GND → GND
- VCC → supply supported by the LCD backpack

**Electrical note:** ESP8266 GPIO uses 3.3 V logic and is not 5 V tolerant. Check the LCD backpack's I²C pull-up voltage and use a suitable level shifter if SDA/SCL are pulled up to 5 V. The supplied receiver firmware uses I²C address 0x27; some backpacks use 0x3F.

## Software requirements

- Arduino IDE with ESP8266 Arduino Core
- DHT sensor library
- LiquidCrystal_I2C library
- MATLAB

## Setup

1. Upload the receiver firmware and note its printed MAC address.
2. Enter that address in `receiverMAC[]` in the transmitter firmware.
3. Upload the transmitter firmware and note its printed MAC address.
4. Enter that address in `transmitterMAC[]` in the receiver firmware.
5. Confirm both boards start on the same channel and ESP-NOW initialises successfully.
6. Connect the receiver to the computer by USB.
7. Set the correct serial port in the MATLAB script (default COM5) and use 115200 baud.
8. Close Arduino Serial Monitor before running MATLAB.

The ESP-NOW link is direct. It does not require a Wi-Fi router or internet connection.

## Channel-selection method

The transmitter scans visible Wi-Fi access points and groups observations into channels 1, 6 and 11. It records the access-point count and average RSSI for each candidate and calculates a heuristic score. The lowest score is preferred, subject to a 20% hysteresis condition. Channel changes are coordinated using REQUEST, READY, COMMIT and SWITCHED messages.

## Scope and limitations

This is a Wi-Fi-aware adaptive channel-selection prototype demonstrating selected cognitive-radio workflow stages: observation, decision and reconfiguration. ESP8266 Wi-Fi scanning reports visible Wi-Fi access points; it does not measure total RF occupancy, noise floor, airtime utilisation, Bluetooth activity or arbitrary non-Wi-Fi emitters. It is not a wideband SDR. The channel score is a heuristic, not a calibrated spectrum-occupancy measurement.

The DHT11 provides auxiliary temperature telemetry and is not used by the channel-selection algorithm. Treat out-of-range sensor values as invalid and investigate the raw serial stream and sensor wiring.

## Validation

Compare fixed-channel operation on channels 1, 6 and 11 with adaptive operation under repeatable conditions. Record transmitted and received packet IDs, estimated packet loss, packet delivery ratio, latency, channel changes and recovery time. Retain raw logs and repeat trials before making performance claims.

## Project documentation

See [docs/PROJECT_OVERVIEW.md](docs/PROJECT_OVERVIEW.md) for the system explanation and technical notes.

## Repository URL

https://github.com/neerajkumarverma602-sudo/Dynamic-radio-spectrum-allocation-using-cognative-radio

# Project Overview

## Purpose
This project is a low-cost ESP8266 prototype that demonstrates Wi-Fi-aware adaptive channel selection. It observes visible Wi-Fi access points on channels 1, 6 and 11, calculates a heuristic channel score, applies hysteresis and coordinates channel switching between a transmitter and receiver over ESP-NOW.

## System architecture
The transmitter is a NodeMCU ESP8266 connected to a DHT11 on D5/GPIO14. The receiver is a second NodeMCU ESP8266 connected to a 16×2 I²C LCD using SDA=D2/GPIO4 and SCL=D1/GPIO5. The receiver sends serial telemetry to a computer, where MATLAB reads DATA_CSV records and displays six live plots.

## Channel observation and scoring
The transmitter scans visible Wi-Fi access points and groups them into channels 1, 6 and 11. It records access-point count and average RSSI for each candidate. The firmware calculates a heuristic score and prefers the lowest-scoring channel, subject to a 20% hysteresis margin.

This is not a direct measurement of channel occupancy. Wi-Fi scan results do not capture all emitters, airtime utilisation, noise floor or non-Wi-Fi interference.

## Channel-switch coordination
The transmitter sends REQUEST with the target channel. The receiver replies READY. The transmitter sends COMMIT and both devices schedule the channel change. After changing channels and updating peer settings, the receiver sends SWITCHED. The firmware uses a coordinated delay before normal telemetry resumes.

## MATLAB telemetry
The receiver emits DATA_CSV records containing packet ID, active channel, temperature, received and lost packet counts, and per-channel AP count, average RSSI and score. MATLAB reads the serial stream at 115200 baud and plots temperature, received packets, RSSI, AP counts, channel scores, active channel and estimated packet loss.

## Wiring
**TX DHT11:** VCC to 3V3, DATA to D5/GPIO14, GND to GND.

**RX LCD:** SDA to D2/GPIO4, SCL to D1/GPIO5, GND to GND, VCC to a supply supported by the backpack. Confirm I²C pull-up voltage; ESP8266 GPIO is not 5 V tolerant. LCD address in the supplied code is 0x27.

## Experimental caution
The supplied dashboard screenshot shows a temperature trace around 330 °C. This is outside the DHT11 operating range and must not be reported as a valid temperature measurement. Recheck the raw serial fields, sensor wiring and MATLAB parser before recording results.

## Scope
The implementation demonstrates selected cognitive-radio workflow concepts—observation, decision and reconfiguration—within a restricted Wi-Fi channel set. It is not a wideband SDR and does not perform raw IQ spectrum analysis. A more complete RF sensing stage would require a suitable SDR or dedicated RF front end.

## Future development
Potential extensions include repeated sensing, channel utilisation metrics, packet-delivery and latency measurements, more robust transaction-based switching recovery, humidity telemetry, and a Simulink model validated against hardware logs.

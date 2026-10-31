# Project Overview

## About the project

Dynamic Radio Spectrum Allocation Using Cognitive Radio is an ESP8266-based prototype that demonstrates how a wireless system can observe nearby Wi-Fi networks, compare available channels and adapt its communication channel. The implementation combines embedded hardware, direct wireless communication and MATLAB-based monitoring.

Two NodeMCU ESP8266 boards form the system. One works as the transmitter and the other as the receiver. They exchange data and channel-control messages through ESP-NOW.

## System architecture

### Transmitter

The transmitter is connected to a DHT11 sensor through D5/GPIO14. It scans visible Wi-Fi access points on channels 1, 6 and 11, records the access-point count and average RSSI for each channel, calculates channel scores and selects a preferred channel.

### Receiver

The receiver handles ESP-NOW data and channel-control messages. A 16×2 I²C LCD displays the active channel, temperature, access-point counts, average RSSI values, channel scores and packet statistics. The receiver also sends structured telemetry to a computer through USB serial.

### MATLAB and Simulink

MATLAB reads the receiver's serial data at 115200 baud and displays six live plots: temperature, received packet count, average RSSI, access-point count, channel score, and active channel with estimated packet loss.

The Simulink model represents the channel-selection workflow with input blocks, comparison logic, a MATLAB Function block and Scope blocks. The ESP8266 firmware handles the live wireless communication and channel switching.

## Channel observation and scoring

The transmitter scans the selected Wi-Fi channels and groups visible access points by channel. It uses the number of detected access points and their average RSSI to calculate a score.

`signalPenalty(j) = (R(j) + 100) / 100`

`S(j) = N(j) + 0.25 × signalPenalty(j)`

- `N(j)`: number of detected access points on channel `j`
- `R(j)`: average RSSI in dBm on channel `j`
- The channel with the lowest score is the preferred candidate.
- A 20% hysteresis condition guides channel changes and helps maintain a stable selection.

## Coordinated channel switching

The transmitter and receiver use a four-message ESP-NOW sequence:

1. **REQUEST** — the transmitter proposes the target channel.
2. **READY** — the receiver confirms that it is ready for the transition.
3. **COMMIT** — the transmitter confirms the planned channel change.
4. **SWITCHED** — the receiver confirms that the new channel is active.

Both boards follow the coordinated timing in the firmware before regular telemetry resumes.

## Telemetry and LCD information

The DHT11 is connected to the transmitter. The current firmware reads temperature approximately every two seconds and includes the latest reading in telemetry packets. The receiver rotates through five LCD pages:

1. Active channel and temperature
2. Access-point counts for channels 1, 6 and 11
3. Average RSSI values
4. Channel scores
5. Received packet count and estimated lost packets

The receiver's serial records begin with `DATA_CSV` and carry the packet ID, active channel, temperature, packet statistics and the channel observations used by MATLAB.

## Hardware connections

### DHT11 — transmitter

| DHT11 | NodeMCU |
|---|---|
| VCC | 3V3 |
| DATA | D5 / GPIO14 |
| GND | GND |

### 16×2 I²C LCD — receiver

| LCD | NodeMCU |
|---|---|
| SDA | D2 / GPIO4 |
| SCL | D1 / GPIO5 |
| GND | GND |
| VCC | Supply supported by the LCD backpack |

The LCD address in the supplied receiver firmware is `0x27). Some modules use `0x3F`. The ESP8266 uses 3.3 V GPIO logic; check the LCD backpack's I²C pull-up voltage and use a suitable level shifter when required.

## Firmware timing

| Operation | Interval |
|---|---:|
| Wi-Fi spectrum rescan | 30,000 ms |
| Telemetry transmission | 1,000 ms |
| DHT11 temperature read | 2,000 ms |
| REQUEST retry | 1,000 ms |
| Coordinated switch delay | 6,000 ms |
| LCD page interval | 3,500 ms |
| LCD refresh check | 100 ms |

## Project scope

The prototype focuses on the cognitive-radio workflow of spectrum observation, channel assessment, decision-making and communication reconfiguration. Its channel assessment uses visible Wi-Fi access points on channels 1, 6 and 11. The DHT11 provides additional temperature telemetry, while the LCD and MATLAB dashboard make the system's operation easy to follow.

## Experimental observations

The project report records channel observations across three time intervals and shows the active-channel trace moving from channel 11 to channel 1 and then returning to channel 11. The recorded values are available in [Experimental Results](EXPERIMENTAL_RESULTS.md).

## Further development

The project provides a starting point for exploring machine-learning-based channel selection, cooperative spectrum sensing, additional telemetry, richer channel measurements and software-defined radio platforms.

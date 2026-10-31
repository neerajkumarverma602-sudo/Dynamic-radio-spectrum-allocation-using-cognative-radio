# Experimental Results

The following observations are transcribed from the project report's experimental results section. Channel scores are shown as approximate values where the report presents them with a tilde.

## Observed channel conditions

| Observation interval (s) | Channel 1 (AP / RSSI / score) | Channel 6 (AP / RSSI / score) | Channel 11 (AP / RSSI / score) |
|---|---|---|---|
| 15–40 | 7 / −82 dBm / ~7 | 3 / −64 dBm / ~3 | 1 / −65 dBm / ~1 |
| 52–80 | 3 / −83 dBm / ~3 | 5 / −71 dBm / ~5 | 5 / −78 dBm / ~5 |
| 90–104 | 4 / −78 dBm / ~4 | Not clearly readable in the dashboard | 1 / −61 dBm / ~1 |

## Active-channel changes

| Approximate time interval (s) | Active channel | Observation |
|---|---:|---|
| 13–51 | 11 | Initial active channel in the displayed trace |
| 52–89 | 1 | Active channel changes from 11 to 1 |
| 90–104 | 11 | Active channel returns to 11 |

## What the results show

The observations show the transmitter comparing channels 1, 6 and 11 using access-point counts and average RSSI. The active-channel trace records a change from channel 11 to channel 1, followed by a return to channel 11. The LCD and MATLAB dashboard provide the channel information and packet statistics during operation.

The channel scores are the values reported in the project report and are presented as approximate observations. The channel 6 entry for the 90–104 second interval is retained as unreadable in the source results table rather than estimated.

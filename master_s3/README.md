# Tire Temperature Dashboard - ESP32-S3

Central dashboard displaying real-time thermal data from 4 tire temperature satellites on a TFT display.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-green.svg)

## Quick Start

### Hardware Required
- ESP32-S3 (44-pin)
- ILI9341 2.8" TFT Display (320×240)
- CAN transceiver (MCP2551 or SN65HVD230)
- 120Ω termination resistors

### Wiring
```
ILI9341 Display    ESP32-S3
    MOSI   ────────  GPIO 11
    MISO   ────────  GPIO 13
    SCK    ────────  GPIO 12
    CS     ────────  GPIO 10
    DC     ────────  GPIO 8
    RST    ────────  GPIO 9
    BL     ────────  GPIO 14

CAN Transceiver    ESP32-S3
    TX     ────────  GPIO 17
    RX     ────────  GPIO 18
```

### TFT_eSPI Configuration

**IMPORTANT**: Edit `libraries/TFT_eSPI/User_Setup.h`:

```cpp
#define ILI9341_DRIVER
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC   8
#define TFT_RST  9
#define TFT_BL   14
#define SPI_FREQUENCY  10000000
#define USE_HSPI_PORT  // CRITICAL for ESP32-S3!
```

### Upload
1. Install TFT_eSPI library
2. Configure User_Setup.h (see above)
3. Select "ESP32S3 Dev Module" in Arduino IDE
4. Upload code

## Display Layout

The screen shows 4 thermal heatmaps (one per wheel):

```
┌──────────────────────────────────┐
│  FL           │  FR              │
│  ▓▓▓▓▓▓       │  NO SIGNAL       │
│  22°-25°      │  0°-0°           │
├──────────────────────────────────┤
│  RL           │  RR              │
│  NO SIGNAL    │  NO SIGNAL       │
│  0°-0°        │  0°-0°           │
├──────────────────────────────────┤
│ 0° ═══════════════════════ 100°  │
└──────────────────────────────────┘
```

Colors: Blue (cold) → Purple → Red → Yellow → White (hot)

## Serial Output

```
=== TIRE TEMP DASHBOARD ===

OK: Display initialized
OK: CAN initialized

Waiting for data...

Uptime: 127s | Packets: 1023
FL: OK (age: 0s)
FR: NO SIGNAL (age: 127s)
RL: NO SIGNAL (age: 127s)
RR: NO SIGNAL (age: 127s)
```

## Performance

- **Display refresh**: 10 FPS
- **CAN processing**: Up to 100 packets per loop
- **Latency**: <100ms from CAN to display
- **Memory**: ~8KB RAM for buffers

## Troubleshooting

**Display blank or garbage?**
- Verify `SPI.begin()` is called before `tft.init()`
- Check `USE_HSPI_PORT` is defined in User_Setup.h
- Confirm pin configuration matches hardware

**"NO SIGNAL" for all wheels?**
- Check CAN wiring (TX/RX not swapped?)
- Verify termination resistors (120Ω each end)
- Confirm satellites are powered and transmitting
- Check 125 kbps baud rate matches

**Partial image updates only?**
- This was a CAN buffer overflow issue
- Solved by increasing RX buffer to 50 packets
- Should not occur with current code

## License

MIT License - See LICENSE file for details

## See Also

- [Satellite Code](../satellite_wroom_stable/README.md)
- [Complete Documentation](CLAUDE.md)

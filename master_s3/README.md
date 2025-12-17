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
- Mode button (optional - built-in BOOT button on GPIO 1 works)

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

Mode Button        ESP32-S3
    (optional)     GPIO 1  ──── GND (when pressed)
                             (Built-in BOOT button works)
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
│  FL           │  FR      [MODE]  │
│  ▓▓▓▓▓▓       │  NO SIGNAL       │
│  55°          │  --              │
├──────────────────────────────────┤
│  RL           │  RR              │
│  NO SIGNAL    │  NO SIGNAL       │
│  --           │  --              │
├──────────────────────────────────┤
│ 25° ═══════════════════════ 70°  │
└──────────────────────────────────┘
```

**Heatmap Colors:** Blue (cold) → Purple → Red → Yellow → White (hot)

**Temperature Display Colors:**
- **Cyan**: <40°C (Cold)
- **Green background**: 40-60°C (Ideal operating range)
- **Yellow**: 60-75°C (Warm)
- **Blinking Red**: >75°C (Critical overheating)

## Display Modes

Press the button on **GPIO 1** (BOOT button) to cycle through 3 display modes:

### 1. FIXED Mode (Blue indicator)
- Fixed temperature range: **25-70°C**
- Best for consistent comparison between wheels
- Temperatures outside range shown as min/max colors
- Actual temperatures always shown in numbers

### 2. DYNAMIC/WHEEL Mode (Red indicator)
- Auto-adjusting range **per wheel**
- Each wheel shows its own min-max range
- Maximum detail for each individual wheel
- Best for analyzing single wheel behavior

### 3. DYNAMIC/GLOBAL Mode (Green indicator)
- Auto-adjusting range **across all wheels**
- All wheels share same temperature scale
- Best for comparing wheel-to-wheel differences
- Updates as global min/max changes

**Mode indicator** shown as colored box (FIX/DYN/GLO) between upper wheels.

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
- **Temperature tracking**: 30-second rolling maximum per wheel

## Temperature Monitoring

Each wheel displays the **maximum temperature** recorded in the last 30 seconds. This helps catch brief temperature spikes that might otherwise be missed during quick glances at the display.

**Color-coded warnings:**
- Temperatures <40°C show in **cyan** (tire needs heat)
- Ideal range 40-60°C shows with **green background** (optimal)
- Warm range 60-75°C shows in **yellow** (monitor closely)
- Critical >75°C **blinks red** background (overheating risk)

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

**Mode button not working?**
- Check GPIO 1 (BOOT button) is accessible
- Button requires ~300ms debounce - don't press too quickly
- Current mode shown as colored box (FIX/DYN/GLO) on display
- External button can be connected to GPIO 1 (connect to GND when pressed)

## License

MIT License - See LICENSE file for details

## See Also

- [Satellite Code](../satellite_wroom_stable/README.md)
- [Complete Documentation](CLAUDE.md)

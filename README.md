# Racing Tire Temperature Monitoring System

A distributed real-time thermal imaging system for race car tire temperature monitoring, using ESP32 microcontrollers, MLX90640 thermal cameras, and CAN bus communication.

![System Overview](https://img.shields.io/badge/Status-Production%20Ready-brightgreen)
![License](https://img.shields.io/badge/license-MIT-blue.svg)

## System Overview

This system consists of **4 wheel satellites** (ESP32-WROOM with thermal cameras) connected via **CAN bus** to a **central master dashboard** (ESP32-S3 with TFT display).

Each satellite reads thermal data from its wheel at 2 Hz and transmits compressed thermal images over the CAN bus. The master receives data from all 4 wheels and displays real-time heatmaps on a 320×240 TFT screen.

## Features

✅ **Real-time thermal imaging** (2 Hz per wheel)
✅ **4 independent wheel sensors** (FL, FR, RL, RR)
✅ **Central dashboard** with live heatmaps
✅ **3 display modes** (Fixed range, Dynamic per wheel, Global dynamic)
✅ **Temperature warnings** (Color-coded: Cold, Ideal, Warm, Critical)
✅ **Budget-friendly** (~$95 total vs $400+ commercial)
✅ **Robust CAN bus** communication
✅ **Minimal latency** (<100ms sensor to display)

## Project Structure

```
racing-tire-thermal-monitor/
├── satellite_wroom_stable/    # Wheel satellite code
│   ├── satellite_wroom_stable.ino
│   ├── README.md
│   └── CLAUDE.md
│
├── master_s3/                  # Central dashboard code
│   ├── master_s3.ino
│   ├── README.md
│   └── CLAUDE.md
│
└── README.md                   # This file
```

## Hardware Requirements

### Per Wheel Satellite (×4)
- ESP32-WROOM-32 (30-pin) - ~$3
- GY-MCU90640 thermal camera - ~$15
- CAN transceiver (MCP2551) - ~$1
- Misc (wires, connectors) - ~$1
- **Total per wheel: ~$20**

### Central Dashboard (×1)
- ESP32-S3 (44-pin) - ~$5
- ILI9341 2.8" TFT display - ~$8
- CAN transceiver (MCP2551) - ~$1
- Misc (wires, connectors) - ~$1
- **Total dashboard: ~$15**

### Complete System Cost
**4 wheels + dashboard = ~$95** (vs $400+ for commercial systems)

## Critical Hardware Note

⚠️ **GY-MCU90640 Modification Required!**

The GY-MCU90640 modules contain a STM32 microcontroller that interferes with standard MLX90640 libraries. You **MUST** solder together the "SET I2C" pads on the back of each module to bypass the STM32 and enable direct I2C access.

**Without this modification, the sensors will not work!**

## Quick Start

### 1. Build One Satellite First

1. Solder "SET I2C" pads on GY-MCU90640
2. Wire ESP32-WROOM as shown in [satellite_wroom_stable/README.md](satellite_wroom_stable/README.md)
3. Upload satellite code
4. Verify thermal data in Serial Monitor

### 2. Build Master Dashboard

1. Configure TFT_eSPI library (see [master_s3/README.md](master_s3/README.md))
2. Wire ESP32-S3 to display and CAN
3. Upload master code
4. Verify display shows "NO SIGNAL" for all wheels

### 3. Connect via CAN Bus

1. Wire CAN bus between satellite and master
2. Add 120Ω termination resistors at each end
3. Power on both units
4. Watch thermal data appear on display!

### 4. Build Remaining Satellites

1. Clone satellite for other wheels (FR, RL, RR)
2. Change `WHEEL_ID` for each (0x11, 0x12, 0x13)
3. Connect all to CAN bus
4. Enjoy full 4-wheel monitoring!

## System Specifications

| Parameter | Value |
|-----------|-------|
| Thermal resolution | 32×24 pixels (compressed to 32×6) |
| Update rate | 2 Hz per wheel |
| Display refresh | 10 FPS |
| CAN bus speed | 125 kbps |
| Temperature range | 0-100°C display range |
| Sensor accuracy | ±1.5°C (MLX90640 spec) |
| Latency | <100ms sensor to display |

## Technical Highlights

### Data Compression
Each wheel sends 32×24 pixel thermal images compressed to 32×6 via vertical averaging, reducing CAN bandwidth by 75% while preserving horizontal detail for contact patch analysis.

### CAN Bus Optimization
- **50-packet RX buffer** prevents overflow during burst transmission
- **1ms inter-packet delay** ensures reliable delivery
- **29 packets per frame** (1 marker + 28 data chunks)

### Display Rendering
- **Lazy updates**: Only redraws when new data arrives
- **Iron colormap**: Intuitive blue→red→white temperature gradient
- **Timeout handling**: Automatic "NO SIGNAL" display

### Display Modes
The dashboard supports 3 display modes (cycle with button on GPIO 1):
- **FIXED (25-70°C)**: Fixed temperature range for consistent comparison
- **DYNAMIC/WHEEL**: Auto-adjusting range per wheel for maximum detail
- **DYNAMIC/GLOBAL**: Auto-adjusting range globally across all active wheels

### Temperature Warnings
Intelligent color-coded temperature display:
- **<40°C (Cyan)**: Cold tire - needs heat
- **40-60°C (Green background)**: Ideal operating range
- **60-75°C (Yellow)**: Warm - monitor closely
- **>75°C (Blinking Red)**: Critical - overheating risk

## Documentation

- **[Satellite README](satellite_wroom_stable/README.md)** - Quick start guide for wheel sensors
- **[Satellite CLAUDE.md](satellite_wroom_stable/CLAUDE.md)** - Complete technical documentation
- **[Master README](master_s3/README.md)** - Quick start guide for dashboard
- **[Master CLAUDE.md](master_s3/CLAUDE.md)** - Complete technical documentation

## Troubleshooting

### Satellite Issues
See [satellite_wroom_stable/README.md](satellite_wroom_stable/README.md#troubleshooting)

### Master Issues
See [master_s3/README.md](master_s3/README.md#troubleshooting)

### CAN Bus Issues
- **No communication**: Check TX/RX not swapped, verify 120Ω terminators
- **Intermittent drops**: Increase satellite delay, check wire quality
- **Buffer overflow**: Already solved with 50-packet RX buffer

## Future Enhancements

- [ ] Data logging to SD card
- [ ] WiFi telemetry for pit crew
- [ ] Historical temperature trending
- [ ] Configurable alarms for overheating
- [ ] Multiple colormap options

## License

MIT License - See [LICENSE](LICENSE) file for details

## Acknowledgments

- Adafruit for the MLX90640 library
- Bodmer for the excellent TFT_eSPI library
- ESP32 community for CAN bus examples

## Contributing

Pull requests welcome! For major changes, please open an issue first.

## Support

For questions or issues:
1. Check documentation in respective folders
2. Review troubleshooting sections
3. Open an issue on GitHub

---

**Built with ❤️ for the racing community**

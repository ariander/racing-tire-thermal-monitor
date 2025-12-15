# Tire Temperature Satellite - ESP32-WROOM-32

Real-time tire temperature monitoring satellite node using GY-MCU90640 thermal camera and CAN bus communication.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32-green.svg)

## Quick Start

### Hardware Required
- ESP32-WROOM-32 (30-pin)
- GY-MCU90640 thermal camera (**"SET I2C" must be soldered!**)
- CAN transceiver (MCP2551 or similar)
- 120Ω termination resistors

### Wiring
```
GY-MCU90640        ESP32-WROOM
    VIN    ────────  3.3V
    GND    ────────  GND
    SDA    ────────  GPIO 21
    SCL    ────────  GPIO 22

CAN Transceiver    ESP32-WROOM
    TX     ────────  GPIO 5
    RX     ────────  GPIO 4
```

### Configuration

Edit these lines for each wheel:

```cpp
#define WHEEL_ID 0x10        // Front Left: 0x10, Front Right: 0x11, etc.
const char* WHEEL_NAME = "FL";
```

### Upload
1. Install Adafruit_MLX90640 library
2. Select "ESP32 Dev Module" in Arduino IDE
3. Upload code
4. Open Serial Monitor (115200 baud)

## Critical: GY-MCU90640 Setup

**The GY-MCU90640 will NOT work without this modification!**

Solder together the two pads marked "SET I2C" on the back of the module. This bypasses the STM32 microcontroller and gives direct I2C access to the MLX90640 sensor.

## Output

Serial Monitor shows:
```
=== TIRE TEMP: FL ===

OK: MLX90640 initialized
OK: CAN initialized

Streaming data...

Frames: 10 | Temp: 22.3 - 25.8 C
```

## Performance

- **Frame rate**: 2 Hz
- **Resolution**: 32×6 pixels (compressed from 32×24)
- **CAN packets**: 29 per frame
- **Latency**: <50ms

## Troubleshooting

**Sensor not detected?**
- Check "SET I2C" solder bridge
- Verify 3.3V power
- Check I2C pullup resistors (4.7kΩ)

**CAN not working?**
- Check termination resistors (120Ω each end)
- Verify TX/RX pins (GPIO 5/4)
- Confirm 125 kbps baud rate

## License

MIT License - See LICENSE file for details

## See Also

- [Master Dashboard](../master_s3/README.md)
- [Complete Documentation](CLAUDE.md)

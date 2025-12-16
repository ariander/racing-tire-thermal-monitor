# Tire Temperature Satellite - ESP32-C3 Super Mini

Alternative satellite implementation using ESP32-C3 Super Mini instead of ESP32-WROOM-32.

![Platform](https://img.shields.io/badge/platform-ESP32--C3-green.svg)

## ⚠️ Critical Requirements

**GY-MCU90640 Modification Required!**

You **MUST** solder together the "SET I2C" pads on the back of each GY-MCU90640 module to bypass the STM32 microcontroller and enable direct I2C access to the MLX90640 sensor.

**Without this modification, the sensor will not work!**

## Hardware

### Components
- ESP32-C3 Super Mini (~$2)
- GY-MCU90640 thermal camera (~$15)
- MCP2551 CAN transceiver (~$1)
- 120Ω termination resistors (only at CAN bus ends)

### Pin Mapping (ESP32-C3 Super Mini)

```
GY-MCU90640        ESP32-C3 Super Mini
    VIN   ────────  3.3V
    GND   ────────  GND
    SDA   ────────  GPIO 8  (standard I2C)
    SCL   ────────  GPIO 9  (standard I2C)

CAN Transceiver    ESP32-C3 Super Mini
    TX    ────────  GPIO 2
    RX    ────────  GPIO 3
    GND   ────────  GND
    VCC   ────────  3.3V or 5V (check your module)
```

### Comparison: C3 Super Mini vs WROOM-32

| Feature | ESP32-C3 Super Mini | ESP32-WROOM-32 |
|---------|-------------------|----------------|
| **Architecture** | RISC-V single core | Xtensa dual core |
| **RAM** | 400KB | 520KB |
| **GPIO pins** | 13 usable | 25+ usable |
| **Size** | Very compact | Larger |
| **Cost** | ~$2 | ~$3 |
| **I2C pins** | GPIO 8, 9 | GPIO 21, 22 |
| **CAN support** | Yes (TWAI) | Yes (TWAI) |
| **For satellite** | ✅ Perfect | ✅ Perfect |
| **For master** | ⚠️ Tight GPIO | ✅ Better choice |

## Configuration for Multiple Wheels

Edit these lines for each satellite:

```cpp
#define WHEEL_ID 0x10     // Change this!
const char* WHEEL_NAME = "FL";
```

### Wheel IDs:
- **Front Left (FL)**: `WHEEL_ID = 0x10`
- **Front Right (FR)**: `WHEEL_ID = 0x11`
- **Rear Left (RL)**: `WHEEL_ID = 0x12`
- **Rear Right (RR)**: `WHEEL_ID = 0x13`

## Arduino IDE Setup

**Board Settings:**
```
Board: "ESP32C3 Dev Module"
Upload Speed: 921600
CPU Frequency: 160MHz
Flash Size: 4MB (32Mb)
Partition Scheme: Default 4MB with spiffs
USB CDC On Boot: Enabled  (for Serial output)
```

**Required Libraries:**
- Wire (built-in)
- Adafruit MLX90640 (install via Library Manager)
- driver/twai.h (ESP32 CAN, built-in)

## Upload Process

1. Connect ESP32-C3 via USB-C
2. Select correct COM port
3. Press and hold BOOT button
4. Click Upload in Arduino IDE
5. Release BOOT button when "Connecting..." appears

## Expected Serial Output

```
=== TIRE TEMP: FL (ESP32-C3) ===

OK: MLX90640 initialized
OK: CAN initialized

Streaming data...

Frames: 10 | Temp: 22.3 - 26.7 C
Frames: 20 | Temp: 22.1 - 27.2 C
```

## Performance

Same as WROOM-32 version:
- **Refresh rate**: 2 Hz per wheel
- **Data compression**: 32×24 → 32×6 pixels
- **CAN transmission**: 29 packets per frame (1 marker + 28 data)
- **Inter-packet delay**: 1ms (prevents master buffer overflow)

## Differences from WROOM-32 Version

### Hardware Differences:
1. **Different GPIO pins** for I2C and CAN
2. **Single core** (vs dual core) - not a problem for this application
3. **Smaller physical size** - better for mounting on wheels
4. **Lower cost** - saves $1 per satellite ($4 total for 4 wheels)

### Code Differences:
- **Only pin definitions changed!**
- All logic, timing, and CAN protocol identical
- Works with same master dashboard (ESP32-S3)

## CAN Bus Daisy Chain Wiring

See main project README for complete daisy chain wiring diagram showing how to connect 4 satellites + 1 master.

## Troubleshooting

### Sensor Not Detected

**Error:**
```
ERROR: MLX90640 init failed!
Check: SET I2C loddet? Power? Wiring?
```

**Solutions:**
- ✅ Verify "SET I2C" pads are soldered together
- ✅ Check 3.3V power to GY-MCU90640
- ✅ Verify SDA=GPIO8, SCL=GPIO9
- ✅ Check I2C pullup resistors (4.7kΩ, usually on sensor board)

### CAN Not Working

**Symptoms:**
- Master shows "NO SIGNAL" for this wheel
- No data received

**Solutions:**
- ✅ Check CAN TX/RX not swapped (TX→GPIO2, RX→GPIO3)
- ✅ Verify 120Ω termination resistors at **CAN bus ends only**
- ✅ Confirm CAN bus wiring (see daisy chain diagram)
- ✅ Check all satellites have unique WHEEL_ID

### Upload Issues

**Error:** `Failed to connect to ESP32-C3`

**Solutions:**
- ✅ Hold BOOT button during upload
- ✅ Enable "USB CDC On Boot" in Arduino IDE
- ✅ Try different USB cable (data cable, not charge-only)
- ✅ Check correct COM port selected

## Why C3 Super Mini Works for Satellites

The C3 Super Mini is actually **ideal** for wheel satellites because:

1. **Sufficient GPIO**: Only needs 4 pins (SDA, SCL, CAN_TX, CAN_RX)
2. **Enough RAM**: 400KB is plenty for thermal processing
3. **TWAI support**: Same CAN controller as WROOM
4. **Compact size**: Easier to mount on wheels
5. **Lower cost**: $2 vs $3 saves money

The **only** limitation is for the **master dashboard** where you need more GPIOs for the TFT display (6 pins) + CAN (2 pins) = 8 pins minimum. For satellites, C3 is perfect!

## License

MIT License - See main project LICENSE file

## See Also

- [Master Dashboard Code](../master_s3/README.md)
- [WROOM-32 Satellite](../satellite_wroom_stable/README.md)
- [CAN Bus Wiring Guide](../README.md#can-bus-daisy-chain)

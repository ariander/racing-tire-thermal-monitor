# Tire Temperature Sensor - Satellite (ESP32-WROOM-32)

## Oversikt

Satellitt-node for tire temperature monitoring system. Leser thermal data fra GY-MCU90640 sensor og sender over CAN bus til master dashboard.

## Hardware

### Microcontroller
- **Board**: ESP32-WROOM-32 (30-pin)
- **Strøm**: 3.3V via USB eller ekstern forsyning

### Sensor
- **Type**: GY-MCU90640 (32x24 thermal camera)
- **KRITISK**: "SET I2C" må loddes sammen på baksiden!
  - Uten denne modet fungerer modulen ikke med Adafruit-biblioteket
  - SET I2C bypasser STM32-mikrokontrolleren på modulen
  - Gir direkte I2C-tilgang til MLX90640-sensoren

### CAN Bus
- **Baud rate**: 125 kbps
- **Transceiver**: Standard CAN transceiver (MCP2551 eller lignende)

## Pinout

```
GY-MCU90640:
  VIN  → 3.3V
  GND  → GND
  SDA  → GPIO 21
  SCL  → GPIO 22

CAN Transceiver:
  TX   → GPIO 5
  RX   → GPIO 4
```

## Konfigurasjon

Endre disse konstantene for hvert hjul:

```cpp
#define WHEEL_ID 0x10        // CAN ID
const char* WHEEL_NAME = "FL";  // Visningsnavn
```

### Anbefalte ID-er:
- **Front Left (FL)**: 0x10
- **Front Right (FR)**: 0x11
- **Rear Left (RL)**: 0x12
- **Rear Right (RR)**: 0x13

## Funksjonalitet

### Sensorlesing
- **Refresh rate**: 2Hz (MLX90640_2_HZ)
- **Mode**: INTERLEAVED (bedre temporal resolution)
- **Resolution**: 18-bit ADC
- **I2C speed**: 100 kHz

### Data Compression
- **Input**: 32×24 pixels (768 floats = ~3KB)
- **Output**: 32×6 pixels (192 bytes)
- **Metode**: Vertikal averaging (4 rader → 1 rad)
- **Encoding**: Float temp → uint8 (temp × 2.55)

### CAN Transmission
1. Sender frame start marker: `[0xFF, frame_id]`
2. Sender 28 data-pakker:
   - Format: `[offset, byte1...byte7]`
   - 1ms delay mellom pakker
3. Total transmission tid: ~30ms per frame

### Status Output
Printer status hvert 5. sekund:
```
Frames: 42 | Temp: 22.3 - 25.8 C
```

## Feilsøking

### "MLX90640 init failed"
**Sjekkliste:**
1. ✅ Er "SET I2C" loddet sammen?
2. ✅ 3.3V strøm til sensor?
3. ✅ SDA/SCL riktig koblet (GPIO 21/22)?
4. ✅ I2C pullup resistors (4.7kΩ)?

### Temperaturer virker feil
- Første ~30 frames kan være unøyaktige (sensor kalibrerer)
- Vent 15 sekunder etter oppstart
- Max temp bør være <100°C i romtemperatur

### CAN-kommunikasjon fungerer ikke
1. Sjekk termineringsresistorer (120Ω i hver ende)
2. Verifiser TX/RX pins (GPIO 5/4)
3. Sjekk baud rate matcher master (125 kbps)

## Dependencies

```
Adafruit_MLX90640  (via Library Manager)
Wire.h             (built-in)
driver/twai.h      (ESP32 CAN driver)
```

## Arduino IDE Settings

```
Board: "ESP32 Dev Module"
Upload Speed: 921600
CPU Frequency: 240MHz
Flash Size: 4MB
Partition Scheme: Default 4MB with spiffs
```

## Ytelse

- **Frame rate**: 2 Hz (500ms per frame)
- **CAN packets**: 29 per frame (1 marker + 28 data)
- **Latency**: <50ms fra sensor til CAN
- **Memory**: ~5KB RAM (hovedsakelig for 768-float array)

## Viktige designvalg

1. **2Hz refresh**: Balanse mellom oppdateringsfrekvens og CAN-belastning
2. **INTERLEAVED mode**: Bedre temporal resolution enn CHESS
3. **1ms CAN delay**: Forhindrer buffer overflow på mottaker
4. **Frame marker**: Identifiserer start på ny frame for synkronisering
5. **Direkte blokkering**: `getFrame()` blokkerer til data klar (ingen egen delay nødvendig)

## Fremtidige forbedringer

- [ ] Adaptiv CAN delay basert på bus load
- [ ] Temperatur-kalibrering per hjul
- [ ] Watchdog timer for auto-recovery
- [ ] Error reporting via dedikerte CAN-meldinger

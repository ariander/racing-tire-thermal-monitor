# Tire Temperature Dashboard - Master (ESP32-S3)

## Oversikt

Master dashboard for tire temperature monitoring system. Mottar thermal data fra 4 satellitter over CAN bus og viser real-time heatmaps på TFT display.

## Hardware

### Microcontroller
- **Board**: ESP32-S3 (44-pin)
- **RAM**: 512KB (ingen PSRAM nødvendig)
- **Strøm**: USB-C eller ekstern 5V

### Display
- **Type**: ILI9341 2.8" TFT (320×240 pixels)
- **Interface**: SPI
- **Driver**: TFT_eSPI library

### CAN Bus
- **Baud rate**: 125 kbps
- **RX buffer**: 50 pakker (økt fra default 5)
- **Transceiver**: Standard CAN transceiver

## Pinout

```
ILI9341 Display (SPI):
  MOSI → GPIO 11
  MISO → GPIO 13
  SCK  → GPIO 12
  CS   → GPIO 10
  DC   → GPIO 8
  RST  → GPIO 9
  BL   → GPIO 14

CAN Transceiver:
  TX   → GPIO 17
  RX   → GPIO 18
```

## Display Layout

```
┌─────────────────────────────────────┐
│  FL (160×80)     │  FR (160×80)     │
│  Heatmap         │  Heatmap         │
│  22°-25°         │  0°-0° (NO SIG)  │
├─────────────────────────────────────┤
│  RL (160×80)     │  RR (160×80)     │
│  Heatmap         │  Heatmap         │
│  0°-0° (NO SIG)  │  0°-0° (NO SIG)  │
├─────────────────────────────────────┤
│ 0° ════════════════════════════ 100°│
└─────────────────────────────────────┘
```

## Funksjonalitet

### CAN Mottak
- **Buffer size**: 50 pakker (kritisk for å unngå overflow!)
- **Processing**: Opptil 100 pakker per loop
- **Wheel mapping**:
  - 0x10 → FL (Front Left)
  - 0x11 → FR (Front Right)
  - 0x12 → RL (Rear Left)
  - 0x13 → RR (Rear Right)

### Data Reassembly
- Mottar 29 CAN-pakker per frame (1 marker + 28 data)
- Skriver direkte til display buffer (192 bytes per hjul)
- Ingen kompleks frame-synkronisering (enklere, mer robust)

### Display Rendering
- **Update rate**: 10 FPS (hver 100ms)
- **Rendering**: Full redraw kun når nye data mottas
- **Pixel scaling**: 32×6 → 160×80 (5×13 pixel blocks)
- **Color map**: Iron colormap (blå→lilla→rød→gul→hvit)

### Timeout Handling
- Viser "NO SIGNAL" hvis ingen data på 2 sekunder
- Automatisk gjenoppretting når data kommer tilbake

### Status Monitoring
Printer status hvert 5. sekund:
```
Uptime: 127s | Packets: 1023
FL: OK (age: 0s)
FR: NO SIGNAL (age: 127s)
RL: NO SIGNAL (age: 127s)
RR: NO SIGNAL (age: 127s)
```

## Feilsøking

### Display viser ingenting
**Sjekkliste:**
1. ✅ `SPI.begin()` kalles **før** `tft.init()`?
2. ✅ User_Setup.h konfigurert riktig?
3. ✅ USE_HSPI_PORT definert?
4. ✅ Pins matcher hardware?

### "NO SIGNAL" for alle hjul
1. Sjekk CAN-kabler (TX/RX byttet om?)
2. Verifiser termineringsresistorer (120Ω)
3. Sjekk at satellitter sender (se Serial Monitor)
4. Verifiser baud rate (125 kbps på begge sider)

### Kun første del av bildet oppdaterer seg
- **Løst**: Økt CAN RX buffer til 50 pakker
- Dette var buffer overflow - for mange pakker kom for raskt

### Flimring eller "ghost images"
- **Løst**: Full redraw kun når nye data mottas
- Tegner ikke unødvendig ofte

## TFT_eSPI Configuration

`User_Setup.h` må inneholde:

```cpp
#define ILI9341_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Pins
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC   8
#define TFT_RST  9
#define TFT_BL   14

// Speed
#define SPI_FREQUENCY  10000000

// KRITISK for ESP32-S3!
#define USE_HSPI_PORT
```

## Dependencies

```
TFT_eSPI           (via Library Manager)
SPI.h              (built-in)
driver/twai.h      (ESP32 CAN driver)
```

## Arduino IDE Settings

```
Board: "ESP32S3 Dev Module"
Upload Speed: 921600
USB CDC On Boot: "Enabled"
CPU Frequency: 240MHz
Flash Size: 16MB (eller din variant)
Partition Scheme: Default
PSRAM: "Disabled" (ikke nødvendig)
```

## Ytelse

- **Display FPS**: 10 (oppdaterer kun ved nye data)
- **CAN processing**: Opptil 100 pakker per loop
- **Latency**: <100ms fra CAN mottak til visning
- **Memory**: ~8KB RAM for thermal buffers

## Viktige designvalg

1. **50-pakke RX buffer**: Forhindrer packet drops ved burst transmission
2. **Direkte buffer update**: Enklere enn frame-synkronisering, mer robust
3. **Lazy rendering**: Tegner kun når nye data mottas
4. **Full redraw**: Enklere enn dirty tracking, fortsatt smooth ved 2Hz
5. **10 FPS update**: Balanse mellom responsivitet og CPU-bruk

## Fremtidige forbedringer

- [ ] "Hottest wheel" highlight (gul temperatur)
- [ ] FPS-visning (toggle med BOOT-knapp)
- [ ] Historikk/trending for hver sensor
- [ ] Konfigurerbar timeout (via UI)
- [ ] WiFi data logging

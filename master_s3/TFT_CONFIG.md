# TFT_eSPI Konfigurasjon for ESP32-S3 + 2.8" Display

## Steg 1: Finn display-driver

Din 2.8" 240x320 display bruker sannsynligvis en av disse:
- **ILI9341** (mest vanlig for 2.8" SPI)
- **ST7789** (nyere versjon)
- **ILI9340** (eldre)

**Hvordan finne ut:**
- Sjekk på baksiden av displayet for chip-merking
- Sjekk eBay/AliExpress-listing
- Hvis ukjent, test ILI9341 først

---

## Steg 2: Finn TFT_eSPI User_Setup.h

På Mac/Linux:
```bash
cd ~/Documents/Arduino/libraries/TFT_eSPI
open User_Setup.h
```

På Windows:
```
C:\Users\[username]\Documents\Arduino\libraries\TFT_eSPI\User_Setup.h
```

---

## Steg 3: Rediger User_Setup.h

**VIKTIG:** Ta backup av original fil først!

### Variant A: ILI9341 Display (mest vanlig)

```cpp
// ═══════════════════════════════════════════════════════════
// USER_SETUP.H FOR ESP32-S3 + ILI9341 2.8" DISPLAY
// ═══════════════════════════════════════════════════════════

// Kommenter ut alt annet, bruk KUN disse innstillingene:

#define ILI9341_DRIVER       // 2.8" display driver

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// ESP32-S3 pins for SPI
#define TFT_MOSI 11   // SPI MOSI
#define TFT_SCLK 12   // SPI Clock
#define TFT_CS   10   // Chip select
#define TFT_DC   8    // Data/Command
#define TFT_RST  9    // Reset

// Valgfri backlight-kontroll
// #define TFT_BL   46   // LED backlight (kommenter inn hvis du har dette)

// Fonts
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH
#define LOAD_GFXFF  // FreeFonts

#define SMOOTH_FONT

// SPI frekvens
#define SPI_FREQUENCY  27000000   // 27MHz (kan testes opp til 80MHz)
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
```

### Variant B: ST7789 Display

Hvis ILI9341 ikke fungerer, prøv dette:

```cpp
#define ST7789_DRIVER      // ST7789 driver

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// ESP32-S3 pins (samme som over)
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC   8
#define TFT_RST  9

// ST7789 spesifikke innstillinger
#define CGRAM_OFFSET

// Resten samme som ILI9341
```

---

## Steg 4: Test display

Last opp denne test-koden til ESP32-S3:

```cpp
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("Testing TFT...");

    tft.init();
    tft.setRotation(0);  // Portrait

    // Test farger
    tft.fillScreen(TFT_RED);
    delay(1000);
    tft.fillScreen(TFT_GREEN);
    delay(1000);
    tft.fillScreen(TFT_BLUE);
    delay(1000);
    tft.fillScreen(TFT_BLACK);

    // Test tekst
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(50, 100);
    tft.println("ESP32-S3");
    tft.setCursor(50, 130);
    tft.println("TFT Test");

    Serial.println("Display OK!");
}

void loop() {
    // Tom
}
```

**Forventet resultat:**
- Rød, grønn, blå skjerm i 1 sekund hver
- Svart skjerm med hvit tekst: "ESP32-S3 TFT Test"

---

## Steg 5: Feilsøking

### Problem: Hvit/blank skjerm

**Løsning 1:** Feil driver
- Prøv ST7789 istedenfor ILI9341 (eller omvendt)

**Løsning 2:** Feil pins
- Dobbeltsjekk kabling mot TFT-modulen
- Vanlige alternative pins for ESP32-S3:
  ```
  MOSI: GPIO 11 eller 13
  SCLK: GPIO 12 eller 14
  CS:   GPIO 10 eller 15
  DC:   GPIO 8  eller 2
  RST:  GPIO 9  eller 4
  ```

**Løsning 3:** Backlight ikke på
- Hvis TFT har BL (backlight) pin, koble den til 3.3V
- Eller kontroller med GPIO:
  ```cpp
  pinMode(46, OUTPUT);
  digitalWrite(46, HIGH);  // Skru på backlight
  ```

### Problem: Farger feil

**Årsak:** Feil fargeformat (RGB vs BGR)

**Løsning:** Legg til i User_Setup.h:
```cpp
#define TFT_RGB_ORDER TFT_BGR  // Eller TFT_RGB
```

### Problem: Display opp-ned

**Løsning:** Endre rotation:
```cpp
tft.setRotation(2);  // Prøv 0, 1, 2, 3
```

---

## Alternative pin-konfigurasjoner

### Hvis du bruker andre pins:

Endre i User_Setup.h:

```cpp
// Eksempel: Bruke andre GPIO
#define TFT_MOSI 13   // Endre til din MOSI pin
#define TFT_SCLK 14   // Endre til din CLK pin
#define TFT_CS   15   // Endre til din CS pin
#define TFT_DC   2    // Endre til din DC pin
#define TFT_RST  4    // Endre til din RST pin
```

**VIKTIG:** Unngå disse GPIO på ESP32-S3:
- GPIO 0: Boot-knapp
- GPIO 19, 20: USB (hvis du bruker USB)
- GPIO 26-32: Ikke eksisterer på S3
- GPIO 33-37: PSRAM (ikke bruk hvis du har PSRAM)

---

## Anbefalt pin-oppsett for ESP32-S3-N8R8

```
Display Pin    ESP32-S3 GPIO    Kommentar
───────────    ─────────────    ─────────
VCC            3.3V             Strøm
GND            GND              Jord
CS             10               Chip Select
RESET          9                Reset
DC/RS          8                Data/Command
MOSI/SDA       11               SPI Data
SCK/SCL        12               SPI Clock
LED/BL         46               Backlight (valgfri)
MISO           13               (ikke nødvendig)

CAN TX         17               CAN transceiver TX
CAN RX         18               CAN transceiver RX
```

---

## Hurtig-test kommandoer

### Test 1: Er displayet koblet til?

```cpp
tft.init();
Serial.println("Display initialized");
```

Hvis dette krasjer: sjekk pins.

### Test 2: Kan vi tegne?

```cpp
tft.fillScreen(TFT_RED);
```

Hvis ingenting skjer: sjekk backlight og driver.

### Test 3: Er touch aktivert? (ikke nødvendig)

```cpp
uint16_t x, y;
if (tft.getTouch(&x, &y)) {
    Serial.printf("Touch: %d, %d\n", x, y);
}
```

---

## Ferdig konfigurasjon

Når displayet fungerer:
1. ✅ Last opp `master_esp32s3.ino`
2. ✅ Åpne Serial Monitor (115200 baud)
3. ✅ Du skal se: "✓ Display klar (240x320)"
4. ✅ Skjermen skal vise 4 hjul-rammer med "Waiting..."

Deretter:
- Koble CAN-modul til GPIO 17/18
- Test med satellitt

---

## Ytterligere ressurser

- [TFT_eSPI GitHub](https://github.com/Bodmer/TFT_eSPI)
- [ESP32-S3 Pinout](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html)
- [ILI9341 Datasheet](https://cdn-shop.adafruit.com/datasheets/ILI9341.pdf)

God testing! 🖥️

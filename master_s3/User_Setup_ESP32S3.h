// ═══════════════════════════════════════════════════════════
// TFT_eSPI User_Setup for ESP32-S3 44-pin + ILI9341
// VIKTIG: Kopier dette til ~/Documents/Arduino/libraries/TFT_eSPI/User_Setup.h
// ═══════════════════════════════════════════════════════════

// KOMMENTER UT ALT ANNET I FILEN! Bruk KUN dette oppsettet.

#define USER_SETUP_INFO "ESP32S3_ILI9341"

// Driver
#define ILI9341_DRIVER
#define ILI9341_2_DRIVER  // Alternative driver

// Display size
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// ESP32-S3 pins
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC   8
#define TFT_RST  9

// Optional
#define TFT_MISO 13
#define TFT_BL   14
#define TFT_BACKLIGHT_ON HIGH

// Fonts
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font
#define LOAD_FONT2  // Font 2. Small 16 pixel high font
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font
#define LOAD_FONT6  // Font 6. Large 48 pixel font
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font
#define LOAD_FONT8  // Font 8. Large 75 pixel font
#define LOAD_GFXFF  // FreeFonts

#define SMOOTH_FONT

// SPI Frequency
#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

// Color order (prøv begge hvis farger er feil)
//#define TFT_RGB_ORDER TFT_RGB  // Colour order Red-Green-Blue
//#define TFT_RGB_ORDER TFT_BGR  // Colour order Blue-Green-Red

// ═══════════════════════════════════════════════════════════
// VIKTIG for ESP32-S3:
// ═══════════════════════════════════════════════════════════

// Unngå DMA hvis det krasjer
#define DISABLE_ALL_LIBRARY_WARNINGS

// ESP32 specific
#ifdef ESP32
  #define SUPPORT_TRANSACTIONS
#endif

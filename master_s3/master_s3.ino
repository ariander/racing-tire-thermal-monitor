/*
 * TIRE TEMP DASHBOARD - MINIMAL & STABLE
 * ESP32-S3 + ILI9341 2.8" TFT + CAN
 *
 * Mottar thermal data fra 4x satellitter over CAN
 * Viser 4x heatmaps (16x12 pixels hver)
 */

#include <TFT_eSPI.h>
#include <SPI.h>
#include "driver/twai.h"

TFT_eSPI tft = TFT_eSPI();

// KONFIGURASJON
#define CAN_RX_PIN 18
#define CAN_TX_PIN 17
#define MODE_BUTTON_PIN 1  // BOOT button eller ekstern knapp til GND

#define HEATMAP_W 160
#define HEATMAP_H 80

// Display modes
enum DisplayMode {
  MODE_FIXED,      // Fast 25-99°C range
  MODE_DYNAMIC_PER_WHEEL,  // Auto range per hjul
  MODE_DYNAMIC_GLOBAL      // Auto range globalt
};
DisplayMode display_mode = MODE_FIXED;

// Wheel data
uint8_t thermal_raw[4][192];      // 32x6 compressed data from satellites
uint8_t thermal_prev[4][192];     // Previous frame (for dirty detection)
unsigned long last_rx[4] = {0, 0, 0, 0};
unsigned long last_draw[4] = {0, 0, 0, 0};
float max_temp_30s[4] = {0, 0, 0, 0};  // Max temp siste 30 sekunder per hjul
unsigned long max_temp_time[4] = {0, 0, 0, 0};  // Når max temp ble målt
const char* names[] = {"FL", "FR", "RL", "RR"};

struct Pos { int x; int y; };
Pos positions[4] = {
  {0, 0},      // FL
  {160, 0},    // FR
  {0, 112},    // RL
  {160, 112}   // RR
};

// Stats
unsigned long packets = 0;

// Color map (iron)
uint16_t temp_color(uint8_t v) {
  if (v < 51) return tft.color565(v*2, 0, 255-v);
  else if (v < 102) { v -= 51; return tft.color565(128+v*2, 0, 128-v*2); }
  else if (v < 153) { v -= 102; return tft.color565(255, v*3, 0); }
  else if (v < 204) { v -= 153; return tft.color565(255, 153+v*2, 0); }
  else { v -= 204; return tft.color565(255, 255, v*5); }
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== TIRE TEMP DASHBOARD ===\n");

    // Mode button setup
    pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);

    // SPI + Display
    SPI.begin(12, 13, 11, 10);
    tft.init();
    tft.setRotation(1);  // 320x240
    tft.fillScreen(TFT_BLACK);

    Serial.println("OK: Display initialized");

    // CAN - med større RX buffer for å unngå packet drops
    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
    g.rx_queue_len = 50;  // Øk fra default 5 til 50

    twai_timing_config_t t = TWAI_TIMING_CONFIG_125KBITS();
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g, &t, &f) != ESP_OK || twai_start() != ESP_OK) {
        Serial.println("ERROR: CAN init failed!");
        tft.setTextColor(TFT_RED);
        tft.drawString("CAN ERROR", 100, 100, 4);
        while(1) delay(1000);
    }

    Serial.println("OK: CAN initialized");
    Serial.println("\nWaiting for data...\n");

    // Draw initial "waiting" screens
    for (int i = 0; i < 4; i++) {
        draw_no_signal(i);
    }

    draw_colorbar();
    draw_mode_indicator();  // Draw initially
}

void loop() {
    static unsigned long last_update = 0;
    static unsigned long last_print = 0;
    static unsigned long last_button = 0;
    static bool last_button_state = HIGH;
    unsigned long now = millis();

    // Check mode button (debounced)
    bool button_state = digitalRead(MODE_BUTTON_PIN);
    if (button_state == LOW && last_button_state == HIGH && (now - last_button > 300)) {
        // Button pressed - cycle through modes
        display_mode = (DisplayMode)((display_mode + 1) % 3);
        last_button = now;

        const char* mode_names[] = {"FIXED 25-70C", "DYNAMIC/WHEEL", "DYNAMIC/GLOBAL"};
        Serial.printf("Mode: %s\n", mode_names[display_mode]);

        // Force full redraw
        for (int i = 0; i < 4; i++) {
            last_draw[i] = 0;
        }
        draw_colorbar();  // This also calls draw_mode_indicator()
    }
    last_button_state = button_state;

    // Process CAN messages - tøm buffer så raskt som mulig!
    twai_message_t msg;
    int msgs_processed = 0;
    while (twai_receive(&msg, 0) == ESP_OK && msgs_processed < 100) {
        msgs_processed++;
        packets++;

        // Map CAN ID to wheel index
        int w = -1;
        if (msg.identifier == 0x10) w = 0;       // FL
        else if (msg.identifier == 0x11) w = 1;  // FR
        else if (msg.identifier == 0x12) w = 2;  // RL
        else if (msg.identifier == 0x13) w = 3;  // RR

        if (w == -1) continue;

        // Hopp over frame start marker (vi bryr oss ikke om synkronisering)
        if (msg.data[0] == 0xFF && msg.data_length_code == 2) {
            last_rx[w] = now;
            continue;
        }

        // Extract data direkte til display buffer: [offset, data1...data7]
        int offset = msg.data[0];
        for (int i = 1; i < msg.data_length_code; i++) {
            int idx = offset + (i - 1);
            if (idx < 192) {
                thermal_raw[w][idx] = msg.data[i];
            }
        }

        last_rx[w] = now;
    }

    // Update display @ 10 FPS (100ms)
    if (now - last_update > 100) {
        for (int i = 0; i < 4; i++) {
            draw_heatmap(i);
        }
        draw_mode_indicator();  // Redraw mode indicator each frame
        last_update = now;
    }

    // Print status every 5s
    if (now - last_print > 5000) {
        Serial.printf("Uptime: %lus | Packets: %lu\n", now/1000, packets);
        for (int i = 0; i < 4; i++) {
            unsigned long age = (now - last_rx[i]) / 1000;
            bool active = (now - last_rx[i]) < 2000;
            Serial.printf("%s: %s (age: %lus)\n", names[i], active ? "OK" : "NO SIGNAL", age);
        }
        Serial.println();
        last_print = now;
    }

    yield();
}

void draw_heatmap(int w) {
    Pos p = positions[w];
    unsigned long now = millis();
    unsigned long age = now - last_rx[w];

    // Check timeout
    if (age > 2000) {
        draw_no_signal(w);
        return;
    }

    // Tegn kun hvis det har kommet ny data siden sist
    if (last_rx[w] <= last_draw[w]) {
        return;  // Ingen nye data
    }
    last_draw[w] = now;

    // Draw thermal pixels (32x6 raw → scale 5x pixels per cell)
    // 32 cols × 5px = 160px wide
    // 6 rows × 13px = 78px tall (fits in 80px)
    int pw = 5;   // pixel width
    int ph = 13;  // pixel height

    // Calculate min/max for this wheel (in raw bytes)
    uint8_t tmin_raw = 255, tmax_raw = 0;
    for (int i = 0; i < 192; i++) {
        if (thermal_raw[w][i] < tmin_raw) tmin_raw = thermal_raw[w][i];
        if (thermal_raw[w][i] > tmax_raw) tmax_raw = thermal_raw[w][i];
    }

    // Convert to actual Celsius (byte encoding: temp = byte / 2.0)
    float tmin_celsius = tmin_raw / 2.0;
    float tmax_celsius = tmax_raw / 2.0;

    // Track max temp over 30 seconds
    if (tmax_celsius > max_temp_30s[w] || (now - max_temp_time[w] > 30000)) {
        max_temp_30s[w] = tmax_celsius;
        max_temp_time[w] = now;
    }

    // For DYNAMIC_GLOBAL mode, find global min/max across all wheels
    uint8_t global_min = tmin_raw, global_max = tmax_raw;
    if (display_mode == MODE_DYNAMIC_GLOBAL) {
        for (int ww = 0; ww < 4; ww++) {
            if ((now - last_rx[ww]) < 2000) {  // Only active wheels
                for (int i = 0; i < 192; i++) {
                    if (thermal_raw[ww][i] < global_min) global_min = thermal_raw[ww][i];
                    if (thermal_raw[ww][i] > global_max) global_max = thermal_raw[ww][i];
                }
            }
        }
        tmin_raw = global_min;
        tmax_raw = global_max;
        tmin_celsius = tmin_raw / 2.0;
        tmax_celsius = tmax_raw / 2.0;
    }

    tft.startWrite();

    // Tegn ALLE pixler (full refresh)
    for (int y = 0; y < 6; y++) {
        for (int x = 0; x < 32; x++) {
            int idx = y * 32 + x;
            uint8_t val = thermal_raw[w][idx];

            // Remap value based on display mode
            uint8_t mapped_val;
            if (display_mode == MODE_FIXED) {
                // FIXED mode: Map 25-70°C to full color range (0-255)
                // Raw byte: 0-255 represents 0-127°C (temp = byte/2)
                // For colors: 25°C (byte=50) → 0, 70°C (byte=140) → 255
                float temp_c = val / 2.0;
                if (temp_c < 25.0) {
                    mapped_val = 0;
                } else if (temp_c > 70.0) {
                    mapped_val = 255;
                } else {
                    mapped_val = ((temp_c - 25.0) / 45.0) * 255;  // 45°C range
                }
            } else {
                // Dynamic modes - remap to 0-255 range based on min/max
                if (tmax_raw > tmin_raw) {
                    mapped_val = ((val - tmin_raw) * 255) / (tmax_raw - tmin_raw);
                } else {
                    mapped_val = 128;  // All same temp
                }
            }

            uint16_t color = temp_color(mapped_val);
            int px = p.x + x * pw;
            int py = p.y + y * ph;

            tft.fillRect(px, py, pw, ph, color);
        }
    }

    tft.endWrite();

    // Wheel label (top left)
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.setCursor(p.x + 0, p.y + 0);
    tft.print(names[w]);

    // Temperature text at bottom - larger and centered
    int ty = p.y + HEATMAP_H + 2;  // Below heatmap
    tft.fillRect(p.x, ty, HEATMAP_W, 30, TFT_BLACK);

    // Find hottest wheel globally
    int hottest_wheel = 0;
    float hottest_temp = max_temp_30s[0];
    for (int i = 1; i < 4; i++) {
        if ((now - last_rx[i]) < 2000 && max_temp_30s[i] > hottest_temp) {
            hottest_temp = max_temp_30s[i];
            hottest_wheel = i;
        }
    }


    // --- START NY LOGIKK ---
    
    // Default: Svart bakgrunn, hvit tekst
    uint16_t bg_color = TFT_BLACK;
    uint16_t txt_color = TFT_WHITE;
    
    float temp = max_temp_30s[w];
    bool blink_on = (now / 200) % 2 == 0; // Rask blink (250ms)

    if (temp > 75.0) {
        // OVER 75: KRITISK (Blinker Rød bakgrunn / Hvit tekst)
        if (blink_on) {
            bg_color = TFT_RED;
            txt_color = TFT_WHITE;
        } else {
            // I "av"-fasen: Svart bakgrunn, Rød tekst (så tallet alltid synes)
            bg_color = TFT_BLACK;
            txt_color = TFT_RED;
        }
    } 
    else if (temp >= 60.0) {
        // 60 - 75: VARMT (Gul tekst, svart bakgrunn)
        txt_color = TFT_YELLOW;
    } 
    else if (temp >= 40.0) {
        // 40 - 60: IDEELT (Grønn bakgrunn, Svart tekst)
        // Dette gir super kontrast og sier "OK" veldig tydelig
        bg_color = TFT_GREEN;
        txt_color = TFT_BLACK;
    } 
    else {
        // UNDER 40: KALDT (Cyan tekst)
        txt_color = TFT_CYAN; 
    }

    // 1. Tegn bakgrunnsboksen (dekker over gamle tall)
    tft.fillRect(p.x, ty, HEATMAP_W, 30, bg_color);

    // 2. Konfigurer tekst
    tft.setTextColor(txt_color, bg_color);
    tft.setTextFont(4);

    // 3. Formater tallet
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%.0f", temp);

    // 4. Sentrer tekst
    int text_width = strlen(temp_str) * 14; 
    int text_x = p.x + (HEATMAP_W - text_width) / 2;

    // 5. Skriv ut
    tft.setCursor(text_x, ty);
    tft.print(temp_str);
    
    // Tegn gradetegn (liten sirkel)
    tft.drawCircle(text_x + text_width + 4, ty + 4, 2, txt_color);
    
    // --- SLUTT NY LOGIKK ---
}

void draw_no_signal(int w) {
    Pos p = positions[w];

    tft.fillRect(p.x, p.y, HEATMAP_W, HEATMAP_H, TFT_DARKGREY);

    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.setTextFont(2);
    tft.setCursor(p.x + 2, p.y + 2);
    tft.print(names[w]);

    tft.setTextColor(TFT_RED, TFT_DARKGREY);
    tft.setTextFont(2);
    tft.setCursor(p.x + 35, p.y + 35);
    tft.print("NO");
    tft.setCursor(p.x + 25, p.y + 50);
    tft.print("SIGNAL");

    // Clear temp text area below heatmap
    tft.fillRect(p.x, p.y + HEATMAP_H, HEATMAP_W, 32, TFT_BLACK);
}

void draw_colorbar() {
    int y = 230;
    int x0 = 28;
    int w = 256;

    // Clear colorbar area
    tft.fillRect(0, y - 10, 320, 20, TFT_BLACK);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);

    if (display_mode == MODE_FIXED) {
        // Fixed 25-70°C visual range (shows actual temp in numbers above)
        tft.setCursor(1, y - 5);
        tft.print("25");
        tft.drawCircle(18, y, 2, TFT_WHITE);

        tft.setCursor(290, y - 5);
        tft.print("70");
        tft.drawCircle(310, y, 2, TFT_WHITE);
    } else if (display_mode == MODE_DYNAMIC_PER_WHEEL) {
        // Dynamic per wheel
        tft.setCursor(1, y - 5);
        tft.print("MIN");
        tft.setCursor(290, y - 5);
        tft.print("MAX");
    } else {
        // Dynamic global
        tft.setCursor(1, y - 5);
        tft.print("MIN");
        tft.setCursor(290, y - 5);
        tft.print("MAX");
    }

    for (int i = 0; i < w; i++) {
        uint16_t c = temp_color((i * 255) / w);
        tft.drawFastVLine(x0 + i, y, 7, c);
    }

    // Draw mode indicator in center between top wheels
    draw_mode_indicator();
}

void draw_mode_indicator() {
    // Mode indicator area (center, between upper and lower wheels)
    int x = 149;  // Your custom position
    int y = 85;   // Your custom position
    int w_box = 29;
    int h_box = 20;

    // Set background color and text based on mode
    uint16_t bg_color, text_color;
    const char* mode_text;

    if (display_mode == MODE_FIXED) {
        bg_color = TFT_BLUE;      // Blue background for FIXED
        text_color = TFT_BLACK;   // Black text
        mode_text = "FIX";
    } else if (display_mode == MODE_DYNAMIC_PER_WHEEL) {
        bg_color = TFT_RED;       // Red background for DYNAMIC/WHEEL
        text_color = TFT_BLACK;   // Black text
        mode_text = "DYN";
    } else {
        bg_color = TFT_GREEN;     // Green background for GLOBAL
        text_color = TFT_BLACK;   // Black text
        mode_text = "GLO";
    }

    // Draw colored background box
    tft.fillRect(x, y, w_box, h_box, bg_color);

    // Draw mode text (bold effect: draw twice with 1px offset)
    tft.setTextFont(2);
    tft.setTextColor(text_color, bg_color);

    // Bold effect - draw text twice
    tft.setCursor(x + 3, y + 2);
    tft.print(mode_text);
}

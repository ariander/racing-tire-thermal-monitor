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

#define HEATMAP_W 160
#define HEATMAP_H 80

// Wheel data
uint8_t thermal_raw[4][192];      // 32x6 compressed data from satellites
uint8_t thermal_prev[4][192];     // Previous frame (for dirty detection)
unsigned long last_rx[4] = {0, 0, 0, 0};
unsigned long last_draw[4] = {0, 0, 0, 0};
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
}

void loop() {
    static unsigned long last_update = 0;
    static unsigned long last_print = 0;
    unsigned long now = millis();

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

    tft.startWrite();

    // Tegn ALLE pixler (full refresh)
    for (int y = 0; y < 6; y++) {
        for (int x = 0; x < 32; x++) {
            int idx = y * 32 + x;
            uint8_t val = thermal_raw[w][idx];

            uint16_t color = temp_color(val);
            int px = p.x + x * pw;
            int py = p.y + y * ph;

            tft.fillRect(px, py, pw, ph, color);
        }
    }

    tft.endWrite();

    // Calculate min/max
    uint8_t tmin = 255, tmax = 0;
    for (int i = 0; i < 192; i++) {
        if (thermal_raw[w][i] < tmin) tmin = thermal_raw[w][i];
        if (thermal_raw[w][i] > tmax) tmax = thermal_raw[w][i];
    }

    // Convert to Celsius (satellite sends: byte = temp * 2.55)
    int min_c = tmin / 2.55;
    int max_c = tmax / 2.55;

    // Wheel label
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.setCursor(p.x + 2, p.y + 2);
    tft.print(names[w]);

    // Temperature text at bottom
    int ty = p.y + HEATMAP_H - 1;
    tft.fillRect(p.x, ty, HEATMAP_W, 13, TFT_BLACK);

    tft.setTextFont(4);
    tft.setCursor(p.x + 20, ty);
    tft.printf("%d", min_c);
    tft.drawCircle(p.x + 50, ty + 3, 2, TFT_WHITE);
    tft.setCursor(p.x + 54, ty);
    tft.print("-");
    tft.setCursor(p.x + 68, ty);
    tft.printf("%d", max_c);
    tft.drawCircle(p.x + 98, ty + 3, 2, TFT_WHITE);
}

void draw_no_signal(int w) {
    Pos p = positions[w];

    tft.fillRect(p.x, p.y, HEATMAP_W, HEATMAP_H, TFT_DARKGREY);

    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.setTextFont(2);
    tft.setCursor(p.x + 2, p.y + 2);
    tft.print(names[w]);

    tft.setTextColor(TFT_RED, TFT_DARKGREY);
    tft.setCursor(p.x + 35, p.y + 35);
    tft.print("NO");
    tft.setCursor(p.x + 25, p.y + 50);
    tft.print("SIGNAL");

    tft.fillRect(p.x, p.y + HEATMAP_H - 13, HEATMAP_W, 13, TFT_BLACK);
}

void draw_colorbar() {
    int y = 230;
    int x0 = 28;
    int w = 256;

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.setCursor(1, y - 5);
    tft.print("0");
    tft.drawCircle(11, y, 2, TFT_WHITE);

    tft.setCursor(288, y - 5);
    tft.print("100");
    tft.drawCircle(316, y, 2, TFT_WHITE);

    for (int i = 0; i < w; i++) {
        uint16_t c = temp_color((i * 255) / w);
        tft.drawFastVLine(x0 + i, y, 7, c);
    }
}

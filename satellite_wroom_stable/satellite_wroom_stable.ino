/*
 * TIRE TEMP SATELLITE - MINIMAL & STABLE
 * ESP32-WROOM-32 + GY-MCU90640 (med SET I2C loddet!) + CAN
 *
 * KABLING:
 * --------
 * GY-MCU90640:
 *   VIN  → 3.3V
 *   GND  → GND
 *   SDA  → GPIO 21
 *   SCL  → GPIO 22
 *   VIKTIG: "SET I2C" må være loddet sammen!
 *
 * CAN Transceiver:
 *   TX   → GPIO 5
 *   RX   → GPIO 4
 */

#include <Wire.h>
#include <Adafruit_MLX90640.h>
#include "driver/twai.h"

// KONFIGURASJON
#define WHEEL_ID 0x10
const char* WHEEL_NAME = "FL";

#define CAN_TX_PIN 5
#define CAN_RX_PIN 4
#define MLX_SDA 21
#define MLX_SCL 22

// Objekter
Adafruit_MLX90640 mlx;
float frame[32*24];
uint8_t compressed[192];  // 32x6 compressed

// Statistikk
unsigned long frames_sent = 0;
unsigned long last_print = 0;

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n=== TIRE TEMP: " + String(WHEEL_NAME) + " ===\n");

    // I2C
    Wire.begin(MLX_SDA, MLX_SCL);
    Wire.setClock(100000);  // 100kHz - stabilt for GY-MCU90640

    if (!mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
        Serial.println("ERROR: MLX90640 init failed!");
        Serial.println("Check: SET I2C loddet? Power? Wiring?");
        while(1) delay(1000);
    }

    mlx.setMode(MLX90640_INTERLEAVED);
    mlx.setResolution(MLX90640_ADC_18BIT);
    mlx.setRefreshRate(MLX90640_2_HZ);

    Serial.println("OK: MLX90640 initialized");

    // CAN
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        Serial.println("ERROR: CAN install failed!");
        while(1) delay(1000);
    }

    if (twai_start() != ESP_OK) {
        Serial.println("ERROR: CAN start failed!");
        while(1) delay(1000);
    }

    Serial.println("OK: CAN initialized");
    Serial.println("\nStreaming data...\n");
}

void loop() {
    // Les frame (blokker inntil klar)
    if (mlx.getFrame(frame) != 0) {
        Serial.println("Frame read failed, skipping...");
        delay(100);
        return;
    }

    // Komprimer 32x24 → 32x6
    compress_frame();

    // Send over CAN
    send_can();

    frames_sent++;

    // Print status hvert 5. sekund
    if (millis() - last_print > 5000) {
        last_print = millis();

        float tMin = 1000, tMax = -1000;
        for (int i = 0; i < 768; i++) {
            if (frame[i] < tMin) tMin = frame[i];
            if (frame[i] > tMax) tMax = frame[i];
        }

        Serial.printf("Frames: %lu | Temp: %.1f - %.1f C\n", frames_sent, tMin, tMax);
    }

    // Ingen delay - getFrame() blokkerer til sensor har data (2Hz)
}

void compress_frame() {
    // Gjennomsnitt 4 rader vertikalt: 32x24 → 32x6
    for (int y = 0; y < 6; y++) {
        for (int x = 0; x < 32; x++) {
            float sum = 0;
            for (int dy = 0; dy < 4; dy++) {
                sum += frame[(y*4 + dy) * 32 + x];
            }
            float avg = sum / 4.0;

            // Normaliser til 0-255 (anta 0-100°C range)
            int val = (int)(avg * 2.55);
            if (val < 0) val = 0;
            if (val > 255) val = 255;

            compressed[y * 32 + x] = val;
        }
    }
}

void send_can() {
    // Send frame start marker først
    static uint8_t frame_id = 0;
    frame_id++;

    twai_message_t msg;
    msg.identifier = WHEEL_ID;
    msg.data_length_code = 2;
    msg.flags = TWAI_MSG_FLAG_NONE;
    msg.data[0] = 0xFF;  // Frame start marker
    msg.data[1] = frame_id;
    twai_transmit(&msg, pdMS_TO_TICKS(100));
    delay(2);  // 2ms mellom marker og data

    // Send 192 bytes som 28 CAN-meldinger (7 bytes data per melding)
    msg.data_length_code = 8;
    for (int chunk = 0; chunk < 28; chunk++) {
        msg.data[0] = chunk * 7;  // Offset

        for (int i = 0; i < 7; i++) {
            int idx = chunk * 7 + i;
            msg.data[i + 1] = (idx < 192) ? compressed[idx] : 0;
        }

        twai_transmit(&msg, pdMS_TO_TICKS(100));
        delay(1);  // 1ms mellom pakker (økt fra 500us)
    }
}

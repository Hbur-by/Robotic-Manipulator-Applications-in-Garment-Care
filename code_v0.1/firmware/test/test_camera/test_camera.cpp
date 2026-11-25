/**
 * RMAGC — OV2640 Camera Test
 *
 * Initializes OV2640 camera on ESP32-S3 and streams JPEG frames over serial.
 * Requires PSRAM to be properly configured in platformio.ini.
 *
 * Protocol: 4-byte little-endian length header + JPEG data.
 * Use software/camera-tools/camera_viewer.py on the PC side to view.
 */

#include "esp_camera.h"
#include <Arduino.h>

// ==================== Camera Pin Definitions ====================
#define PWDN_GPIO_NUM     17
#define RESET_GPIO_NUM    16
#define XCLK_GPIO_NUM     38
#define SIOD_GPIO_NUM     5
#define SIOC_GPIO_NUM     4

#define Y9_GPIO_NUM       15
#define Y8_GPIO_NUM       14
#define Y7_GPIO_NUM       13
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       11
#define Y4_GPIO_NUM       10
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM       8

#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM     39
#define LED_PIN           21

void setup() {
  Serial.begin(1500000);  // High baud for image streaming
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(800);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk  = XCLK_GPIO_NUM;
  config.pin_pclk  = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href  = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn  = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;

  // Balanced settings for stability
  config.pixel_format  = PIXFORMAT_JPEG;
  config.frame_size    = FRAMESIZE_QVGA;   // 320×240
  config.jpeg_quality  = 18;               // Good quality, moderate size
  config.fb_count      = 1;                // Single buffer for stability

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    // Blink LED to indicate failure
    while (1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(200);
    }
  }
  digitalWrite(LED_PIN, LOW);
  Serial.println("Camera initialized OK");
}

void loop() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    delay(15);
    return;
  }

  // Send 4-byte length header (little-endian)
  Serial.write((uint8_t*)&fb->len, 4);
  // Send JPEG data
  Serial.write(fb->buf, fb->len);
  Serial.flush();

  esp_camera_fb_return(fb);
  delay(15);  // Rate limit to prevent overflow
}

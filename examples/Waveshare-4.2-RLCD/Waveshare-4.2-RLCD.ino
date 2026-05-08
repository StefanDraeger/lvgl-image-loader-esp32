/**
 * LvglImageLoader - Example for Waveshare RLCD 4.2"
 *
 * Hardware:
 * - Waveshare RLCD 4.2" (400x300) (https://www.waveshare.com/esp32-s3-rlcd-4.2.htm)
 * - ESP32-S3 with PSRAM
 * 
 * Wiring:
 * - GPIO 12 → MOSI
 * - GPIO 11 → CLK
 * - GPIO  5 → CS
 * - GPIO 40 → DC
 * - GPIO 41 → RST
 */

#include "display_bsp.h"
#include "src/app_bsp/lvgl_bsp.h"
#include "src/ui_src/generated/gui_guider.h"
#include <lvgl.h>
#include <WiFi.h>
#include "lvgl_image_loader.h"

#define MOSI 12
#define SCL 11
#define DC 5
#define CS 40
#define RST 41
#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 300

const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";
const char *imageUrl = "https://github.com/StefanDraeger/lvgl-image-loader-esp32/blob/main/images/beispiel.jpg?raw=true";

const int IMAGE_WIDTH = 200;
const int IMAGE_HEIGHT = 200;

DisplayPort RlcdPort(MOSI, SCL, DC, CS, RST, SCREEN_WIDTH, SCREEN_HEIGHT);
LvglImageLoader imageLoader;

static void Lvgl_FlushCallback(lv_display_t *drv, const lv_area_t *area,
                               uint8_t *color_map) {
  uint16_t *buffer = (uint16_t *)color_map;
  for (int y = area->y1; y <= area->y2; y++) {
    for (int x = area->x1; x <= area->x2; x++) {
      uint8_t color = (*buffer < 0x7fff) ? ColorBlack : ColorWhite;
      RlcdPort.RLCD_SetPixel(x, y, color);
      buffer++;
    }
  }
  RlcdPort.RLCD_Display();
  lv_disp_flush_ready(drv);
}

void setup() {
  Serial.begin(115200);
  RlcdPort.RLCD_Init();
  Lvgl_PortInit(400, 300, Lvgl_FlushCallback);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nVerbunden!");

  if (Lvgl_lock(-1)) {
    // Screen vorbereiten
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);

    // Container mittig
    lv_obj_t *container = lv_obj_create(screen);
    lv_obj_set_size(container, IMAGE_WIDTH, IMAGE_HEIGHT);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(container, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(container);

    // Bild laden
    imageLoader.loadImage(imageUrl, container, IMAGE_WIDTH, IMAGE_HEIGHT);

    Lvgl_unlock();
  }
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
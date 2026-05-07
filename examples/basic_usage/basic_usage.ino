/**
 * Basic Usage Example
 * 
 * Shows how to use LvglImageLoader to display
 * a JPEG image from a HTTPS URL in a LVGL object.
 */

#include <lvgl.h>
#include <WiFi.h>
#include "lvgl_image_loader.h"

const char *ssid     = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";
const char *imageUrl = "https://example.com/image.jpg";

LvglImageLoader imageLoader;
lv_obj_t *imageContainer;

void setup() {
    Serial.begin(115200);

    // WiFi verbinden
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nVerbunden!");

    // LVGL initialisieren
    lv_init();

    // Container erstellen
    imageContainer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(imageContainer, 144, 144);
    lv_obj_center(imageContainer);

    // Optionale Einstellungen
    imageLoader.setMaxRetries(3);
    imageLoader.setRetryDelay(5000);
    imageLoader.setTimeout(15000);

    // Bild laden und anzeigen
    imageLoader.loadImage(imageUrl, imageContainer, 144, 144);
}

void loop() {
    lv_timer_handler();
    delay(5);
}
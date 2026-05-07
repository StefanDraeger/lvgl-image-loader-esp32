/**
 * LVGL Image Loader for ESP32
 * 
 * Downloads and decodes JPEG images from HTTPS URLs
 * and displays them as LVGL image objects.
 * 
 * Author:  [Dein Name]
 * License: MIT
 * GitHub:  https://github.com/[dein-name]/lvgl-image-loader-esp32
 */

#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <JPEGDEC.h>
#include <lvgl.h>

class LvglImageLoader {
public:
    /**
     * Constructor
     */
    LvglImageLoader();

    /**
     * Destructor
     * Frees allocated PSRAM buffers
     */
    ~LvglImageLoader();

    /**
     * Downloads a JPEG image from a HTTPS URL,
     * decodes it and displays it in a LVGL image object.
     *
     * @param url       HTTPS URL to the JPEG image
     * @param container LVGL object to display the image in
     * @param width     Expected image width in pixels
     * @param height    Expected image height in pixels
     * @return          true if successful, false otherwise
     */
    bool loadImage(const char *url, lv_obj_t *container, 
                   int width, int height);

    /**
     * Sets the number of retries on connection failure
     * Default: 3
     *
     * @param retries Number of retries
     */
    void setMaxRetries(int retries) { _maxRetries = retries; }

    /**
     * Sets the delay between retries in milliseconds
     * Default: 5000ms
     *
     * @param delayMs Delay in milliseconds
     */
    void setRetryDelay(int delayMs) { _retryDelayMs = delayMs; }

    /**
     * Sets the connection timeout in milliseconds
     * Default: 15000ms
     *
     * @param timeoutMs Timeout in milliseconds
     */
    void setTimeout(int timeoutMs) { _timeoutMs = timeoutMs; }

private:
    // Einstellungen
    int _maxRetries   = 3;
    int _retryDelayMs = 5000;
    int _timeoutMs    = 15000;

    // Interne Buffer
    uint16_t *_imageBuffer = nullptr;
    uint8_t  *_jpgBuffer   = nullptr;
    size_t    _jpgSize     = 0;
    int       _imageWidth  = 0;
    int       _imageHeight = 0;

    // LVGL Image Descriptor (static damit LVGL darauf zugreifen kann)
    lv_image_dsc_t _imgDsc;

    // JPEGDEC Instanz
    JPEGDEC _jpeg;

    // Private Methoden
    bool _downloadImage(const char *url);
    bool _decodeJpeg();
    void _freeBuffers();

    // JPEG Callback (muss static sein für JPEGDEC)
    static int _jpegDrawCallback(JPEGDRAW *pDraw);

    // Statischer Zeiger auf aktuelle Instanz für Callback
    static LvglImageLoader *_instance;
};
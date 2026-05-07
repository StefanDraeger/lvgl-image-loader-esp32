#include "lvgl_image_loader.h"

// Statische Instanz für JPEG Callback
LvglImageLoader *LvglImageLoader::_instance = nullptr;

LvglImageLoader::LvglImageLoader() {
    _instance = this;
}

LvglImageLoader::~LvglImageLoader() {
    _freeBuffers();
}

bool LvglImageLoader::loadImage(const char *url, lv_obj_t *container,
                                 int width, int height) {
    _imageWidth  = width;
    _imageHeight = height;

    // Alten Buffer freigeben
    _freeBuffers();

    // Neuen Bildbuffer im PSRAM anlegen
    _imageBuffer = (uint16_t *)ps_malloc(width * height * sizeof(uint16_t));
    if (!_imageBuffer) {
        Serial.println("[ImageLoader] Nicht genug PSRAM fuer imageBuffer!");
        return false;
    }

    // Buffer mit Weiss fuellen
    for (int i = 0; i < width * height; i++) {
        _imageBuffer[i] = 0xFFFF;
    }

    // Warten damit vorheriger SSL-Kontext freigegeben wird
    Serial.println("[ImageLoader] Warte auf SSL-Cleanup...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Bild herunterladen mit Retry
    bool success  = false;
    int retryCount = 0;

    while (!success && retryCount < _maxRetries) {
        if (retryCount > 0) {
            Serial.printf("[ImageLoader] Retry %d von %d...\n", 
                          retryCount, _maxRetries);
            vTaskDelay(pdMS_TO_TICKS(_retryDelayMs));
        }

        success = _downloadImage(url);
        retryCount++;
    }

    if (!success) {
        Serial.println("[ImageLoader] Download fehlgeschlagen!");
        _freeBuffers();
        return false;
    }

    // JPEG dekodieren
    if (!_decodeJpeg()) {
        Serial.println("[ImageLoader] Dekodierung fehlgeschlagen!");
        _freeBuffers();
        return false;
    }

    // LVGL Image Descriptor befüllen
    _imgDsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    _imgDsc.header.cf    = LV_COLOR_FORMAT_RGB565;
    _imgDsc.header.w     = width;
    _imgDsc.header.h     = height;
    _imgDsc.data_size    = width * height * 2;
    _imgDsc.data         = (const uint8_t *)_imageBuffer;

    // LVGL Container leeren und Bild anzeigen
    lv_obj_clean(container);
    lv_obj_t *img = lv_image_create(container);
    lv_image_set_src(img, &_imgDsc);
    lv_obj_center(img);

    Serial.println("[ImageLoader] Bild erfolgreich angezeigt!");
    return true;
}

bool LvglImageLoader::_downloadImage(const char *url) {
    // URL parsen
    String urlStr     = String(url);
    String host       = "";
    String path       = "";
    bool   useSSL     = urlStr.startsWith("https://");
    int    port       = useSSL ? 443 : 80;
    int    startIndex = useSSL ? 8 : 7;
    int    slashIndex = urlStr.indexOf('/', startIndex);

    if (slashIndex == -1) {
        host = urlStr.substring(startIndex);
        path = "/";
    } else {
        host = urlStr.substring(startIndex, slashIndex);
        path = urlStr.substring(slashIndex);
    }

    Serial.printf("[ImageLoader] Verbinde mit %s:%d\n", host.c_str(), port);

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(_timeoutMs);

    if (!client.connect(host.c_str(), port)) {
        Serial.println("[ImageLoader] TCP Verbindung fehlgeschlagen!");
        return false;
    }

    // HTTP Request senden
    client.printf("GET %s HTTP/1.1\r\n", path.c_str());
    client.printf("Host: %s\r\n", host.c_str());
    client.println("User-Agent: ESP32-LVGL-ImageLoader/1.0");
    client.println("Accept: image/jpeg");
    client.println("Connection: close");
    client.println();

    // Status-Zeile lesen
    String responseLine = client.readStringUntil('\n');
    int httpCode = 0;
    if (responseLine.startsWith("HTTP/")) {
        httpCode = responseLine.substring(9, 12).toInt();
    }
    Serial.printf("[ImageLoader] HTTP Code: %d\n", httpCode);

    // Header lesen
    String redirectUrl = "";
    int contentLength  = -1;

    while (client.connected()) {
        String headerLine = client.readStringUntil('\n');
        headerLine.trim();
        if (headerLine.length() == 0) break;

        if (headerLine.startsWith("Content-Length:") ||
            headerLine.startsWith("content-length:")) {
            contentLength = headerLine.substring(16).toInt();
        }
        if (headerLine.startsWith("Location:") ||
            headerLine.startsWith("location:")) {
            redirectUrl = headerLine.substring(10);
            redirectUrl.trim();
        }
    }

    // Redirect folgen
    if ((httpCode == 301 || httpCode == 302 || httpCode == 307)
         && redirectUrl.length() > 0) {
        Serial.printf("[ImageLoader] Redirect zu: %s\n", redirectUrl.c_str());
        client.stop();
        return _downloadImage(redirectUrl.c_str());
    }

    if (httpCode != 200) {
        Serial.printf("[ImageLoader] HTTP Fehler: %d\n", httpCode);
        client.stop();
        return false;
    }

    // Bilddaten lesen
    if (contentLength > 0) {
        _jpgSize   = contentLength;
        _jpgBuffer = (uint8_t *)ps_malloc(_jpgSize);

        if (!_jpgBuffer) {
            Serial.println("[ImageLoader] Nicht genug PSRAM!");
            client.stop();
            return false;
        }

        size_t index              = 0;
        unsigned long start       = millis();

        while (index < _jpgSize && (millis() - start) < (unsigned long)_timeoutMs) {
            if (client.available()) {
                int bytesRead  = client.readBytes(
                                    _jpgBuffer + index,
                                    min((size_t)client.available(),
                                        _jpgSize - index));
                index         += bytesRead;
                start          = millis();
            } else {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }

        Serial.printf("[ImageLoader] Gelesene Bytes: %d / %d\n", 
                      index, _jpgSize);

        if (index < _jpgSize) {
            Serial.println("[ImageLoader] Timeout beim Lesen!");
            client.stop();
            free(_jpgBuffer);
            _jpgBuffer = nullptr;
            return false;
        }

    } else {
        // Dynamisch lesen ohne Content-Length
        size_t bufferSize = 32768;
        _jpgBuffer = (uint8_t *)ps_malloc(bufferSize);

        if (!_jpgBuffer) {
            Serial.println("[ImageLoader] Nicht genug PSRAM!");
            client.stop();
            return false;
        }

        size_t index              = 0;
        unsigned long start       = millis();

        while (client.connected() && 
               (millis() - start) < (unsigned long)_timeoutMs) {
            if (client.available()) {
                if (index + 4096 > bufferSize) {
                    bufferSize   += 16384;
                    uint8_t *tmp  = (uint8_t *)ps_realloc(_jpgBuffer, bufferSize);
                    if (!tmp) {
                        Serial.println("[ImageLoader] ps_realloc fehlgeschlagen!");
                        break;
                    }
                    _jpgBuffer = tmp;
                }
                int bytesRead  = client.readBytes(
                                    _jpgBuffer + index,
                                    client.available());
                index         += bytesRead;
                start          = millis();
            } else {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }

        _jpgSize = index;
        Serial.printf("[ImageLoader] Gelesene Bytes: %d\n", _jpgSize);

        if (_jpgSize == 0) {
            Serial.println("[ImageLoader] Keine Daten gelesen!");
            client.stop();
            free(_jpgBuffer);
            _jpgBuffer = nullptr;
            return false;
        }
    }

    client.stop();
    return true;
}

bool LvglImageLoader::_decodeJpeg() {
    if (!_jpgBuffer || _jpgSize == 0) return false;

    bool success = false;

    if (_jpeg.openRAM(_jpgBuffer, _jpgSize, _jpegDrawCallback)) {
        Serial.println("[ImageLoader] JPEG decode startet...");
        _jpeg.decode(0, 0, 0);
        _jpeg.close();
        success = true;
        Serial.println("[ImageLoader] JPEG decode fertig.");
    } else {
        Serial.println("[ImageLoader] JPEG oeffnen fehlgeschlagen!");
    }

    free(_jpgBuffer);
    _jpgBuffer = nullptr;
    _jpgSize   = 0;

    return success;
}

void LvglImageLoader::_freeBuffers() {
    if (_imageBuffer) {
        free(_imageBuffer);
        _imageBuffer = nullptr;
    }
    if (_jpgBuffer) {
        free(_jpgBuffer);
        _jpgBuffer = nullptr;
    }
}

int LvglImageLoader::_jpegDrawCallback(JPEGDRAW *pDraw) {
    if (!_instance || !_instance->_imageBuffer) return 0;

    uint16_t *pixels = (uint16_t *)pDraw->pPixels;

    for (int y = 0; y < pDraw->iHeight; y++) {
        for (int x = 0; x < pDraw->iWidth; x++) {
            int targetX = pDraw->x + x;
            int targetY = pDraw->y + y;

            if (targetX < _instance->_imageWidth && 
                targetY < _instance->_imageHeight) {
                uint16_t color = pixels[y * pDraw->iWidth + x];

                // RGB565 → Grauwert
                uint8_t r = ((color >> 11) & 0x1F) * 255 / 31;
                uint8_t g = ((color >> 5)  & 0x3F) * 255 / 63;
                uint8_t b = (color & 0x1F)          * 255 / 31;

                uint8_t  brightness = (r + g + b) / 3;
                uint8_t  gray       = brightness >> 3;
                uint16_t gray565    = (gray << 11) | 
                                      ((gray << 5) & 0x07E0) | 
                                       gray;

                _instance->_imageBuffer[targetY * _instance->_imageWidth 
                                        + targetX] = gray565;
            }
        }
    }
    return 1;
}
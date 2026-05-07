# LVGL Image Loader for ESP32

Downloads JPEG images from HTTPS URLs and displays them 
as LVGL image objects on ESP32 with PSRAM.

## Features

- HTTPS support (via `WiFiClientSecure`)
- Automatic retry on connection failure
- Redirect handling (301, 302, 307)
- PSRAM buffer management
- Grayscale conversion for e-paper / RLCD displays
- Works with LVGL v9

## Requirements

| Library    | Version |
|------------|---------|
| LVGL       | >= 9.0  |
| JPEGDEC    | >= 1.0  |
| ArduinoJson| >= 7.0  |

## Hardware

- ESP32 with PSRAM (e.g. ESP32-S3)

## Installation

1. Download ZIP
2. Arduino IDE → Sketch → Include Library → Add .ZIP Library

## Usage

```cpp
#include "lvgl_image_loader.h"

LvglImageLoader imageLoader;

lv_obj_t *container = lv_obj_create(lv_scr_act());
lv_obj_set_size(container, 144, 144);

imageLoader.loadImage(
    "https://example.com/image.jpg",
    container,
    144,   // width
    144    // height
);
```

## Configuration

```cpp
imageLoader.setMaxRetries(3);      // Anzahl Versuche
imageLoader.setRetryDelay(5000);   // Wartezeit zwischen Versuchen (ms)
imageLoader.setTimeout(15000);     // Verbindungs-Timeout (ms)
```

## License

MIT License - see [LICENSE](LICENSE)
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

## Supported Hardware

| Board                        | PSRAM | Status     |
|------------------------------|-------|------------|
| Waveshare ESP32-S3 RLCD 4.2" | ✅    | ✅ Tested  |
| ESP32-S3 with PSRAM          | ✅    | ✅ Tested  |
| ESP32 with PSRAM             | ✅    | ⚠️ Untested|
| CYD ESP32-2432S028R          | ❌    | ❌ No PSRAM|

> **⚠️ Note:** This library requires an ESP32 with PSRAM.
> Boards without PSRAM (e.g. CYD ESP32-2432S028R) are
> not supported.

## Requirements

| Library    | Version |
|------------|---------|
| LVGL       | >= 9.0  |
| JPEGDEC    | >= 1.0  |

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
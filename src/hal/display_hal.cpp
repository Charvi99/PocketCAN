#include "display_hal.h"
#include <Arduino.h>

M5GFX DisplayHAL::display;
lv_disp_draw_buf_t DisplayHAL::draw_buf;
lv_color_t* DisplayHAL::buf1 = nullptr;
lv_color_t* DisplayHAL::buf2 = nullptr;
uint8_t DisplayHAL::current_brightness = 255;

bool DisplayHAL::init() {
    // Initialize M5Stack Tab5 display
    display.init();

    // Initialize LVGL
    lv_init();

    // Allocate double buffers in PSRAM for smoother rendering
    // Using partial buffering (10 rows at a time) to reduce memory usage
    size_t buffer_size = LVGL_PARTIAL_BUF_SIZE;

    buf1 = (lv_color_t*)heap_caps_malloc(
        sizeof(lv_color_t) * buffer_size,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );

    buf2 = (lv_color_t*)heap_caps_malloc(
        sizeof(lv_color_t) * buffer_size,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );

    if (!buf1 || !buf2) {
        Serial.println("ERROR: Failed to allocate LVGL buffers");
        if (buf1) free(buf1);
        if (buf2) free(buf2);
        return false;
    }

    // Initialize LVGL display buffer with double buffering
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buffer_size);

    // Register display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    disp_drv.flush_cb = flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.sw_rotate = 1;
    disp_drv.rotated = LV_DISP_ROT_90;  // Landscape mode
    lv_disp_drv_register(&disp_drv);

    // Set default brightness
    display.setBrightness(current_brightness);

    Serial.printf("Display HAL initialized with double buffering (%d KB per buffer)\n",
                  (buffer_size * sizeof(lv_color_t)) / 1024);
    return true;
}

void DisplayHAL::flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    display.pushImageDMA(area->x1, area->y1, w, h, (uint16_t*)&color_p->full);
    lv_disp_flush_ready(disp);
}

void DisplayHAL::set_brightness(uint8_t brightness) {
    current_brightness = brightness;
    display.setBrightness(brightness);
}

uint8_t DisplayHAL::get_brightness() {
    return current_brightness;
}

M5GFX& DisplayHAL::get_display() {
    return display;
}

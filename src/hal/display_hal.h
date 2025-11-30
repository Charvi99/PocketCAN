#pragma once

/**
 * Display Hardware Abstraction Layer
 * Manages M5Stack Tab5 display initialization and LVGL integration
 */

#include <M5GFX.h>
#include "lvgl.h"
#include "../config/hardware_config.h"

class DisplayHAL {
public:
    /**
     * Initialize display hardware and LVGL
     * @return true if successful
     */
    static bool init();

    /**
     * Set display brightness
     * @param brightness 0-255
     */
    static void set_brightness(uint8_t brightness);

    /**
     * Get current brightness
     * @return brightness value 0-255
     */
    static uint8_t get_brightness();

    /**
     * LVGL flush callback
     */
    static void flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);

    /**
     * Get display instance
     */
    static M5GFX& get_display();

private:
    static M5GFX display;
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t *buf;
    static uint8_t current_brightness;
};

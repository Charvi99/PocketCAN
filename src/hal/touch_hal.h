#pragma once

/**
 * Touch Input Hardware Abstraction Layer
 * Manages touchscreen input for LVGL
 */

#include "lvgl.h"
#include "display_hal.h"

class TouchHAL {
public:
    /**
     * Initialize touch input
     * @return true if successful
     */
    static bool init();

    /**
     * LVGL touch read callback
     */
    static void read_cb(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);

private:
    static bool initialized;
};

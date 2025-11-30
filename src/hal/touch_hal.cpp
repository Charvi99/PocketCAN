#include "touch_hal.h"
#include <Arduino.h>

bool TouchHAL::initialized = false;

bool TouchHAL::init() {
    // Register touch input device with LVGL
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = read_cb;
    lv_indev_drv_register(&indev_drv);

    initialized = true;
    Serial.println("Touch HAL initialized");
    return true;
}

void TouchHAL::read_cb(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    lgfx::touch_point_t tp[3];
    M5GFX& display = DisplayHAL::get_display();
    uint8_t touchpad = display.getTouchRaw(tp, 3);

    if (touchpad > 0) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = tp[0].x;
        data->point.y = tp[0].y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

#pragma once

/**
 * Splash Screen
 * Shows PocketCAN logo during startup
 */

#include "lvgl.h"

class ScreenSplash {
public:
    /**
     * Create and display splash screen
     * @param duration_ms How long to show splash (0 = manual dismiss)
     * @return Splash screen object
     */
    static lv_obj_t* create(uint32_t duration_ms = 2000);

    /**
     * Dismiss splash screen
     */
    static void dismiss(lv_obj_t* splash);

private:
    static void timer_cb(lv_timer_t* timer);
};

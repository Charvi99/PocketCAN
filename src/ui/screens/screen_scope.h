#pragma once

/**
 * Scope Screen
 * CAN Signal Analyzer / Oscilloscope
 * Shows raw serial output from Nucleo-G431RB for debugging
 */

#include "lvgl.h"

class ScreenScope {
public:
    /**
     * Create scope screen
     * @return Screen object
     */
    static lv_obj_t* create();

    /**
     * Update scope screen
     * @param screen Screen object
     */
    static void update(lv_obj_t* screen);

    /**
     * Cleanup when screen is destroyed
     */
    static void cleanup();

private:
    // UI Components
    static lv_obj_t* output_textarea;
    static lv_obj_t* stats_label;
    static lv_obj_t* status_indicator;

    // State
    static uint32_t last_update_time;
    static bool auto_scroll;

    // Layout
    static lv_obj_t* create_header(lv_obj_t* parent);
    static lv_obj_t* create_output_area(lv_obj_t* parent);
    static lv_obj_t* create_control_buttons(lv_obj_t* parent);

    // Event callbacks
    static void clear_button_cb(lv_event_t* e);
    static void autoscroll_cb(lv_event_t* e);
};

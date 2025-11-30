#pragma once

/**
 * Main Dashboard Screen
 * Grid-based navigation with large touch-friendly buttons
 */

#include "lvgl.h"
#include "ui/ui_manager.h"

class ScreenMain {
public:
    /**
     * Create main dashboard screen
     * @return Screen object
     */
    static lv_obj_t* create();

    /**
     * Update dashboard (refresh stats, time, etc.)
     * @param screen Screen object
     */
    static void update(lv_obj_t* screen);

private:
    /**
     * Create navigation grid
     */
    static lv_obj_t* create_nav_grid(lv_obj_t* parent);

    /**
     * Create a navigation tile
     */
    static lv_obj_t* create_nav_tile(lv_obj_t* parent, const char* icon, const char* title,
                                      const char* subtitle, uint32_t color, lv_event_cb_t callback);

    /**
     * Event callbacks
     */
    static void menu_button_cb(lv_event_t* e);
    static void sniffer_tile_cb(lv_event_t* e);
    static void transmit_tile_cb(lv_event_t* e);
    static void emulator_tile_cb(lv_event_t* e);
    static void settings_tile_cb(lv_event_t* e);

    static lv_obj_t* menu_panel;
};

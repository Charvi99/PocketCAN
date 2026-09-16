#pragma once

/**
 * Settings Screen
 * Configure application settings with persistent storage
 */

#include "lvgl.h"
#include "../../services/settings_manager.h"

class ScreenSettings {
public:
    /**
     * Create settings screen
     * @return Screen object
     */
    static lv_obj_t* create();

    /**
     * Update settings screen
     * @param screen Screen object
     */
    static void update(lv_obj_t* screen);

    /**
     * Cleanup when screen is destroyed
     */
    static void cleanup();

private:
    // UI Components
    static lv_obj_t* can_baud_dropdown;
    static lv_obj_t* brightness_slider;
    static lv_obj_t* brightness_label;
    static lv_obj_t* theme_switch;
    static lv_obj_t* auto_scroll_switch;
    static lv_obj_t* table_update_slider;
    static lv_obj_t* table_update_label;
    static lv_obj_t* auto_log_switch;
    static lv_obj_t* splash_switch;

    // Temporary settings (not saved until user clicks Save)
    static AppSettings temp_settings;

    // Layout creation
    static lv_obj_t* create_settings_list(lv_obj_t* parent);
    static lv_obj_t* create_button_row(lv_obj_t* parent);

    // Setting items
    static void create_can_baud_setting(lv_obj_t* parent);
    static void create_brightness_setting(lv_obj_t* parent);
    static void create_theme_setting(lv_obj_t* parent);
    static void create_auto_scroll_setting(lv_obj_t* parent);
    static void create_table_update_setting(lv_obj_t* parent);
    static void create_auto_log_setting(lv_obj_t* parent);
    static void create_splash_setting(lv_obj_t* parent);

    // Helper to create setting row
    static lv_obj_t* create_setting_row(lv_obj_t* parent, const char* title, const char* description = nullptr);

    // Event callbacks
    static void save_button_cb(lv_event_t* e);
    static void cancel_button_cb(lv_event_t* e);
    static void can_baud_cb(lv_event_t* e);
    static void brightness_cb(lv_event_t* e);
    static void theme_cb(lv_event_t* e);
    static void auto_scroll_cb(lv_event_t* e);
    static void table_update_cb(lv_event_t* e);
    static void auto_log_cb(lv_event_t* e);
    static void splash_cb(lv_event_t* e);

    // Helpers
    static uint8_t baud_rate_to_index(CANBaudRate baud);
    static CANBaudRate index_to_baud_rate(uint8_t index);
};

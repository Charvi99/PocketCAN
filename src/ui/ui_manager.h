#pragma once

/**
 * UI Manager
 * Manages LVGL screens and navigation
 */

#include "lvgl.h"
#include "../services/state_manager.h"
#include "widgets/status_bar.h"
#include "../config/app_config.h"

enum class Screen {
    MAIN_DASHBOARD,
    SNIFFER,
    TRANSMIT,
    EMULATOR,
    FILTER,
    SETTINGS,
    SCOPE
};

class UIManager {
public:
    /**
     * Initialize UI system
     */
    static bool init();

    /**
     * Navigate to screen
     */
    static void navigate_to(Screen screen);

    /**
     * Get current screen
     */
    static Screen get_current_screen();

    /**
     * Update UI (call from main loop)
     */
    static void update();

    /**
     * Show notification message
     */
    static void show_notification(const char* message, uint32_t duration_ms = 2000);

private:
    static Screen current_screen;
    static lv_obj_t* active_screen_obj;
    static lv_obj_t* status_bar;
    static uint32_t splash_start_time;
    static bool splash_shown;

    /**
     * Create splash screen
     */
    static void create_splash_screen();

    /**
     * Create main dashboard screen
     */
    static void create_main_dashboard();

    /**
     * Create sniffer screen
     */
    static void create_sniffer_screen();

    /**
     * Create transmit screen
     */
    static void create_transmit_screen();

    /**
     * Create settings screen
     */
    static void create_settings_screen();

    /**
     * Create scope screen
     */
    static void create_scope_screen();

    static void home_button_cb(lv_event_t* e);
};

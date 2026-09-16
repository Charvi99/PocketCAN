#pragma once

/**
 * Settings Manager
 * Handles persistent storage of application settings
 */

#include <Preferences.h>
#include "../config/can_config.h"

// Theme modes
enum class ThemeMode {
    LIGHT = 0,
    DARK = 1
};

// Application settings structure
struct AppSettings {
    // CAN settings
    CANBaudRate can_baudrate;

    // Display settings
    uint8_t brightness;         // 0-255
    ThemeMode theme;            // Light or Dark mode

    // UI settings
    bool auto_scroll;           // Auto-scroll in chronological mode
    uint16_t table_update_ms;   // Table update interval in ms

    // Data logging
    bool auto_log_on_start;     // Automatically enable logging when starting sniffer

    // System
    bool show_splash;           // Show splash screen on boot
};

class SettingsManager {
public:
    /**
     * Initialize settings manager
     * Loads settings from persistent storage
     */
    static bool init();

    /**
     * Load settings from persistent storage
     * @return Loaded settings
     */
    static AppSettings load();

    /**
     * Save settings to persistent storage
     * @param settings Settings to save
     * @return true if successful
     */
    static bool save(const AppSettings& settings);

    /**
     * Get current settings
     * @return Current settings
     */
    static const AppSettings& get();

    /**
     * Apply settings to system
     * Updates display, CAN, etc. based on settings
     * @param settings Settings to apply
     */
    static void apply(const AppSettings& settings);

    /**
     * Reset to default settings
     * @return Default settings
     */
    static AppSettings get_defaults();

private:
    static Preferences prefs;
    static AppSettings current_settings;
    static bool initialized;

    static const char* PREFS_NAMESPACE;
};

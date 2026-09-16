#include "settings_manager.h"
#include "../hal/display_hal.h"
#include "../hal/can_hal.h"
#include "state_manager.h"
#include "../ui/themes/theme_manager.h"
#include <Arduino.h>

Preferences SettingsManager::prefs;
AppSettings SettingsManager::current_settings;
bool SettingsManager::initialized = false;
const char* SettingsManager::PREFS_NAMESPACE = "pocketcan";

bool SettingsManager::init() {
    if (initialized) {
        return true;
    }

    Serial.println("Initializing Settings Manager...");

    // Load settings from storage
    AppSettings loaded_settings = load();

    // Initialize theme manager with loaded theme
    ThemeManager::init(loaded_settings.theme);

    // Apply loaded settings (this will update current_settings internally)
    apply(loaded_settings);

    initialized = true;
    Serial.println("Settings Manager initialized");
    return true;
}

AppSettings SettingsManager::load() {
    AppSettings settings = get_defaults();

    // Open preferences in read-only mode
    if (!prefs.begin(PREFS_NAMESPACE, true)) {
        Serial.println("WARNING: Could not open preferences, using defaults");
        return settings;
    }

    // Load CAN settings
    settings.can_baudrate = static_cast<CANBaudRate>(
        prefs.getUInt("can_baud", static_cast<uint32_t>(DEFAULT_CAN_BAUD))
    );

    // Load display settings
    settings.brightness = prefs.getUChar("brightness", 255);
    settings.theme = static_cast<ThemeMode>(prefs.getUChar("theme", 1)); // Default Dark

    // Load UI settings
    settings.auto_scroll = prefs.getBool("auto_scroll", true);
    settings.table_update_ms = prefs.getUShort("tbl_update", 200);

    // Load logging settings
    settings.auto_log_on_start = prefs.getBool("auto_log", false);

    // Load system settings
    settings.show_splash = prefs.getBool("show_splash", true);

    prefs.end();

    Serial.println("Settings loaded from storage");
    Serial.printf("  CAN Baud: %lu\n", static_cast<uint32_t>(settings.can_baudrate));
    Serial.printf("  Brightness: %d\n", settings.brightness);
    Serial.printf("  Theme: %s\n", settings.theme == ThemeMode::DARK ? "Dark" : "Light");

    return settings;
}

bool SettingsManager::save(const AppSettings& settings) {
    // Open preferences in read-write mode
    if (!prefs.begin(PREFS_NAMESPACE, false)) {
        Serial.println("ERROR: Could not open preferences for writing");
        return false;
    }

    // Save CAN settings
    prefs.putUInt("can_baud", static_cast<uint32_t>(settings.can_baudrate));

    // Save display settings
    prefs.putUChar("brightness", settings.brightness);
    prefs.putUChar("theme", static_cast<uint8_t>(settings.theme));

    // Save UI settings
    prefs.putBool("auto_scroll", settings.auto_scroll);
    prefs.putUShort("tbl_update", settings.table_update_ms);

    // Save logging settings
    prefs.putBool("auto_log", settings.auto_log_on_start);

    // Save system settings
    prefs.putBool("show_splash", settings.show_splash);

    prefs.end();

    Serial.println("Settings saved to storage");
    return true;
}

const AppSettings& SettingsManager::get() {
    return current_settings;
}

void SettingsManager::apply(const AppSettings& settings) {
    Serial.println("Applying settings...");

    // Apply display brightness
    DisplayHAL::set_brightness(settings.brightness);
    Serial.printf("  Brightness: %d\n", settings.brightness);

    // Apply theme if changed
    if (settings.theme != current_settings.theme) {
        Serial.printf("  Theme changing: %s -> %s\n",
                     current_settings.theme == ThemeMode::DARK ? "Dark" : "Light",
                     settings.theme == ThemeMode::DARK ? "Dark" : "Light");
        ThemeManager::set_theme(settings.theme);
        Serial.println("  Theme applied - screen reload required for full effect");
    }

    // Apply CAN baud rate if changed
    if (settings.can_baudrate != current_settings.can_baudrate) {
        Serial.printf("  CAN Baud Rate changing: %lu -> %lu\n",
                     static_cast<uint32_t>(current_settings.can_baudrate),
                     static_cast<uint32_t>(settings.can_baudrate));

        if (CANHAL::set_baudrate(settings.can_baudrate)) {
            // Restart CAN bus
            if (CANHAL::start()) {
                Serial.println("  CAN bus reconfigured and started");
                StateManager::set_can_state(CANBusState::RUNNING);
            } else {
                Serial.println("  WARNING: Failed to restart CAN bus");
                StateManager::set_can_state(CANBusState::ERROR);
            }
        } else {
            Serial.println("  ERROR: Failed to change CAN baud rate");
        }
    }

    // Note: Auto-scroll and table update are used by screen_sniffer during update
    // Note: Auto-log is checked when sniffer starts
    // Note: Show splash is checked during boot

    // Update current settings
    current_settings = settings;

    Serial.println("Settings applied successfully");
}

AppSettings SettingsManager::get_defaults() {
    AppSettings defaults;

    // CAN defaults
    defaults.can_baudrate = DEFAULT_CAN_BAUD;

    // Display defaults
    defaults.brightness = 255;          // Full brightness
    defaults.theme = ThemeMode::DARK;   // Dark mode

    // UI defaults
    defaults.auto_scroll = true;
    defaults.table_update_ms = 200;

    // Logging defaults
    defaults.auto_log_on_start = false;

    // System defaults
    defaults.show_splash = true;

    return defaults;
}

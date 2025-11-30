#pragma once

/**
 * Application Configuration
 * UI settings, themes, and application behavior
 */

// UI Configuration
#define UI_ANIMATION_TIME_MS    200
#define UI_SCROLL_SENSITIVITY   5
#define UI_DEBOUNCE_MS         50

// Theme
#define USE_DARK_THEME         true
#define STATUS_BAR_HEIGHT      40
#define BOTTOM_BAR_HEIGHT      60

// Data Logging
#define AUTO_SAVE_INTERVAL_MS  60000    // Auto-save every 60 seconds
#define MAX_LOG_FILE_SIZE_MB   100      // Maximum log file size

// Oscilloscope
#define SCOPE_SAMPLE_RATE_KHZ  1000     // 1 MHz sampling
#define SCOPE_BUFFER_DEPTH     10000    // Number of samples to store
#define SCOPE_TRIGGER_MODES    3        // Rising, Falling, Both

// Application Version
#define APP_VERSION_MAJOR      1
#define APP_VERSION_MINOR      0
#define APP_VERSION_PATCH      0
#define APP_NAME               "PocketCAN"

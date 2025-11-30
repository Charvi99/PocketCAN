#pragma once

/**
 * Hardware Configuration for M5Stack Tab5
 * Pin definitions and hardware-specific settings
 */

// Display Configuration
#define DISPLAY_WIDTH           720
#define DISPLAY_HEIGHT          1280
#define DISPLAY_ROTATION        90      // Landscape mode

// LVGL Buffer Configuration
#define LVGL_BUFFER_SIZE        (DISPLAY_WIDTH * DISPLAY_HEIGHT)
#define LVGL_PARTIAL_BUF_SIZE   (DISPLAY_WIDTH * 10)

// CAN Bus Configuration
#define CAN_TX_PIN              GPIO_NUM_17
#define CAN_RX_PIN              GPIO_NUM_18
#define CAN_OSCILLOSCOPE_PIN    GPIO_NUM_19  // For voltage monitoring

// Storage
#define SD_CARD_CS_PIN          -1  // TBD based on Tab5 schematic
#define USE_INTERNAL_FLASH      true

// System
#define SYSTEM_TICK_MS          1
#define WATCHDOG_TIMEOUT_MS     5000

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

// CAN Bus Configuration (Port A on M5Stack Tab5)
// Confirmed working with M5Stack CAN Unit (SN65HVD230 transceiver)
#define CAN_TX_PIN              GPIO_NUM_53  // ESP32 TX → CAN Unit CTX
#define CAN_RX_PIN              GPIO_NUM_54  // ESP32 RX → CAN Unit CRX
// #define CAN_OSCILLOSCOPE_PIN    GPIO_NUM_19  // Disabled - GPIO 19 used for SPI MISO

// Scope SPI (Nucleo-G431RB Communication)
#define SCOPE_SPI_SCK           GPIO_NUM_5   // SPI Clock
#define SCOPE_SPI_MOSI          GPIO_NUM_18  // Master Out Slave In (ESP32 → Nucleo)
#define SCOPE_SPI_MISO          GPIO_NUM_19  // Master In Slave Out (ESP32 ← Nucleo)
#define SCOPE_SPI_CS            GPIO_NUM_45  // Chip Select

// Storage
#define SD_CARD_CS_PIN          -1  // TBD based on Tab5 schematic
#define USE_INTERNAL_FLASH      true

// System
#define SYSTEM_TICK_MS          1
#define WATCHDOG_TIMEOUT_MS     5000

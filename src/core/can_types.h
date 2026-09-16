#pragma once

/**
 * Platform-free CAN types.
 * Deliberately includes nothing from ESP-IDF, Arduino, or LVGL so that
 * core logic compiles and tests on a host.
 */

#include <cstdint>

#include "../config/can_config.h"

// A single CAN 2.0A/B frame.
struct CANMessage {
    uint32_t id;                    // CAN identifier (11-bit or 29-bit)
    uint8_t data[8];                // Data bytes; bytes at or past dlc are zero
    uint8_t dlc;                    // Data length code (0-8)
    CANFrameType type;              // Standard or Extended
    uint32_t timestamp_ms;          // Capture time
    bool rtr;                       // Remote transmission request
};

// Rolling bus statistics.
struct CANStats {
    uint32_t rx_count;
    uint32_t tx_count;
    uint32_t error_count;           // tx_error_counter + rx_error_counter
    uint32_t bus_off_count;         // bus-off events, not error frames
    float bus_load_percent;         // estimated, see CanStatsCollector
};

// Lifecycle of the bus as the application sees it.
enum class CANBusState {
    STOPPED,
    RUNNING,
    BUS_OFF,        // controller has gone bus-off, recovery not yet attempted
    RECOVERING,     // recovery initiated, waiting for the controller to settle
    FAILED          // recovery gave up; needs user intervention
};

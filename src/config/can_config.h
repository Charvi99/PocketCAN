#pragma once

/**
 * CAN Bus Configuration
 * Default settings and supported modes
 */

#include <cstdint>

// Supported CAN Baud Rates
enum class CANBaudRate : uint32_t {
    BAUD_10K    = 10000,
    BAUD_20K    = 20000,
    BAUD_50K    = 50000,
    BAUD_100K   = 100000,
    BAUD_125K   = 125000,
    BAUD_250K   = 250000,
    BAUD_500K   = 500000,
    BAUD_800K   = 800000,
    BAUD_1M     = 1000000
};

// CAN Frame Type
enum class CANFrameType : uint8_t {
    STANDARD = 0,   // 11-bit ID
    EXTENDED = 1    // 29-bit ID
};

// Default Settings
#define DEFAULT_CAN_BAUD        CANBaudRate::BAUD_500K
#define DEFAULT_FRAME_TYPE      CANFrameType::STANDARD

// Buffer Sizes
#define CAN_RX_BUFFER_SIZE      1000    // Number of CAN messages to buffer
#define CAN_TX_QUEUE_SIZE       50      // Transmit queue size

// Filter Configuration
#define MAX_FILTERS             8       // Maximum number of simultaneous filters
#define MAX_MASK_FILTERS        4       // Maximum number of mask filters

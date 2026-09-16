#pragma once

/**
 * CAN Bus Hardware Abstraction Layer
 * ESP32 TWAI (CAN) controller interface
 */

#include <cstdint>
#include "driver/twai.h"
#include "../config/can_config.h"
#include "../core/can_types.h"

class CANHAL {
public:
    /**
     * Initialize CAN controller
     * @param baudrate CAN baud rate
     * @return true if successful
     */
    static bool init(CANBaudRate baudrate = DEFAULT_CAN_BAUD);

    /**
     * Start CAN bus
     * @return true if successful
     */
    static bool start();

    /**
     * Stop CAN bus
     * @return true if successful
     */
    static bool stop();

    /**
     * Set CAN baud rate
     * @param baudrate New baud rate
     * @return true if successful
     */
    static bool set_baudrate(CANBaudRate baudrate);

    /**
     * Transmit a CAN message
     * @param msg Message to transmit
     * @return true if successful
     */
    static bool transmit(const CANMessage& msg);

    /**
     * Receive a CAN message (non-blocking)
     * @param msg Output message
     * @return true if message received
     */
    static bool receive(CANMessage& msg);

    /**
     * Check if messages are available
     * @return number of messages in RX queue
     */
    static uint32_t available();

    /**
     * Get CAN statistics
     * @return Current statistics
     */
    static CANStats get_stats();

    /**
     * Reset statistics
     */
    static void reset_stats();

    /**
     * Check if bus is in error state
     * @return true if bus-off or error passive
     */
    static bool is_bus_error();

private:
    static CANBaudRate current_baudrate;
    static CANStats stats;
    static bool initialized;
    static bool running;

    /**
     * Convert CANBaudRate to TWAI timing config
     */
    static twai_timing_config_t get_timing_config(CANBaudRate baudrate);
};

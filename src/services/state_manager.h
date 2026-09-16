#pragma once

/**
 * State Manager
 * Centralized application state management
 */

#include "../config/can_config.h"
#include "../core/can_types.h"

enum class AppMode {
    SNIFFER,
    TRANSMITTER,
    EMULATOR,
    OSCILLOSCOPE,
    SETTINGS
};

struct ApplicationState {
    AppMode current_mode;
    CANBusState can_state;
    CANBaudRate can_baudrate;
    bool filtering_enabled;
    bool logging_enabled;
    uint32_t message_count;
    float bus_load;
    uint32_t error_count;
};

class StateManager {
public:
    /**
     * Initialize state manager
     */
    static void init();

    /**
     * Get current application state
     */
    static const ApplicationState& get_state();

    /**
     * Set application mode
     */
    static void set_mode(AppMode mode);

    /**
     * Set CAN bus state
     */
    static void set_can_state(CANBusState state);

    /**
     * Set CAN baud rate
     */
    static void set_can_baudrate(CANBaudRate baudrate);

    /**
     * Enable/disable filtering
     */
    static void set_filtering_enabled(bool enabled);

    /**
     * Enable/disable logging
     */
    static void set_logging_enabled(bool enabled);

    /**
     * Update message count
     */
    static void set_message_count(uint32_t count);

    /**
     * Update bus load
     */
    static void set_bus_load(float load);

    /**
     * Update error count
     */
    static void set_error_count(uint32_t count);

private:
    static ApplicationState state;
};

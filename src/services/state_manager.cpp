#include "state_manager.h"

ApplicationState StateManager::state = {
    .current_mode = AppMode::SNIFFER,
    .can_state = CANBusState::STOPPED,
    .can_baudrate = DEFAULT_CAN_BAUD,
    .filtering_enabled = false,
    .logging_enabled = false,
    .message_count = 0,
    .bus_load = 0.0f,
    .error_count = 0
};

void StateManager::init() {
    // Initialize with defaults
    state.current_mode = AppMode::SNIFFER;
    state.can_state = CANBusState::STOPPED;
    state.can_baudrate = DEFAULT_CAN_BAUD;
    state.filtering_enabled = false;
    state.logging_enabled = false;
    state.message_count = 0;
    state.bus_load = 0.0f;
    state.error_count = 0;
}

const ApplicationState& StateManager::get_state() {
    return state;
}

void StateManager::set_mode(AppMode mode) {
    state.current_mode = mode;
}

void StateManager::set_can_state(CANBusState new_state) {
    state.can_state = new_state;
}

void StateManager::set_can_baudrate(CANBaudRate baudrate) {
    state.can_baudrate = baudrate;
}

void StateManager::set_filtering_enabled(bool enabled) {
    state.filtering_enabled = enabled;
}

void StateManager::set_logging_enabled(bool enabled) {
    state.logging_enabled = enabled;
}

void StateManager::set_message_count(uint32_t count) {
    state.message_count = count;
}

void StateManager::set_bus_load(float load) {
    state.bus_load = load;
}

void StateManager::set_error_count(uint32_t count) {
    state.error_count = count;
}

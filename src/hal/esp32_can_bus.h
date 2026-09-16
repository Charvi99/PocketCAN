#pragma once

/**
 * ESP32-P4 TWAI implementation of ICANBus.
 * This is what the firmware always runs. Lifecycle lives here rather than on
 * the interface so that FakeCanBus stays trivial.
 */

#include "driver/twai.h"

#include "../core/can_timing.h"
#include "../core/i_can_bus.h"
#include "../config/can_config.h"

class Esp32CanBus : public ICANBus {
public:
    bool init(CANBaudRate baudrate = DEFAULT_CAN_BAUD);
    bool start();
    bool stop();
    bool set_baudrate(CANBaudRate baudrate);

    bool transmit(const CANMessage& msg) override;
    bool receive(CANMessage& msg) override;
    CANStats get_stats() const override;
    bool is_bus_error() const override;
    bool recover() override;

    uint32_t available() const;
    void reset_stats();
    CANBaudRate get_baudrate() const { return current_baudrate; }
    bool is_running() const { return running; }

private:
    static twai_timing_config_t timing_config_for(CanTimingId id);

    CANBaudRate current_baudrate = DEFAULT_CAN_BAUD;
    mutable CANStats stats = {};
    bool initialized = false;
    bool running = false;
};

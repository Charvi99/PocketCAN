#pragma once

/**
 * CAN Transmitter
 * Handles single-shot and periodic CAN message transmission
 */

#include "can_types.h"
#include "i_can_bus.h"

struct PeriodicMessage {
    CANMessage message;
    uint32_t interval_ms;
    uint32_t last_sent;
    bool active;
};

class CANTransmitter {
public:
    explicit CANTransmitter(ICANBus& bus) : bus(bus) {}

    /**
     * Initialize transmitter.
     * @param now current millisecond tick, used to seed periodic schedules
     */
    void init(uint32_t now);

    /**
     * Send a single CAN message
     * @param msg Message to send
     * @return true if successful
     */
    bool send(const CANMessage& msg);

    /**
     * Add periodic message
     * @param msg Message to send periodically
     * @param interval_ms Interval in milliseconds
     * @return Slot index, or -1 if failed
     */
    int add_periodic(const CANMessage& msg, uint32_t interval_ms);

    /**
     * Remove periodic message
     */
    bool remove_periodic(int index);

    /**
     * Enable/disable periodic message
     */
    bool set_periodic_enabled(int index, bool enabled);

    /**
     * Update periodic message interval
     */
    bool set_periodic_interval(int index, uint32_t interval_ms);

    /**
     * Clear all periodic messages
     */
    void clear_periodic();

    /**
     * Send any periodic messages that have come due.
     * @param now current millisecond tick, supplied by CanService
     */
    void update(uint32_t now);

    /**
     * Get number of periodic messages
     */
    int get_periodic_count() const { return periodic_count; }

    /**
     * Get periodic message by index
     */
    const PeriodicMessage* get_periodic(int index) const;

private:
    static constexpr int MAX_PERIODIC = 10;
    ICANBus& bus;
    PeriodicMessage periodic_messages[MAX_PERIODIC];
    int periodic_count = 0;
    uint32_t now_ = 0;      // last tick seen; lets add_periodic seed last_sent
};

#pragma once

/**
 * CAN service - the composition root for the CAN subsystem.
 *
 * Owns the bus and the four core modules, and exposes one update() to the
 * main loop. This is the object whose absence made every core module in this
 * project unreachable: they were all constructed, none were ever wired.
 *
 * Time enters the system here and nowhere else. update() takes the current
 * tick and passes it down, so wrap-around behaviour is unit-testable.
 */

#include "../config/can_config.h"
#include "../core/can_filter.h"
#include "../core/can_sniffer.h"
#include "../core/can_transmitter.h"
#include "../core/can_types.h"
#include "../core/device_emulator.h"
#include "../core/i_can_bus.h"
#include "can_stats.h"

class CanService {
public:
    explicit CanService(ICANBus& bus);

    void init(uint32_t now);
    bool start(uint32_t now);
    void stop();

    /**
     * One pass of the receive pipeline plus the scheduled transmissions.
     * Bounded by MAX_FRAMES_PER_TICK so a saturated bus cannot starve LVGL.
     * @param now current millisecond tick - the single time source
     */
    void update(uint32_t now);

    bool send(const CANMessage& msg);

    void set_bitrate(uint32_t bits_per_second);

    CANSniffer&     sniffer()     { return sniffer_; }
    CANFilter&      filter()      { return filter_; }
    CANTransmitter& transmitter() { return transmitter_; }
    DeviceEmulator& emulator()    { return emulator_; }

    const CanStatsCollector& stats() const { return stats_; }
    const RingBuffer<CANMessage>& history() const { return sniffer_.get_buffer(); }

    CANBusState bus_state() const { return state; }

private:
    void service_bus_health(uint32_t now);

    static constexpr uint8_t  MAX_RECOVERY_ATTEMPTS = 5;
    static constexpr uint32_t RECOVERY_SETTLE_MS    = 100;

    ICANBus&          bus;
    CANSniffer        sniffer_;
    CANFilter         filter_;
    CANTransmitter    transmitter_;
    DeviceEmulator    emulator_;
    CanStatsCollector stats_;

    CANBusState state = CANBusState::STOPPED;
    uint8_t  recovery_attempts = 0;
    uint32_t recovery_started  = 0;
};

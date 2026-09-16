#pragma once

/**
 * Bus statistics collector.
 * Bus load is an estimate: it counts frame bits, including a flat 10%
 * allowance for bit stuffing, over a rolling one-second window. Error frames
 * are not counted. The UI labels the figure as estimated.
 */

#include <cstdint>

#include "../core/can_types.h"

class CanStatsCollector {
public:
    void set_bitrate(uint32_t bits_per_second);

    void record_rx(const CANMessage& msg);
    void record_tx(const CANMessage& msg);

    /**
     * Close the current window if a second has elapsed.
     * @param now current millisecond tick, supplied by CanService
     */
    void update(uint32_t now);

    void reset(uint32_t now);

    float    bus_load_percent() const { return load_percent; }
    uint32_t rx_count() const         { return rx; }
    uint32_t tx_count() const         { return tx; }

    /**
     * Bits a frame occupies on the wire, stuffing allowance included.
     * Standard: 47 bits of framing; extended: 67. Public for testability.
     */
    static uint32_t frame_bits(const CANMessage& msg);

private:
    uint32_t bitrate_bps  = 500000;
    uint32_t window_bits  = 0;
    uint32_t window_start = 0;
    float    load_percent = 0.0f;
    uint32_t rx = 0;
    uint32_t tx = 0;
};

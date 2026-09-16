#pragma once

/**
 * CAN Sniffer
 * Captures and buffers CAN messages from the bus
 */

#include "can_types.h"
#include "../utils/ring_buffer.h"
#include <functional>

using CANMessageCallback = std::function<void(const CANMessage&)>;

class CANSniffer {
public:
    /**
     * Initialize sniffer.
     * @param now current millisecond tick, seeds the rate window
     */
    void init(uint32_t now);

    /**
     * Start sniffing
     */
    void start();

    /**
     * Stop sniffing
     */
    void stop();

    /**
     * Check if sniffing is active
     */
    bool is_running() const { return running; }

    /**
     * Record one captured frame. Called by CanService for every frame that
     * survives the filter.
     */
    void record(const CANMessage& msg);

    /**
     * Recompute the messages-per-second figure. Cheap; call every tick.
     * @param now current millisecond tick, supplied by CanService
     */
    void update_stats(uint32_t now);

    /**
     * Get message buffer
     */
    const RingBuffer<CANMessage>& get_buffer() const { return message_buffer; }

    /**
     * Register callback for new messages
     */
    void on_message_received(CANMessageCallback callback);

    /**
     * Clear captured history and reset counters.
     * @param now current millisecond tick, re-seeds the rate window
     */
    void clear(uint32_t now);

    /**
     * Get message count
     */
    uint32_t get_message_count() const;

    /**
     * Get messages per second
     */
    float get_messages_per_second() const;

private:
    bool running = false;
    RingBuffer<CANMessage> message_buffer{CAN_RX_BUFFER_SIZE};
    CANMessageCallback message_callback = nullptr;

    // Statistics
    uint32_t message_count = 0;
    uint32_t last_count = 0;
    uint32_t last_stat_time = 0;
    float messages_per_sec = 0.0f;
};

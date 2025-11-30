#pragma once

/**
 * CAN Sniffer
 * Captures and buffers CAN messages from the bus
 */

#include "../hal/can_hal.h"
#include "../utils/ring_buffer.h"
#include <functional>

using CANMessageCallback = std::function<void(const CANMessage&)>;

class CANSniffer {
public:
    /**
     * Initialize sniffer
     */
    void init();

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
     * Update - call from main loop
     * Processes incoming CAN messages
     */
    void update();

    /**
     * Get message buffer
     */
    const RingBuffer<CANMessage>& get_buffer() const { return message_buffer; }

    /**
     * Register callback for new messages
     */
    void on_message_received(CANMessageCallback callback);

    /**
     * Clear message buffer
     */
    void clear();

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

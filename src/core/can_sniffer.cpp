#include "can_sniffer.h"

void CANSniffer::init(uint32_t now) {
    message_buffer.clear();
    message_count  = 0;
    last_count     = 0;
    last_stat_time = now;
    running        = false;
}

void CANSniffer::start() {
    running = true;
}

void CANSniffer::stop() {
    running = false;
}

void CANSniffer::record(const CANMessage& msg) {
    if (!running) {
        return;
    }

    message_buffer.push(msg);
    message_count++;

    if (message_callback) {
        message_callback(msg);
    }
}

void CANSniffer::update_stats(uint32_t now) {
    uint32_t elapsed = now - last_stat_time;   // wrap-safe subtraction
    if (elapsed >= 1000) {
        messages_per_sec = (message_count - last_count) * 1000.0f / elapsed;
        last_count       = message_count;
        last_stat_time   = now;
    }
}

void CANSniffer::on_message_received(CANMessageCallback callback) {
    message_callback = callback;
}

void CANSniffer::clear(uint32_t now) {
    message_buffer.clear();
    message_count    = 0;
    last_count       = 0;
    messages_per_sec = 0.0f;
    last_stat_time   = now;
}

uint32_t CANSniffer::get_message_count() const {
    return message_count;
}

float CANSniffer::get_messages_per_second() const {
    return messages_per_sec;
}

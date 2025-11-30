#include "can_sniffer.h"
#include <Arduino.h>

void CANSniffer::init() {
    message_buffer.clear();
    message_count = 0;
    last_count = 0;
    last_stat_time = millis();
    running = false;
}

void CANSniffer::start() {
    if (!running) {
        clear();
        running = true;
        Serial.println("CAN Sniffer started");
    }
}

void CANSniffer::stop() {
    if (running) {
        running = false;
        Serial.println("CAN Sniffer stopped");
    }
}

void CANSniffer::update() {
    if (!running) {
        return;
    }

    // Process all available messages
    CANMessage msg;
    while (CANHAL::receive(msg)) {
        // Add to buffer
        message_buffer.push(msg);
        message_count++;

        // Trigger callback if registered
        if (message_callback) {
            message_callback(msg);
        }
    }

    // Update statistics every second
    uint32_t now = millis();
    if (now - last_stat_time >= 1000) {
        messages_per_sec = (message_count - last_count) * 1000.0f / (now - last_stat_time);
        last_count = message_count;
        last_stat_time = now;
    }
}

void CANSniffer::on_message_received(CANMessageCallback callback) {
    message_callback = callback;
}

void CANSniffer::clear() {
    message_buffer.clear();
    message_count = 0;
    last_count = 0;
    messages_per_sec = 0.0f;
    last_stat_time = millis();
}

uint32_t CANSniffer::get_message_count() const {
    return message_count;
}

float CANSniffer::get_messages_per_second() const {
    return messages_per_sec;
}

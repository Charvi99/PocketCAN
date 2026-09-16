#include "can_transmitter.h"
#include <cstring>

void CANTransmitter::init(uint32_t now) {
    now_ = now;
    clear_periodic();
}

bool CANTransmitter::send(const CANMessage& msg) {
    return bus.transmit(msg);
}

int CANTransmitter::add_periodic(const CANMessage& msg, uint32_t interval_ms) {
    if (periodic_count >= MAX_PERIODIC) {
        return -1;
    }

    periodic_messages[periodic_count].message     = msg;
    periodic_messages[periodic_count].interval_ms = interval_ms;
    periodic_messages[periodic_count].last_sent   = now_;
    periodic_messages[periodic_count].active      = true;

    return periodic_count++;
}

bool CANTransmitter::remove_periodic(int index) {
    if (index < 0 || index >= periodic_count) {
        return false;
    }

    // Shift messages down
    for (int i = index; i < periodic_count - 1; i++) {
        periodic_messages[i] = periodic_messages[i + 1];
    }
    periodic_count--;
    return true;
}

bool CANTransmitter::set_periodic_enabled(int index, bool enabled) {
    if (index < 0 || index >= periodic_count) {
        return false;
    }
    periodic_messages[index].active = enabled;
    if (enabled) {
        periodic_messages[index].last_sent = now_;
    }
    return true;
}

bool CANTransmitter::set_periodic_interval(int index, uint32_t interval_ms) {
    if (index < 0 || index >= periodic_count) {
        return false;
    }
    periodic_messages[index].interval_ms = interval_ms;
    return true;
}

void CANTransmitter::clear_periodic() {
    periodic_count = 0;
    memset(periodic_messages, 0, sizeof(periodic_messages));
}

void CANTransmitter::update(uint32_t now) {
    now_ = now;

    for (int i = 0; i < periodic_count; i++) {
        if (!periodic_messages[i].active) {
            continue;
        }

        // Subtraction, not addition: correct across the 49.7-day millis() wrap.
        if (now - periodic_messages[i].last_sent >= periodic_messages[i].interval_ms) {
            if (bus.transmit(periodic_messages[i].message)) {
                periodic_messages[i].last_sent = now;
            }
        }
    }
}

const PeriodicMessage* CANTransmitter::get_periodic(int index) const {
    if (index < 0 || index >= periodic_count) {
        return nullptr;
    }
    return &periodic_messages[index];
}

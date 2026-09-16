#include "device_emulator.h"
#include <cstring>

void DeviceEmulator::init() {
    clear_rules();
    running = false;
    pending_count = 0;
}

void DeviceEmulator::start() {
    if (!running) {
        running = true;
        pending_count = 0;
    }
}

void DeviceEmulator::stop() {
    if (running) {
        running = false;
        pending_count = 0;
    }
}

int DeviceEmulator::add_rule(const EmulationRule& rule) {
    if (rule_count >= MAX_RULES) {
        return -1;
    }

    rules[rule_count] = rule;
    return rule_count++;
}

bool DeviceEmulator::remove_rule(int index) {
    if (index < 0 || index >= rule_count) {
        return false;
    }

    for (int i = index; i < rule_count - 1; i++) {
        rules[i] = rules[i + 1];
    }
    rule_count--;
    return true;
}

bool DeviceEmulator::set_rule_enabled(int index, bool enabled) {
    if (index < 0 || index >= rule_count) {
        return false;
    }
    rules[index].enabled = enabled;
    return true;
}

const EmulationRule* DeviceEmulator::get_rule(int index) const {
    if (index < 0 || index >= rule_count) {
        return nullptr;
    }
    return &rules[index];
}

void DeviceEmulator::clear_rules() {
    rule_count = 0;
    memset(rules, 0, sizeof(rules));
}

void DeviceEmulator::process_message(const CANMessage& msg, uint32_t now) {
    if (!running) {
        return;
    }

    for (int i = 0; i < rule_count; i++) {
        if (!rules[i].enabled) {
            continue;
        }
        if (msg.id == rules[i].trigger_id) {
            add_pending_response(rules[i].response, rules[i].delay_ms, now);
        }
    }
}

void DeviceEmulator::add_pending_response(const CANMessage& msg, uint32_t delay_ms,
                                          uint32_t now) {
    if (pending_count >= MAX_RULES) {
        return;   // queue full; drop rather than overwrite a pending response
    }

    pending_responses[pending_count].message   = msg;
    pending_responses[pending_count].send_time = now + delay_ms;   // may wrap
    pending_responses[pending_count].active    = true;
    pending_count++;
}

void DeviceEmulator::update(uint32_t now) {
    if (!running) {
        return;
    }

    for (int i = 0; i < pending_count; i++) {
        if (!pending_responses[i].active) {
            continue;
        }

        // Signed difference, not 'now >= send_time'. send_time is allowed to
        // wrap past zero; the signed delta stays correct across the wrap as
        // long as the delay is under ~24 days, which every real delay is.
        if (static_cast<int32_t>(now - pending_responses[i].send_time) >= 0) {
            if (bus.transmit(pending_responses[i].message)) {
                pending_responses[i].active = false;
            }
        }
    }

    // Compact the queue, dropping sent responses.
    int write_idx = 0;
    for (int read_idx = 0; read_idx < pending_count; read_idx++) {
        if (pending_responses[read_idx].active) {
            if (write_idx != read_idx) {
                pending_responses[write_idx] = pending_responses[read_idx];
            }
            write_idx++;
        }
    }
    pending_count = write_idx;
}

bool DeviceEmulator::load_profile(const char* filename) {
    (void)filename;
    return false;   // deferred to sub-project C
}

bool DeviceEmulator::save_profile(const char* filename) {
    (void)filename;
    return false;   // deferred to sub-project C
}

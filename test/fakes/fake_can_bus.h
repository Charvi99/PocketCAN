#pragma once

/**
 * Scripted ICANBus for host tests.
 * rx_queue is what the bus will hand back; sent records what the code
 * under test emitted.
 */

#include <deque>
#include <vector>

#include "core/i_can_bus.h"

class FakeCanBus : public ICANBus {
public:
    std::deque<CANMessage>  rx_queue;
    std::vector<CANMessage> sent;
    CANStats stats{};
    bool bus_error      = false;
    bool recover_called = false;
    bool transmit_ok    = true;

    bool transmit(const CANMessage& msg) override {
        if (!transmit_ok) {
            return false;
        }
        sent.push_back(msg);
        stats.tx_count++;
        return true;
    }

    bool receive(CANMessage& msg) override {
        if (rx_queue.empty()) {
            return false;
        }
        msg = rx_queue.front();
        rx_queue.pop_front();
        stats.rx_count++;
        return true;
    }

    CANStats get_stats() const override { return stats; }
    bool is_bus_error() const override  { return bus_error; }

    bool recover() override {
        recover_called = true;
        return true;
    }

    // Convenience for tests.
    void queue(uint32_t id, uint8_t dlc = 8) {
        CANMessage m = {};
        m.id   = id;
        m.dlc  = dlc;
        m.type = CANFrameType::STANDARD;
        rx_queue.push_back(m);
    }
};

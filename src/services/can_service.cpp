#include "can_service.h"

CanService::CanService(ICANBus& bus)
    : bus(bus), transmitter_(bus), emulator_(bus) {}

void CanService::init(uint32_t now) {
    sniffer_.init(now);
    filter_.init();
    transmitter_.init(now);
    emulator_.init();
    stats_.reset(now);
    state = CANBusState::STOPPED;
    recovery_attempts = 0;
}

bool CanService::start(uint32_t now) {
    sniffer_.start();
    stats_.reset(now);
    state = CANBusState::RUNNING;
    recovery_attempts = 0;
    return true;
}

void CanService::stop() {
    sniffer_.stop();
    emulator_.stop();
    state = CANBusState::STOPPED;
}

void CanService::set_bitrate(uint32_t bits_per_second) {
    stats_.set_bitrate(bits_per_second);
}

bool CanService::send(const CANMessage& msg) {
    if (!bus.transmit(msg)) {
        return false;
    }
    stats_.record_tx(msg);
    return true;
}

void CanService::update(uint32_t now) {
    service_bus_health(now);
    if (state == CANBusState::BUS_OFF || state == CANBusState::RECOVERING ||
        state == CANBusState::FAILED) {
        return;
    }

    // Bounded receive loop. The bus is drained even while capture is paused,
    // so the hardware RX queue does not overflow behind a paused UI.
    CANMessage msg;
    int budget = MAX_FRAMES_PER_TICK;
    while (budget-- > 0 && bus.receive(msg)) {
        stats_.record_rx(msg);

        if (!filter_.check_message(msg)) {
            continue;
        }

        sniffer_.record(msg);
        emulator_.process_message(msg, now);
    }

    transmitter_.update(now);
    emulator_.update(now);
    sniffer_.update_stats(now);
    stats_.update(now);
}

void CanService::service_bus_health(uint32_t now) {
    (void)now;   // Task 12 implements the recovery state machine here
}

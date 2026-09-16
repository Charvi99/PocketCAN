#include "can_stats.h"

uint32_t CanStatsCollector::frame_bits(const CANMessage& msg) {
    const uint32_t overhead = (msg.type == CANFrameType::EXTENDED) ? 67u : 47u;
    const uint32_t raw      = overhead + 8u * msg.dlc;
    return (raw * 11u) / 10u;   // flat 10% bit-stuffing allowance
}

void CanStatsCollector::set_bitrate(uint32_t bits_per_second) {
    bitrate_bps = bits_per_second ? bits_per_second : 500000;
}

void CanStatsCollector::record_rx(const CANMessage& msg) {
    rx++;
    window_bits += frame_bits(msg);
}

void CanStatsCollector::record_tx(const CANMessage& msg) {
    tx++;
    window_bits += frame_bits(msg);
}

void CanStatsCollector::update(uint32_t now) {
    const uint32_t elapsed = now - window_start;   // wrap-safe subtraction
    if (elapsed < 1000) {
        return;
    }

    const float capacity = (static_cast<float>(bitrate_bps) * elapsed) / 1000.0f;
    load_percent = capacity > 0.0f ? (window_bits * 100.0f) / capacity : 0.0f;
    if (load_percent > 100.0f) {
        load_percent = 100.0f;
    }

    window_bits  = 0;
    window_start = now;
}

void CanStatsCollector::reset(uint32_t now) {
    window_bits  = 0;
    window_start = now;
    load_percent = 0.0f;
    rx = 0;
    tx = 0;
}

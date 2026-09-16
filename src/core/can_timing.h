#pragma once

/**
 * CAN bit timing selection.
 *
 * This header is deliberately free of ESP-IDF types so the mapping can be
 * unit-tested on a host. It names a timing preset; esp32_can_bus.cpp turns
 * the name into the ESP-IDF TWAI_TIMING_CONFIG_* register values. Keeping
 * the two apart is what makes a wrong mapping a test failure rather than a
 * bus that silently never syncs.
 */

#include <cstdint>

#include "can_types.h"

enum class CanTimingId : uint8_t {
    T_10K, T_20K, T_50K, T_100K, T_125K, T_250K, T_500K, T_800K, T_1M
};

struct CanTimingSpec {
    CanTimingId id;
    uint32_t    nominal_bps;
};

constexpr CanTimingSpec can_timing_for(CANBaudRate rate) {
    switch (rate) {
        case CANBaudRate::BAUD_10K:  return {CanTimingId::T_10K,     10000};
        case CANBaudRate::BAUD_20K:  return {CanTimingId::T_20K,     20000};
        case CANBaudRate::BAUD_50K:  return {CanTimingId::T_50K,     50000};
        case CANBaudRate::BAUD_100K: return {CanTimingId::T_100K,   100000};
        case CANBaudRate::BAUD_125K: return {CanTimingId::T_125K,   125000};
        case CANBaudRate::BAUD_250K: return {CanTimingId::T_250K,   250000};
        case CANBaudRate::BAUD_500K: return {CanTimingId::T_500K,   500000};
        case CANBaudRate::BAUD_800K: return {CanTimingId::T_800K,   800000};
        case CANBaudRate::BAUD_1M:   return {CanTimingId::T_1M,    1000000};
    }
    return {CanTimingId::T_500K, 500000};   // unreachable; enum is exhaustive
}

#pragma once

/**
 * The CAN bus seam.
 *
 * COMPILE-TIME ABSTRACTION ONLY. The shipped firmware always binds this to
 * Esp32CanBus and talks straight to the TWAI peripheral - the device is
 * standalone and never depends on a host at runtime. The cost is one vtable
 * lookup per frame. The only other implementation is FakeCanBus, which lives
 * in test/ and is never flashed.
 *
 * Bus I/O only. Lifecycle (init/start/stop/set_baudrate) stays on the
 * concrete class so the fake stays trivial.
 */

#include "can_types.h"

class ICANBus {
public:
    virtual ~ICANBus() = default;

    // Send one frame. Returns false if the bus is not running or the TX
    // queue did not accept it within the driver timeout.
    virtual bool transmit(const CANMessage& msg) = 0;

    // Non-blocking receive of one frame. Returns false when nothing is queued.
    virtual bool receive(CANMessage& msg) = 0;

    virtual CANStats get_stats() const = 0;

    // True when the controller is bus-off or already recovering.
    virtual bool is_bus_error() const = 0;

    // Ask the controller to begin bus-off recovery. Returns false if it
    // could not be initiated. Recovery policy lives in CanService.
    virtual bool recover() = 0;
};

#include <Arduino.h>

#include "config/app_config.h"
#include "config/can_config.h"
#include "config/hardware_config.h"

#include "hal/display_hal.h"
#include "hal/esp32_can_bus.h"
#include "hal/storage_hal.h"
#include "hal/touch_hal.h"

#include "services/can_service.h"
#include "services/state_manager.h"

#include "ui/ui_manager.h"

// Composition root. These two are the whole object graph: the bus is the
// hardware, the service owns everything that uses it.
static Esp32CanBus can_bus;
static CanService  can_service(can_bus);

// Accessor for the UI layer (sub-project B wires screens to this).
CanService& app_can_service() {
    return can_service;
}

static void fatal(const char* subsystem) {
    Serial.printf("FATAL: %s failed to initialize\n", subsystem);
    DisplayHAL::show_fatal(subsystem);
    // Park, but keep the watchdog fed and the message on screen.
    for (;;) {
        delay(1000);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("========================================");
    Serial.printf("%s v%d.%d.%d\n", APP_NAME,
                  APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);
    Serial.println("========================================");

    // Display first, so every later failure has somewhere to be reported.
    if (!DisplayHAL::init()) {
        Serial.println("FATAL: display failed to initialize");
        for (;;) {
            delay(1000);
        }
    }

    if (!TouchHAL::init()) {
        fatal("Touch");
    }

    StateManager::init();

    if (!UIManager::init()) {
        fatal("UI");
    }

    if (!StorageHAL::init()) {
        Serial.println("WARNING: storage unavailable - logging disabled");
        UIManager::show_notification("Storage unavailable", 3000);
    }

    const uint32_t now = millis();

    if (!can_bus.init(DEFAULT_CAN_BAUD)) {
        fatal("CAN controller");
    }

    can_service.init(now);
    can_service.set_bitrate(static_cast<uint32_t>(DEFAULT_CAN_BAUD));

    if (can_bus.start()) {
        can_service.start(now);          // <- the call this project never made
        StateManager::set_can_state(CANBusState::RUNNING);
        Serial.println("CAN bus started");
    } else {
        StateManager::set_can_state(CANBusState::FAILED);
        UIManager::show_notification("CAN bus failed to start", 5000);
        Serial.println("WARNING: CAN bus failed to start");
    }

    StateManager::set_can_baudrate(DEFAULT_CAN_BAUD);

    Serial.println("Initialization complete");
}

void loop() {
    const uint32_t now = millis();

    lv_timer_handler();

    can_service.update(now);

    StateManager::set_message_count(can_service.sniffer().get_message_count());
    StateManager::set_bus_load(can_service.stats().bus_load_percent());
    StateManager::set_error_count(can_bus.get_stats().error_count);
    StateManager::set_can_state(can_service.bus_state());

    UIManager::update();

    delay(SYSTEM_TICK_MS);
}

#include <Arduino.h>
#include "config/hardware_config.h"
#include "config/can_config.h"
#include "config/app_config.h"

// HAL Layer
#include "hal/display_hal.h"
#include "hal/touch_hal.h"
#include "hal/can_hal.h"
#include "hal/storage_hal.h"

// Core Layer
#include "core/can_sniffer.h"
#include "core/can_filter.h"
#include "core/can_transmitter.h"
#include "core/device_emulator.h"

// Services Layer
#include "services/state_manager.h"

// UI Layer
#include "ui/ui_manager.h"

// Global instances
CANSniffer sniffer;
CANFilter filter;
CANTransmitter transmitter;
DeviceEmulator emulator;

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    delay(1000);

    Serial.println("========================================");
    Serial.printf("%s v%d.%d.%d\n", APP_NAME,
                  APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);
    Serial.println("========================================");

    // Initialize HAL Layer
    Serial.println("Initializing Hardware...");

    if (!DisplayHAL::init()) {
        Serial.println("ERROR: Failed to initialize display");
        while(1);
    }

    if (!TouchHAL::init()) {
        Serial.println("ERROR: Failed to initialize touch");
        while(1);
    }

    if (!CANHAL::init(DEFAULT_CAN_BAUD)) {
        Serial.println("ERROR: Failed to initialize CAN");
        while(1);
    }

    if (!StorageHAL::init()) {
        Serial.println("WARNING: Storage initialization failed");
        // Continue anyway - not critical
    }

    // Initialize Services
    Serial.println("Initializing Services...");
    StateManager::init();

    // Initialize Core Modules
    Serial.println("Initializing Core Modules...");
    sniffer.init();
    filter.init();
    transmitter.init();
    emulator.init();

    // Initialize UI
    Serial.println("Initializing UI...");
    if (!UIManager::init()) {
        Serial.println("ERROR: Failed to initialize UI");
        while(1);
    }

    // Start CAN bus
    Serial.println("Starting CAN Bus...");
    if (CANHAL::start()) {
        StateManager::set_can_state(CANBusState::RUNNING);
        Serial.println("CAN Bus started successfully");
    } else {
        StateManager::set_can_state(CANBusState::ERROR);
        Serial.println("WARNING: Failed to start CAN bus");
    }

    Serial.println("========================================");
    Serial.println("Initialization Complete!");
    Serial.println("========================================");
}

void loop() {
    // Handle LVGL tasks
    lv_timer_handler();

    // Update core modules
    sniffer.update();
    transmitter.update();
    emulator.update();

    // Update state
    StateManager::set_message_count(sniffer.get_message_count());
    CANStats stats = CANHAL::get_stats();
    StateManager::set_error_count(stats.error_count);
    StateManager::set_bus_load(stats.bus_load_percent);

    // Check for CAN bus errors
    if (CANHAL::is_bus_error()) {
        StateManager::set_can_state(CANBusState::ERROR);
    }

    // Update UI
    UIManager::update();

    // Small delay to prevent watchdog issues
    delay(SYSTEM_TICK_MS);
}

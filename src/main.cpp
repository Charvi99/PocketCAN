#include <Arduino.h>
#include "config/hardware_config.h"
#include "config/can_config.h"
#include "config/app_config.h"
#include "driver/twai.h"

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
#include "services/settings_manager.h"
#include "services/scope_manager.h"

// UI Layer
#include "ui/ui_manager.h"

#include <M5Unified.h>

// Global instances
CANSniffer sniffer;
CANFilter filter;
CANTransmitter transmitter;
DeviceEmulator emulator;

// Test mode variables
bool test_mode_enabled = false;
uint32_t test_message_interval = 100;  // ms between test messages
uint32_t last_test_message_time = 0;
uint32_t test_message_count = 0;

/**
 * Generate a random CAN message for testing
 */
/**
 * Generate a random CAN message for testing from a predefined list of IDs
 */
CANMessage generate_random_can_message() {
    // Define a list of 5 standard and 3 extended CAN IDs
    struct PredefinedID {
        uint32_t id;
        CANFrameType type;
    };

    const static PredefinedID predefined_ids[] = {
        // 11-bit IDs (Standard)
        {0x100, CANFrameType::STANDARD},
        {0x2A5, CANFrameType::STANDARD},
        {0x333, CANFrameType::STANDARD},
        {0x4F0, CANFrameType::STANDARD},
        {0x555, CANFrameType::STANDARD},
        // 29-bit IDs (Extended)
        {0x18DA10F1, CANFrameType::EXTENDED},
        {0x10FEF100, CANFrameType::EXTENDED},
        {0x0CFFD040, CANFrameType::EXTENDED}
    };
    const int num_predefined_ids = sizeof(predefined_ids) / sizeof(predefined_ids[0]);

    CANMessage msg;

    // Select a random ID from the predefined list
    int index = random(num_predefined_ids);
    msg.id = predefined_ids[index].id;
    msg.type = predefined_ids[index].type;

    // Random DLC (0-8)
    msg.dlc = random(9);

    // Random data
    for (int i = 0; i < msg.dlc; i++) {
        msg.data[i] = random(256);
    }

    // Timestamp
    msg.timestamp_ms = millis();

    // RTR (rare)
    msg.rtr = (random(100) < 5);  // 5% chance of RTR

    return msg;
}

/**
 * Inject a test message into the sniffer
 */
void inject_test_message() {
    CANMessage msg = generate_random_can_message();

    // Inject into sniffer buffer
    sniffer.inject_test_message(msg);

    // Log to serial (optional, can be disabled for performance)
    if (test_message_count % 100 == 0) {
        // Only print every 100th message to avoid flooding serial
        Serial.printf("TEST: %lu messages generated (ID=0x%X)\n",
                     test_message_count + 1, msg.id);
    }

    test_message_count++;
}

/**
 * Handle serial commands for testing
 */
void handle_serial_commands() {
    if (!Serial.available()) return;

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.equalsIgnoreCase("help") || cmd.equals("?")) {
        Serial.println("\n=== PocketCAN Commands ===");
        Serial.println("start        - Start test message generation");
        Serial.println("stop         - Stop test message generation");
        Serial.println("status       - Show test mode status");
        Serial.println("interval <ms>- Set message interval (default: 100ms)");
        Serial.println("single       - Generate single test message");
        Serial.println("burst <n>    - Generate n test messages");
        Serial.println("can          - Show detailed CAN bus status");
        Serial.println("gpio         - Show GPIO pin configuration");
        Serial.println("help         - Show this help");
        Serial.println("==========================\n");
    }
    else if (cmd.equalsIgnoreCase("gpio")) {
        Serial.println("\n======= GPIO Test =======");
        Serial.printf("CAN TX Pin: GPIO_%d\n", CAN_TX_PIN);
        Serial.printf("CAN RX Pin: GPIO_%d\n", CAN_RX_PIN);
        Serial.println("\n⚠️  WARNING: GPIO 1/2 are typically I2C (SDA/SCL) pins!");
        Serial.println("    These may already be in use by:");
        Serial.println("    - Touch controller");
        Serial.println("    - Power management");
        Serial.println("    - Other I2C peripherals");
        Serial.println("\n    If CAN doesn't work, try different GPIO pins!");
        Serial.println("\n    Suggested alternatives:");
        Serial.println("    - GPIO 10/11 (if available)");
        Serial.println("    - GPIO 12/13 (if available)");
        Serial.println("    - Check Tab5 schematic for free pins");

        // Try reading the RX pin
        pinMode(CAN_RX_PIN, INPUT);
        Serial.printf("\nRX Pin State: %s\n", digitalRead(CAN_RX_PIN) ? "HIGH" : "LOW");

        // Try toggling TX pin
        Serial.println("Toggling TX pin 5 times (watch with multimeter)...");
        pinMode(CAN_TX_PIN, OUTPUT);
        for (int i = 0; i < 5; i++) {
            digitalWrite(CAN_TX_PIN, HIGH);
            delay(200);
            Serial.printf("  TX = HIGH (iteration %d)\n", i+1);
            digitalWrite(CAN_TX_PIN, LOW);
            delay(200);
            Serial.printf("  TX = LOW (iteration %d)\n", i+1);
        }

        Serial.println("\n⚠️  GPIO test may interfere with I2C/CAN. Power cycle after test!");
        Serial.println("========================\n");
    }
    else if (cmd.equalsIgnoreCase("can")) {
        Serial.println("\n======= CAN Status =======");
        Serial.printf("Sniffer running: %s\n", sniffer.is_running() ? "YES" : "NO");
        Serial.printf("Messages received: %lu\n", sniffer.get_message_count());
        Serial.printf("Messages/sec: %.1f\n", sniffer.get_messages_per_second());
        CANStats stats = CANHAL::get_stats();
        Serial.printf("Available in queue: %lu\n", CANHAL::available());
        Serial.printf("RX count: %lu\n", stats.rx_count);
        Serial.printf("TX count: %lu\n", stats.tx_count);
        Serial.printf("Error count: %lu\n", stats.error_count);
        Serial.printf("Bus error: %s\n", CANHAL::is_bus_error() ? "YES" : "NO");

        // Get detailed TWAI status
        twai_status_info_t twai_status;
        if (twai_get_status_info(&twai_status) == ESP_OK) {
            const char* states[] = {"STOPPED", "RUNNING", "BUS_OFF", "RECOVERING"};
            Serial.printf("\n--- TWAI Controller ---\n");
            Serial.printf("State: %s (%d)\n", states[twai_status.state], twai_status.state);
            Serial.printf("TX Queue: %lu msgs\n", twai_status.msgs_to_tx);
            Serial.printf("RX Queue: %lu msgs\n", twai_status.msgs_to_rx);
            Serial.printf("TX Errors: %lu\n", twai_status.tx_error_counter);
            Serial.printf("RX Errors: %lu\n", twai_status.rx_error_counter);
            Serial.printf("TX Failed: %lu\n", twai_status.tx_failed_count);
            Serial.printf("RX Missed: %lu\n", twai_status.rx_missed_count);
            Serial.printf("RX Overrun: %lu\n", twai_status.rx_overrun_count);
            Serial.printf("Arbitration Lost: %lu\n", twai_status.arb_lost_count);
            Serial.printf("Bus Error: %lu\n", twai_status.bus_error_count);
        }
        Serial.println("==========================\n");
    }
    else if (cmd.equalsIgnoreCase("start")) {
        test_mode_enabled = true;
        test_message_count = 0;
        last_test_message_time = millis();
        Serial.println("✓ Test mode STARTED");
        Serial.printf("  Interval: %lu ms\n", test_message_interval);
    }
    else if (cmd.equalsIgnoreCase("stop")) {
        test_mode_enabled = false;
        Serial.println("✓ Test mode STOPPED");
        Serial.printf("  Total messages generated: %lu\n", test_message_count);
    }
    else if (cmd.equalsIgnoreCase("status")) {
        Serial.println("\n=== Test Mode Status ===");
        Serial.printf("Enabled: %s\n", test_mode_enabled ? "YES" : "NO");
        Serial.printf("Interval: %lu ms\n", test_message_interval);
        Serial.printf("Messages generated: %lu\n", test_message_count);
        Serial.printf("Sniffer buffer size: %lu\n", sniffer.get_message_count());
        Serial.printf("Messages/sec: %.1f\n", sniffer.get_messages_per_second());
        Serial.println("========================\n");
    }
    else if (cmd.startsWith("interval ")) {
        int interval = cmd.substring(9).toInt();
        if (interval >= 10 && interval <= 10000) {
            test_message_interval = interval;
            Serial.printf("✓ Interval set to %d ms\n", interval);
        } else {
            Serial.println("✗ Invalid interval (range: 10-10000 ms)");
        }
    }
    else if (cmd.equalsIgnoreCase("single")) {
        inject_test_message();
        Serial.println("✓ Generated 1 test message");
    }
    else if (cmd.startsWith("burst ")) {
        int count = cmd.substring(6).toInt();
        if (count > 0 && count <= 1000) {
            Serial.printf("Generating %d messages...\n", count);
            for (int i = 0; i < count; i++) {
                inject_test_message();
                delay(1);  // Small delay to prevent buffer overflow
            }
            Serial.printf("✓ Generated %d test messages\n", count);
        } else {
            Serial.println("✗ Invalid count (range: 1-1000)");
        }
    }
    else {
        Serial.printf("✗ Unknown command: '%s'\n", cmd.c_str());
        Serial.println("Type 'help' for available commands");
    }
}

/**
 * Update test message generator
 */
void update_test_generator() {
    if (!test_mode_enabled) return;

    uint32_t now = millis();
    if (now - last_test_message_time >= test_message_interval) {
        inject_test_message();
        last_test_message_time = now;
    }
}

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
    SettingsManager::init();
    ScopeManager::init();

    // Initialize Core Modules
    Serial.println("Initializing Core Modules...");
    sniffer.init();
    // NOTE: Sniffer is NOT auto-started - user must click Start button on screen
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
    Serial.println();
    Serial.println("Type 'help' for test mode commands");
    Serial.println();
}

void loop() {
    // Handle serial commands (test mode)
    handle_serial_commands();

    // Update test message generator
    update_test_generator();

    // Handle LVGL tasks
    lv_timer_handler();

    // Update core modules
    sniffer.update();
    transmitter.update();
    emulator.update();

    // Update scope manager
    ScopeManager::update();

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
    // digitalWrite(49, HIGH);
    // digitalWrite(50, LOW);
    // delay(2000); // 3.3V on Pin A, 0V on Pin B
    
    // digitalWrite(49, LOW);
    // digitalWrite(50, HIGH);
    // delay(2000); // 0V on Pin A, 3.3V on Pin B
}

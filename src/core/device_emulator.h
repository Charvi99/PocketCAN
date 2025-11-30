#pragma once

/**
 * Device Emulator
 * Emulates CAN devices by responding to specific messages
 */

#include "../hal/can_hal.h"

struct EmulationRule {
    uint32_t trigger_id;            // CAN ID that triggers response
    CANMessage response;            // Message to send in response
    uint32_t delay_ms;              // Delay before sending response
    bool enabled;
    char name[32];                  // Rule name/description
};

class DeviceEmulator {
public:
    /**
     * Initialize emulator
     */
    void init();

    /**
     * Start emulation
     */
    void start();

    /**
     * Stop emulation
     */
    void stop();

    /**
     * Check if emulation is active
     */
    bool is_running() const { return running; }

    /**
     * Add emulation rule
     * @return Rule index, or -1 if failed
     */
    int add_rule(const EmulationRule& rule);

    /**
     * Remove emulation rule
     */
    bool remove_rule(int index);

    /**
     * Enable/disable rule
     */
    bool set_rule_enabled(int index, bool enabled);

    /**
     * Get rule by index
     */
    const EmulationRule* get_rule(int index) const;

    /**
     * Get number of rules
     */
    int get_rule_count() const { return rule_count; }

    /**
     * Clear all rules
     */
    void clear_rules();

    /**
     * Process incoming message for emulation
     */
    void process_message(const CANMessage& msg);

    /**
     * Update - call from main loop
     */
    void update();

    /**
     * Load emulation profile from file
     * @param filename Profile file path
     * @return true if successful
     */
    bool load_profile(const char* filename);

    /**
     * Save emulation profile to file
     * @param filename Profile file path
     * @return true if successful
     */
    bool save_profile(const char* filename);

private:
    static constexpr int MAX_RULES = 20;

    struct PendingResponse {
        CANMessage message;
        uint32_t send_time;
        bool active;
    };

    EmulationRule rules[MAX_RULES];
    int rule_count = 0;
    bool running = false;

    PendingResponse pending_responses[MAX_RULES];
    int pending_count = 0;

    void add_pending_response(const CANMessage& msg, uint32_t delay_ms);
};

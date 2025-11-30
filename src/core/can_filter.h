#pragma once

/**
 * CAN Message Filter
 * Filters and masks CAN messages based on ID and data patterns
 */

#include "../hal/can_hal.h"
#include "../config/can_config.h"

struct FilterRule {
    uint32_t id;                    // CAN ID to filter
    uint32_t mask;                  // Mask (1 = must match, 0 = don't care)
    bool enabled;                   // Rule active
    bool accept;                    // true = accept, false = reject
    CANFrameType type;              // Standard or Extended
};

class CANFilter {
public:
    /**
     * Initialize filter
     */
    void init();

    /**
     * Add filter rule
     * @return Rule index, or -1 if failed
     */
    int add_rule(const FilterRule& rule);

    /**
     * Remove filter rule
     */
    bool remove_rule(int index);

    /**
     * Clear all rules
     */
    void clear_rules();

    /**
     * Enable/disable a rule
     */
    bool set_rule_enabled(int index, bool enabled);

    /**
     * Get rule by index
     */
    const FilterRule* get_rule(int index) const;

    /**
     * Get number of active rules
     */
    int get_rule_count() const;

    /**
     * Check if message passes filter
     * @param msg Message to check
     * @return true if message should pass
     */
    bool check_message(const CANMessage& msg) const;

    /**
     * Enable/disable filtering (bypass all filters)
     */
    void set_enabled(bool enabled);

    /**
     * Check if filtering is enabled
     */
    bool is_enabled() const { return filter_enabled; }

private:
    FilterRule rules[MAX_FILTERS];
    int rule_count = 0;
    bool filter_enabled = false;

    /**
     * Check if ID matches rule
     */
    bool id_matches(uint32_t id, const FilterRule& rule) const;
};

#include "can_filter.h"
#include <Arduino.h>
#include <cstring>

void CANFilter::init() {
    clear_rules();
    filter_enabled = false;
}

int CANFilter::add_rule(const FilterRule& rule) {
    if (rule_count >= MAX_FILTERS) {
        Serial.println("ERROR: Maximum filters reached");
        return -1;
    }

    rules[rule_count] = rule;
    return rule_count++;
}

bool CANFilter::remove_rule(int index) {
    if (index < 0 || index >= rule_count) {
        return false;
    }

    // Shift rules down
    for (int i = index; i < rule_count - 1; i++) {
        rules[i] = rules[i + 1];
    }
    rule_count--;
    return true;
}

void CANFilter::clear_rules() {
    rule_count = 0;
    memset(rules, 0, sizeof(rules));
}

bool CANFilter::set_rule_enabled(int index, bool enabled) {
    if (index < 0 || index >= rule_count) {
        return false;
    }
    rules[index].enabled = enabled;
    return true;
}

const FilterRule* CANFilter::get_rule(int index) const {
    if (index < 0 || index >= rule_count) {
        return nullptr;
    }
    return &rules[index];
}

int CANFilter::get_rule_count() const {
    return rule_count;
}

bool CANFilter::id_matches(uint32_t id, const FilterRule& rule) const {
    // Apply mask and compare
    return (id & rule.mask) == (rule.id & rule.mask);
}

bool CANFilter::check_message(const CANMessage& msg) const {
    // If filtering disabled, accept all
    if (!filter_enabled) {
        return true;
    }

    // If no rules, accept all
    if (rule_count == 0) {
        return true;
    }

    bool has_any_accept_rules = false;

    // First Match Wins Policy
    for (int i = 0; i < rule_count; i++) {
        if (!rules[i].enabled) {
            continue;
        }

        if (rules[i].accept) {
            has_any_accept_rules = true;
        }

        // Check frame type matches
        if (rules[i].type != msg.type) {
            continue;
        }

        // Check if ID matches
        if (id_matches(msg.id, rules[i])) {
            // Found a match! Return the rule's decision immediately.
            return rules[i].accept;
        }
    }

    // No rules matched this message. Determine default policy.
    
    // If we have ANY "Accept" rules defined, we are in "Whitelist Mode" (Default Deny).
    if (has_any_accept_rules) {
        return false;
    }

    // Otherwise we are in "Blacklist Mode" (Default Allow).
    return true;
}

void CANFilter::set_enabled(bool enabled) {
    filter_enabled = enabled;
}

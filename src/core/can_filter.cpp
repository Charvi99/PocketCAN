#include "can_filter.h"
#include <cstring>

void CANFilter::init() {
    clear_rules();
    filter_enabled = false;
}

int CANFilter::add_rule(const FilterRule& rule) {
    if (rule_count >= MAX_FILTERS) {
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
    if (!filter_enabled) {
        return true;
    }
    if (rule_count == 0) {
        return true;
    }

    bool matched_accept = false;
    bool matched_reject = false;
    bool allow_list_present = false;

    for (int i = 0; i < rule_count; i++) {
        if (!rules[i].enabled) {
            continue;
        }

        // Counted before the frame-type check: an accept rule for extended
        // frames still means the user is running an allow-list, and a
        // standard frame that matches nothing must not sail through it.
        if (rules[i].accept) {
            allow_list_present = true;
        }

        if (rules[i].type != msg.type) {
            continue;
        }

        if (id_matches(msg.id, rules[i])) {
            if (rules[i].accept) {
                matched_accept = true;
            } else {
                matched_reject = true;
            }
        }
    }

    // Reject always wins over accept.
    if (matched_reject) {
        return false;
    }
    if (matched_accept) {
        return true;
    }

    // Nothing matched. With an allow-list active that means reject; with only
    // reject rules configured it means this frame was never on the block list.
    return !allow_list_present;
}

void CANFilter::set_enabled(bool enabled) {
    filter_enabled = enabled;
}

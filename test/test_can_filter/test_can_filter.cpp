#include <unity.h>

#include "core/can_filter.h"

void setUp(void) {}
void tearDown(void) {}

static CANMessage make_msg(uint32_t id) {
    CANMessage m = {};
    m.id   = id;
    m.dlc  = 8;
    m.type = CANFrameType::STANDARD;
    return m;
}

static FilterRule make_rule(uint32_t id, bool accept) {
    FilterRule r = {};
    r.id      = id;
    r.mask    = 0x7FF;          // exact match on a standard ID
    r.enabled = true;
    r.accept  = accept;
    r.type    = CANFrameType::STANDARD;
    return r;
}

// Guards defect #3. An allow-list must reject everything it did not name.
void test_filter_accept_list_rejects_unmatched(void) {
    CANFilter f;
    f.init();
    f.set_enabled(true);
    f.add_rule(make_rule(0x123, true));

    TEST_ASSERT_TRUE(f.check_message(make_msg(0x123)));
    TEST_ASSERT_FALSE(f.check_message(make_msg(0x456)));
}

// The complementary direction: a reject-list passes everything it did not name.
void test_filter_reject_list_blocks_matched(void) {
    CANFilter f;
    f.init();
    f.set_enabled(true);
    f.add_rule(make_rule(0x123, false));

    TEST_ASSERT_FALSE(f.check_message(make_msg(0x123)));
    TEST_ASSERT_TRUE(f.check_message(make_msg(0x456)));
}

// An accept rule for a different frame type still constitutes an allow-list,
// so a standard frame that matches nothing must still be rejected.
void test_filter_accept_list_applies_across_frame_types(void) {
    CANFilter f;
    f.init();
    f.set_enabled(true);
    FilterRule ext = make_rule(0x123, true);
    ext.type = CANFrameType::EXTENDED;
    f.add_rule(ext);

    TEST_ASSERT_FALSE(f.check_message(make_msg(0x123)));   // standard frame
}

// Filtering off, or no rules at all, means pass everything.
void test_filter_disabled_passes_all(void) {
    CANFilter f;
    f.init();
    TEST_ASSERT_TRUE(f.check_message(make_msg(0x999)));

    f.set_enabled(true);
    TEST_ASSERT_TRUE(f.check_message(make_msg(0x999)));   // enabled, zero rules
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_filter_accept_list_rejects_unmatched);
    RUN_TEST(test_filter_reject_list_blocks_matched);
    RUN_TEST(test_filter_accept_list_applies_across_frame_types);
    RUN_TEST(test_filter_disabled_passes_all);
    return UNITY_END();
}

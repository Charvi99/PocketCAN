#include <unity.h>

#include "services/can_stats.h"

void setUp(void) {}
void tearDown(void) {}

static CANMessage std_frame(uint8_t dlc) {
    CANMessage m = {};
    m.id   = 0x123;
    m.dlc  = dlc;
    m.type = CANFrameType::STANDARD;
    return m;
}

// Guards defect #12. 100 standard frames with dlc=8 at 500 kbit/s:
//   bits per frame = (47 + 8*8) = 111, x1.1 for stuffing = 122
//   100 frames     = 12200 bits over a 1 second window
//   load           = 12200 / 500000 = 2.44%
void test_bus_load_matches_known_traffic(void) {
    CanStatsCollector s;
    s.reset(0);
    s.set_bitrate(500000);

    for (int i = 0; i < 100; i++) {
        s.record_rx(std_frame(8));
    }
    s.update(1000);   // close the window

    TEST_ASSERT_FLOAT_WITHIN(0.05f, 2.44f, s.bus_load_percent());
    TEST_ASSERT_EQUAL_UINT32(100, s.rx_count());
}

// An extended frame carries 20 more bits of overhead than a standard one.
void test_bus_load_accounts_for_extended_frames(void) {
    CanStatsCollector s;
    s.reset(0);
    s.set_bitrate(500000);

    CANMessage ext = std_frame(8);
    ext.type = CANFrameType::EXTENDED;
    for (int i = 0; i < 100; i++) {
        s.record_rx(ext);
    }
    s.update(1000);

    // (67 + 64) = 131, x1.1 = 144; 14400 / 500000 = 2.88%
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 2.88f, s.bus_load_percent());
}

// A silent bus reads zero, and the window rolls rather than accumulating.
void test_bus_load_returns_to_zero_on_a_silent_bus(void) {
    CanStatsCollector s;
    s.reset(0);
    s.set_bitrate(500000);

    for (int i = 0; i < 100; i++) {
        s.record_rx(std_frame(8));
    }
    s.update(1000);
    TEST_ASSERT_TRUE(s.bus_load_percent() > 2.0f);

    s.update(2000);   // a second window with no traffic
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, s.bus_load_percent());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_bus_load_matches_known_traffic);
    RUN_TEST(test_bus_load_accounts_for_extended_frames);
    RUN_TEST(test_bus_load_returns_to_zero_on_a_silent_bus);
    return UNITY_END();
}

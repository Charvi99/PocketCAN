#include <unity.h>

#include "core/can_timing.h"

void setUp(void) {}
void tearDown(void) {}

static const CANBaudRate ALL_RATES[] = {
    CANBaudRate::BAUD_10K,  CANBaudRate::BAUD_20K,  CANBaudRate::BAUD_50K,
    CANBaudRate::BAUD_100K, CANBaudRate::BAUD_125K, CANBaudRate::BAUD_250K,
    CANBaudRate::BAUD_500K, CANBaudRate::BAUD_800K, CANBaudRate::BAUD_1M,
};
static const int RATE_COUNT = sizeof(ALL_RATES) / sizeof(ALL_RATES[0]);

// Guards defect #9 (20K mapped to 25K) and #10 (10K missing entirely).
// Every rate must map to a spec whose nominal bit rate equals the enum's
// own numeric value. The 20K bug fails the equality; the 10K bug fails it
// too, because the default branch returns 500000.
void test_can_timing_table_is_complete_and_distinct(void) {
    for (int i = 0; i < RATE_COUNT; i++) {
        CanTimingSpec spec = can_timing_for(ALL_RATES[i]);
        TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(ALL_RATES[i]), spec.nominal_bps);
    }

    // No two rates may share a timing id.
    for (int i = 0; i < RATE_COUNT; i++) {
        for (int j = i + 1; j < RATE_COUNT; j++) {
            TEST_ASSERT_NOT_EQUAL(
                static_cast<int>(can_timing_for(ALL_RATES[i]).id),
                static_cast<int>(can_timing_for(ALL_RATES[j]).id));
        }
    }
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_can_timing_table_is_complete_and_distinct);
    return UNITY_END();
}

#include <unity.h>

#include "core/can_transmitter.h"
#include "fakes/fake_can_bus.h"

void setUp(void) {}
void tearDown(void) {}

static CANMessage make_msg(uint32_t id) {
    CANMessage m = {};
    m.id   = id;
    m.dlc  = 2;
    m.type = CANFrameType::STANDARD;
    return m;
}

void test_transmitter_send_reaches_the_bus(void) {
    FakeCanBus bus;
    CANTransmitter tx(bus);
    tx.init(0);

    TEST_ASSERT_TRUE(tx.send(make_msg(0x100)));
    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());
    TEST_ASSERT_EQUAL_UINT32(0x100, bus.sent[0].id);
}

void test_transmitter_periodic_interval(void) {
    FakeCanBus bus;
    CANTransmitter tx(bus);
    tx.init(1000);

    TEST_ASSERT_EQUAL_INT(0, tx.add_periodic(make_msg(0x200), 100));

    tx.update(1050);
    TEST_ASSERT_EQUAL_UINT32(0, bus.sent.size());   // not yet due

    tx.update(1100);
    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());   // due

    tx.update(1150);
    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());   // not due again

    tx.update(1200);
    TEST_ASSERT_EQUAL_UINT32(2, bus.sent.size());
}

// The transmitter already used overflow-safe subtraction; this pins it so a
// later edit cannot regress it into the emulator's mistake.
void test_transmitter_periodic_survives_millis_wrap(void) {
    FakeCanBus bus;
    CANTransmitter tx(bus);
    const uint32_t near_wrap = 0xFFFFFF00u;
    tx.init(near_wrap);

    tx.add_periodic(make_msg(0x300), 500);

    tx.update(near_wrap + 400);   // still 0xFFFFFF... - not due
    TEST_ASSERT_EQUAL_UINT32(0, bus.sent.size());

    tx.update(near_wrap + 500);   // wrapped past zero - due
    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_transmitter_send_reaches_the_bus);
    RUN_TEST(test_transmitter_periodic_interval);
    RUN_TEST(test_transmitter_periodic_survives_millis_wrap);
    return UNITY_END();
}

#include <unity.h>

#include <cstring>

#include "core/device_emulator.h"
#include "fakes/fake_can_bus.h"

void setUp(void) {}
void tearDown(void) {}

static CANMessage make_msg(uint32_t id) {
    CANMessage m = {};
    m.id   = id;
    m.dlc  = 1;
    m.type = CANFrameType::STANDARD;
    return m;
}

static EmulationRule make_rule(uint32_t trigger, uint32_t response_id, uint32_t delay_ms) {
    EmulationRule r = {};
    r.trigger_id = trigger;
    r.response   = make_msg(response_id);
    r.delay_ms   = delay_ms;
    r.enabled    = true;
    std::strncpy(r.name, "test", sizeof(r.name) - 1);
    return r;
}

void test_emulator_responds_to_trigger_after_delay(void) {
    FakeCanBus bus;
    DeviceEmulator em(bus);
    em.init();
    em.start();
    em.add_rule(make_rule(0x7DF, 0x7E8, 50));

    em.process_message(make_msg(0x7DF), 1000);

    em.update(1040);
    TEST_ASSERT_EQUAL_UINT32(0, bus.sent.size());   // delay not elapsed

    em.update(1050);
    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());
    TEST_ASSERT_EQUAL_UINT32(0x7E8, bus.sent[0].id);

    em.update(1100);
    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());   // fires exactly once
}

// Guards defect #7. send_time wraps past zero; the response must still fire
// at the right moment, and must not fire early.
void test_emulator_fires_across_millis_wrap(void) {
    FakeCanBus bus;
    DeviceEmulator em(bus);
    em.init();
    em.start();
    em.add_rule(make_rule(0x100, 0x200, 500));

    const uint32_t near_wrap = 0xFFFFFF00u;   // send_time becomes 0x000000F4
    em.process_message(make_msg(0x100), near_wrap);

    em.update(near_wrap + 400);    // wraps to 0x90 - not due
    TEST_ASSERT_EQUAL_UINT32(0, bus.sent.size());

    em.update(near_wrap + 500);    // wraps to 0xF4 - due
    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());
}

void test_emulator_ignores_messages_when_stopped(void) {
    FakeCanBus bus;
    DeviceEmulator em(bus);
    em.init();
    em.add_rule(make_rule(0x100, 0x200, 0));

    em.process_message(make_msg(0x100), 1000);   // never started
    em.update(2000);
    TEST_ASSERT_EQUAL_UINT32(0, bus.sent.size());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_emulator_responds_to_trigger_after_delay);
    RUN_TEST(test_emulator_fires_across_millis_wrap);
    RUN_TEST(test_emulator_ignores_messages_when_stopped);
    return UNITY_END();
}

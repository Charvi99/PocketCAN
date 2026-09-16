#include <unity.h>

#include "fakes/fake_can_bus.h"
#include "services/can_service.h"

void setUp(void) {}
void tearDown(void) {}

static FilterRule accept_rule(uint32_t id) {
    FilterRule r = {};
    r.id      = id;
    r.mask    = 0x7FF;
    r.enabled = true;
    r.accept  = true;
    r.type    = CANFrameType::STANDARD;
    return r;
}

// A flooded bus must not be drained in one tick - lv_timer_handler() has to
// get a turn.
void test_can_service_respects_frame_budget(void) {
    FakeCanBus bus;
    CanService svc(bus);
    svc.init(0);
    svc.start(0);

    for (int i = 0; i < 1000; i++) {
        bus.queue(0x100 + i);
    }

    svc.update(10);

    TEST_ASSERT_EQUAL_UINT32(MAX_FRAMES_PER_TICK, svc.history().size());
    TEST_ASSERT_EQUAL_UINT32(1000 - MAX_FRAMES_PER_TICK, bus.rx_queue.size());
}

// The full pipeline: receive -> filter -> history -> emulator -> stats.
void test_can_service_routes_filtered_frames_to_history_and_emulator(void) {
    FakeCanBus bus;
    CanService svc(bus);
    svc.init(0);
    svc.start(0);

    svc.filter().add_rule(accept_rule(0x7DF));
    svc.filter().set_enabled(true);

    EmulationRule rule = {};
    rule.trigger_id      = 0x7DF;
    rule.response        = CANMessage{};
    rule.response.id     = 0x7E8;
    rule.response.dlc    = 1;
    rule.response.type   = CANFrameType::STANDARD;
    rule.delay_ms        = 0;
    rule.enabled         = true;
    svc.emulator().add_rule(rule);
    svc.emulator().start();

    bus.queue(0x7DF);    // passes the filter, triggers the emulator
    bus.queue(0x456);    // filtered out

    svc.update(10);

    TEST_ASSERT_EQUAL_UINT32(1, svc.history().size());
    const CANMessage* captured = svc.history().get(0);
    TEST_ASSERT_NOT_NULL(captured);
    TEST_ASSERT_EQUAL_UINT32(0x7DF, captured->id);

    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());
    TEST_ASSERT_EQUAL_UINT32(0x7E8, bus.sent[0].id);
}

// Capture paused means no history, but the bus is still drained so the
// hardware RX queue does not overflow.
void test_can_service_drains_bus_while_capture_is_paused(void) {
    FakeCanBus bus;
    CanService svc(bus);
    svc.init(0);
    svc.start(0);
    svc.sniffer().stop();

    bus.queue(0x100);
    bus.queue(0x101);
    svc.update(10);

    TEST_ASSERT_EQUAL_UINT32(0, svc.history().size());
    TEST_ASSERT_EQUAL_UINT32(0, bus.rx_queue.size());
}

void test_can_service_recovers_from_bus_off(void) {
    FakeCanBus bus;
    CanService svc(bus);
    svc.init(0);
    svc.start(0);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(CANBusState::RUNNING),
                          static_cast<int>(svc.bus_state()));

    bus.bus_error = true;
    svc.update(1000);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(CANBusState::RECOVERING),
                          static_cast<int>(svc.bus_state()));
    TEST_ASSERT_TRUE(bus.recover_called);

    // Recovery succeeded on the hardware; the settle delay must still elapse.
    bus.bus_error = false;
    svc.update(1050);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(CANBusState::RECOVERING),
                          static_cast<int>(svc.bus_state()));

    svc.update(1100);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(CANBusState::RUNNING),
                          static_cast<int>(svc.bus_state()));
}

void test_can_service_gives_up_after_five_attempts(void) {
    FakeCanBus bus;
    CanService svc(bus);
    svc.init(0);
    svc.start(0);

    bus.bus_error = true;   // never clears
    uint32_t now = 1000;
    for (int attempt = 0; attempt < 6; attempt++) {
        svc.update(now);          // notice, initiate recovery
        now += 200;
        svc.update(now);          // settle elapsed, still bus-off -> retry
        now += 200;
    }

    TEST_ASSERT_EQUAL_INT(static_cast<int>(CANBusState::FAILED),
                          static_cast<int>(svc.bus_state()));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_can_service_respects_frame_budget);
    RUN_TEST(test_can_service_routes_filtered_frames_to_history_and_emulator);
    RUN_TEST(test_can_service_drains_bus_while_capture_is_paused);
    RUN_TEST(test_can_service_recovers_from_bus_off);
    RUN_TEST(test_can_service_gives_up_after_five_attempts);
    return UNITY_END();
}

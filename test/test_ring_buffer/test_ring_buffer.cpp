#include <unity.h>

#include "utils/ring_buffer.h"

void setUp(void) {}
void tearDown(void) {}

// Guards: overwrite semantics at capacity.
void test_ring_buffer_wraps_and_reports_size(void) {
    RingBuffer<int> rb(3);

    rb.push(1);
    rb.push(2);
    rb.push(3);
    TEST_ASSERT_EQUAL_UINT32(3, rb.size());
    TEST_ASSERT_TRUE(rb.is_full());

    rb.push(4);   // overwrites the oldest (1)

    TEST_ASSERT_EQUAL_UINT32(3, rb.size());
    int oldest = 0;
    TEST_ASSERT_TRUE(rb.peek(0, oldest));
    TEST_ASSERT_EQUAL_INT(2, oldest);

    int newest = 0;
    TEST_ASSERT_TRUE(rb.peek(2, newest));
    TEST_ASSERT_EQUAL_INT(4, newest);
}

// Guards: push() always returns true; its doc comment claimed otherwise.
void test_ring_buffer_push_returns_documented_value(void) {
    RingBuffer<int> rb(2);
    TEST_ASSERT_TRUE(rb.push(1));
    TEST_ASSERT_TRUE(rb.push(2));
    TEST_ASSERT_TRUE(rb.push(3));   // full - still true, oldest discarded
    TEST_ASSERT_EQUAL_UINT32(2, rb.size());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_ring_buffer_wraps_and_reports_size);
    RUN_TEST(test_ring_buffer_push_returns_documented_value);
    return UNITY_END();
}

#pragma once

/**
 * Time Utilities
 * Helper functions for timestamp formatting
 */

#include <cstdint>
#include <cstdio>
#include <Arduino.h>

class TimeUtils {
public:
    /**
     * Format timestamp as HH:MM:SS.mmm
     * @param timestamp_ms Timestamp in milliseconds
     * @param output Output buffer (must be at least 13 bytes)
     */
    static void format_timestamp(uint32_t timestamp_ms, char* output) {
        uint32_t total_sec = timestamp_ms / 1000;
        uint32_t ms = timestamp_ms % 1000;
        uint32_t hours = total_sec / 3600;
        uint32_t minutes = (total_sec % 3600) / 60;
        uint32_t seconds = total_sec % 60;

        sprintf(output, "%02lu:%02lu:%02lu.%03lu", hours, minutes, seconds, ms);
    }

    /**
     * Format timestamp as relative time (e.g., "2.345s", "123ms")
     * @param timestamp_ms Timestamp in milliseconds
     * @param reference_ms Reference timestamp
     * @param output Output buffer (must be at least 16 bytes)
     */
    static void format_relative_time(uint32_t timestamp_ms, uint32_t reference_ms, char* output) {
        int32_t diff = timestamp_ms - reference_ms;
        if (abs(diff) < 1000) {
            sprintf(output, "%ldms", diff);
        } else {
            float seconds = diff / 1000.0f;
            sprintf(output, "%.3fs", seconds);
        }
    }

    /**
     * Get current uptime in milliseconds
     */
    static uint32_t get_uptime_ms() {
        return millis();
    }

    /**
     * Get current uptime in microseconds
     */
    static uint64_t get_uptime_us() {
        return micros();
    }
};

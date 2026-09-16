#pragma once

/**
 * Scope Manager
 * Manages SPI communication with external oscilloscope (Nucleo-G431RB)
 * For CAN signal analysis and protocol decoding
 */

#include <Arduino.h>
#include <SPI.h>

#define SCOPE_SPI_SPEED     10000000  // 10 MHz SPI clock
#define SCOPE_BUFFER_SIZE   512       // Smaller buffer to prevent crashes
#define SCOPE_SPI_MODE      SPI_MODE0 // CPOL=0, CPHA=0

class ScopeManager {
public:
    /**
     * Initialize scope manager
     * Sets up serial communication with external scope
     */
    static bool init();

    /**
     * Update - call from main loop
     * Reads data from serial port
     */
    static void update();

    /**
     * Check if scope is connected
     */
    static bool is_connected();

    /**
     * Get received data buffer
     */
    static const char* get_buffer();

    /**
     * Get buffer size (actual bytes, not strlen)
     */
    static uint32_t get_buffer_size();

    /**
     * Get number of bytes received
     */
    static uint32_t get_bytes_received();

    /**
     * Clear buffer
     */
    static void clear_buffer();

    /**
     * Get bytes per second rate
     */
    static float get_bytes_per_second();

private:
    static SPIClass* scope_spi;
    static char rx_buffer[SCOPE_BUFFER_SIZE];
    static uint32_t buffer_index;
    static uint32_t total_bytes_received;
    static uint32_t last_stats_time;
    static uint32_t bytes_this_second;
    static float bytes_per_second;
    static bool initialized;
    static int cs_pin;
};

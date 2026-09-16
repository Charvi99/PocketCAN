#include "scope_manager.h"
#include "../config/hardware_config.h"
#include <Arduino.h>

SPIClass* ScopeManager::scope_spi = nullptr;
char ScopeManager::rx_buffer[SCOPE_BUFFER_SIZE] = {0};
uint32_t ScopeManager::buffer_index = 0;
uint32_t ScopeManager::total_bytes_received = 0;
uint32_t ScopeManager::last_stats_time = 0;
uint32_t ScopeManager::bytes_this_second = 0;
float ScopeManager::bytes_per_second = 0.0f;
bool ScopeManager::initialized = false;
int ScopeManager::cs_pin = SCOPE_SPI_CS;

bool ScopeManager::init() {
    if (initialized) {
        return true;
    }

    Serial.println("========================================");
    Serial.println("Scope Manager Initialization (SPI)");
    Serial.println("========================================");

    // Initialize SPI for scope communication
    // Using HSPI bus on ESP32
    scope_spi = new SPIClass(HSPI);
    scope_spi->begin(SCOPE_SPI_SCK, SCOPE_SPI_MISO, SCOPE_SPI_MOSI, SCOPE_SPI_CS);

    // Configure CS pin
    pinMode(cs_pin, OUTPUT);
    digitalWrite(cs_pin, HIGH);  // CS idle high

    // Give SPI time to initialize
    delay(100);

    Serial.printf("Scope SPI initialized:\n");
    Serial.printf("  SPI Speed: %d Hz\n", SCOPE_SPI_SPEED);
    Serial.printf("  SCK Pin: GPIO_%d\n", SCOPE_SPI_SCK);
    Serial.printf("  MISO Pin: GPIO_%d\n", SCOPE_SPI_MISO);
    Serial.printf("  MOSI Pin: GPIO_%d\n", SCOPE_SPI_MOSI);
    Serial.printf("  CS Pin: GPIO_%d\n", SCOPE_SPI_CS);
    Serial.println("========================================");

    buffer_index = 0;
    total_bytes_received = 0;
    last_stats_time = millis();
    bytes_this_second = 0;
    bytes_per_second = 0.0f;

    initialized = true;
    Serial.println("Scope Manager initialized successfully");
    return true;
}

void ScopeManager::update() {
    if (!initialized || !scope_spi) {
        return;
    }

    // Rate limit: Only do SPI transfer every 50ms (20 Hz)
    static uint32_t last_transfer_time = 0;
    uint32_t now = millis();

    if (now - last_transfer_time < 50) {
        return;  // Skip this update, too soon
    }
    last_transfer_time = now;

    // SPI transaction: request data from Nucleo
    const int chunk_size = 64;
    uint8_t tx_buffer[chunk_size];
    uint8_t rx_data[chunk_size];

    // Prepare dummy data to send (all zeros)
    memset(tx_buffer, 0x00, chunk_size);

    // Begin SPI transaction
    scope_spi->beginTransaction(SPISettings(SCOPE_SPI_SPEED, MSBFIRST, SCOPE_SPI_MODE));
    digitalWrite(cs_pin, LOW);  // Assert CS (active low)
    delayMicroseconds(10);      // Small delay for slave to respond

    // Transfer data (send dummy bytes, receive actual data from Nucleo)
    for (int i = 0; i < chunk_size; i++) {
        rx_data[i] = scope_spi->transfer(tx_buffer[i]);
    }

    delayMicroseconds(10);      // Small delay before releasing CS
    digitalWrite(cs_pin, HIGH);  // De-assert CS
    scope_spi->endTransaction();

    // Debug: Print first 16 bytes of raw SPI data every second for diagnostics
    static uint32_t last_debug_print = 0;
    if (now - last_debug_print >= 1000) {
        Serial.print("[SCOPE SPI RAW] First 16 bytes: ");
        for (int i = 0; i < 16; i++) {
            Serial.printf("%02X ", rx_data[i]);
        }
        Serial.println();
        last_debug_print = now;
    }

    // Process received data - DON'T store 0x00 bytes (they're idle/no data)
    int valid_bytes = 0;
    for (int i = 0; i < chunk_size; i++) {
        uint8_t byte = rx_data[i];

        // Skip 0x00 bytes (no data available from Nucleo)
        if (byte == 0x00) {
            continue;
        }

        valid_bytes++;
        total_bytes_received++;
        bytes_this_second++;

        // Store only non-zero bytes in buffer
        if (buffer_index < SCOPE_BUFFER_SIZE - 2) {
            rx_buffer[buffer_index++] = (char)byte;
        } else {
            // Buffer full - just stop storing, don't reset
            // This prevents the crash from continuous resets
            Serial.println("\n[SCOPE] Buffer full, ignoring new data");
            break;  // Stop processing this chunk
        }
    }

    // Always null terminate for safety
    if (buffer_index < SCOPE_BUFFER_SIZE) {
        rx_buffer[buffer_index] = '\0';
    }

    // Update statistics every second
    if (now - last_stats_time >= 1000) {
        bytes_per_second = bytes_this_second;
        bytes_this_second = 0;
        last_stats_time = now;

        // Debug output every second
        Serial.printf("[SCOPE] Buffer: %lu bytes, Rate: %.0f B/s (non-zero), Total: %lu, Valid: %d/64\n",
                      buffer_index, bytes_per_second, total_bytes_received, valid_bytes);
    }
}

bool ScopeManager::is_connected() {
    // Consider connected if we've received data recently
    return initialized && (millis() - last_stats_time < 2000) && (bytes_per_second > 0);
}

const char* ScopeManager::get_buffer() {
    return rx_buffer;
}

uint32_t ScopeManager::get_buffer_size() {
    return buffer_index;
}

uint32_t ScopeManager::get_bytes_received() {
    return total_bytes_received;
}

void ScopeManager::clear_buffer() {
    buffer_index = 0;
    total_bytes_received = 0;
    rx_buffer[0] = '\0';
    Serial.println("Scope buffer cleared");
}

float ScopeManager::get_bytes_per_second() {
    return bytes_per_second;
}

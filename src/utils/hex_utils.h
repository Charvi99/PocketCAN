#pragma once

/**
 * Hexadecimal Utilities
 * Helper functions for hex conversion and formatting
 */

#include <cstdint>
#include <cstdio>

class HexUtils {
public:
    /**
     * Convert byte array to hex string
     * @param data Input data
     * @param len Data length
     * @param output Output buffer (must be at least len*2+1 bytes)
     * @param uppercase Use uppercase hex digits
     */
    static void bytes_to_hex(const uint8_t* data, size_t len, char* output, bool uppercase = true) {
        const char* format = uppercase ? "%02X" : "%02x";
        for (size_t i = 0; i < len; i++) {
            sprintf(output + i * 2, format, data[i]);
        }
        output[len * 2] = '\0';
    }

    /**
     * Convert single byte to hex string
     */
    static void byte_to_hex(uint8_t byte, char* output, bool uppercase = true) {
        const char* format = uppercase ? "%02X" : "%02x";
        sprintf(output, format, byte);
        output[2] = '\0';
    }

    /**
     * Convert hex string to byte array
     * @param hex Input hex string
     * @param data Output buffer
     * @param max_len Maximum bytes to write
     * @return Number of bytes written
     */
    static size_t hex_to_bytes(const char* hex, uint8_t* data, size_t max_len) {
        size_t len = strlen(hex) / 2;
        if (len > max_len) {
            len = max_len;
        }

        for (size_t i = 0; i < len; i++) {
            sscanf(hex + i * 2, "%2hhx", &data[i]);
        }
        return len;
    }

    /**
     * Format CAN ID as hex string
     */
    static void format_can_id(uint32_t id, char* output, bool extended) {
        if (extended) {
            sprintf(output, "%08X", id);
        } else {
            sprintf(output, "%03X", id & 0x7FF);
        }
    }
};

#pragma once

/**
 * Storage Hardware Abstraction Layer
 * Manages SD card and internal flash storage
 */

#include <cstdint>
#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>

enum class StorageType {
    INTERNAL_FLASH,
    SD_CARD
};

class StorageHAL {
public:
    /**
     * Initialize storage system
     * @return true if successful
     */
    static bool init();

    /**
     * Check if SD card is available
     * @return true if SD card detected
     */
    static bool is_sd_available();

    /**
     * Get preferred storage (SD if available, otherwise internal)
     * @return Storage type
     */
    static StorageType get_preferred_storage();

    /**
     * Get file system for storage type
     * @param type Storage type
     * @return File system reference
     */
    static fs::FS& get_fs(StorageType type);

    /**
     * Get free space in bytes
     * @param type Storage type
     * @return Free space
     */
    static uint64_t get_free_space(StorageType type);

    /**
     * Get total space in bytes
     * @param type Storage type
     * @return Total space
     */
    static uint64_t get_total_space(StorageType type);

private:
    static bool sd_initialized;
    static bool flash_initialized;
};

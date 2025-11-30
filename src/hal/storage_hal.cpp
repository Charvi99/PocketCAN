#include "storage_hal.h"
#include <Arduino.h>

bool StorageHAL::sd_initialized = false;
bool StorageHAL::flash_initialized = false;

bool StorageHAL::init() {
    // Initialize SPIFFS (internal flash)
    if (!SPIFFS.begin(true)) {
        Serial.println("WARNING: Failed to initialize SPIFFS");
        flash_initialized = false;
    } else {
        flash_initialized = true;
        Serial.println("SPIFFS initialized");
    }

    // Try to initialize SD card
    // Note: Update SD_CARD_CS_PIN in hardware_config.h when schematic is available
    // if (SD.begin(SD_CARD_CS_PIN)) {
    //     sd_initialized = true;
    //     Serial.println("SD card initialized");
    // } else {
    //     Serial.println("SD card not detected");
    //     sd_initialized = false;
    // }

    return flash_initialized || sd_initialized;
}

bool StorageHAL::is_sd_available() {
    return sd_initialized;
}

StorageType StorageHAL::get_preferred_storage() {
    return sd_initialized ? StorageType::SD_CARD : StorageType::INTERNAL_FLASH;
}

fs::FS& StorageHAL::get_fs(StorageType type) {
    if (type == StorageType::SD_CARD && sd_initialized) {
        return SD;
    }
    return SPIFFS;
}

uint64_t StorageHAL::get_free_space(StorageType type) {
    if (type == StorageType::INTERNAL_FLASH && flash_initialized) {
        return SPIFFS.totalBytes() - SPIFFS.usedBytes();
    } else if (type == StorageType::SD_CARD && sd_initialized) {
        return SD.totalBytes() - SD.usedBytes();
    }
    return 0;
}

uint64_t StorageHAL::get_total_space(StorageType type) {
    if (type == StorageType::INTERNAL_FLASH && flash_initialized) {
        return SPIFFS.totalBytes();
    } else if (type == StorageType::SD_CARD && sd_initialized) {
        return SD.totalBytes();
    }
    return 0;
}

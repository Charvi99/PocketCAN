#include "can_hal.h"
#include <Arduino.h>
#include "../config/hardware_config.h"

CANBaudRate CANHAL::current_baudrate = DEFAULT_CAN_BAUD;
CANStats CANHAL::stats = {0};
bool CANHAL::initialized = false;
bool CANHAL::running = false;

twai_timing_config_t CANHAL::get_timing_config(CANBaudRate baudrate) {
    switch (baudrate) {
        case CANBaudRate::BAUD_1M:
            return TWAI_TIMING_CONFIG_1MBITS();
        case CANBaudRate::BAUD_800K:
            return TWAI_TIMING_CONFIG_800KBITS();
        case CANBaudRate::BAUD_500K:
            return TWAI_TIMING_CONFIG_500KBITS();
        case CANBaudRate::BAUD_250K:
            return TWAI_TIMING_CONFIG_250KBITS();
        case CANBaudRate::BAUD_125K:
            return TWAI_TIMING_CONFIG_125KBITS();
        case CANBaudRate::BAUD_100K:
            return TWAI_TIMING_CONFIG_100KBITS();
        case CANBaudRate::BAUD_50K:
            return TWAI_TIMING_CONFIG_50KBITS();
        case CANBaudRate::BAUD_20K:
            return TWAI_TIMING_CONFIG_25KBITS();
        default:
            return TWAI_TIMING_CONFIG_500KBITS();
    }
}

bool CANHAL::init(CANBaudRate baudrate) {
    if (initialized) {
        Serial.println("CAN HAL already initialized");
        return true;
    }

    // Configure CAN timing
    twai_timing_config_t t_config = get_timing_config(baudrate);

    // Configure CAN filter (accept all messages)
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Configure CAN general settings
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        CAN_TX_PIN,
        CAN_RX_PIN,
        TWAI_MODE_NORMAL
    );
    g_config.rx_queue_len = CAN_RX_BUFFER_SIZE;
    g_config.tx_queue_len = CAN_TX_QUEUE_SIZE;

    // Install TWAI driver
    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        Serial.printf("ERROR: Failed to install TWAI driver: %d\n", err);
        return false;
    }

    current_baudrate = baudrate;
    initialized = true;
    reset_stats();

    Serial.printf("CAN HAL initialized at %d baud\n", static_cast<uint32_t>(baudrate));
    return true;
}

bool CANHAL::start() {
    if (!initialized) {
        Serial.println("ERROR: CAN HAL not initialized");
        return false;
    }

    if (running) {
        Serial.println("CAN already running");
        return true;
    }

    esp_err_t err = twai_start();
    if (err != ESP_OK) {
        Serial.printf("ERROR: Failed to start TWAI: %d\n", err);
        return false;
    }

    running = true;
    Serial.println("CAN bus started");
    return true;
}

bool CANHAL::stop() {
    if (!running) {
        return true;
    }

    esp_err_t err = twai_stop();
    if (err != ESP_OK) {
        Serial.printf("ERROR: Failed to stop TWAI: %d\n", err);
        return false;
    }

    running = false;
    Serial.println("CAN bus stopped");
    return true;
}

bool CANHAL::set_baudrate(CANBaudRate baudrate) {
    if (running) {
        stop();
    }

    if (initialized) {
        twai_driver_uninstall();
        initialized = false;
    }

    return init(baudrate);
}

bool CANHAL::transmit(const CANMessage& msg) {
    if (!running) {
        return false;
    }

    twai_message_t twai_msg;
    twai_msg.identifier = msg.id;
    twai_msg.data_length_code = msg.dlc;
    twai_msg.rtr = msg.rtr;
    twai_msg.extd = (msg.type == CANFrameType::EXTENDED);
    memcpy(twai_msg.data, msg.data, msg.dlc);

    esp_err_t err = twai_transmit(&twai_msg, pdMS_TO_TICKS(10));
    if (err == ESP_OK) {
        stats.tx_count++;
        return true;
    }

    return false;
}

bool CANHAL::receive(CANMessage& msg) {
    if (!running) {
        return false;
    }

    twai_message_t twai_msg;
    esp_err_t err = twai_receive(&twai_msg, 0);  // Non-blocking

    if (err == ESP_OK) {
        msg.id = twai_msg.identifier;
        msg.dlc = twai_msg.data_length_code;
        msg.rtr = twai_msg.rtr;
        msg.type = twai_msg.extd ? CANFrameType::EXTENDED : CANFrameType::STANDARD;
        msg.timestamp_ms = millis();
        memcpy(msg.data, twai_msg.data, twai_msg.data_length_code);

        stats.rx_count++;
        return true;
    }

    return false;
}

uint32_t CANHAL::available() {
    if (!running) {
        return 0;
    }

    twai_status_info_t status;
    twai_get_status_info(&status);
    return status.msgs_to_rx;
}

CANStats CANHAL::get_stats() {
    if (running) {
        twai_status_info_t status;
        twai_get_status_info(&status);
        stats.error_count = status.tx_error_counter + status.rx_error_counter;
        stats.bus_off_count = status.bus_error_count;
    }
    return stats;
}

void CANHAL::reset_stats() {
    stats.rx_count = 0;
    stats.tx_count = 0;
    stats.error_count = 0;
    stats.bus_off_count = 0;
    stats.bus_load_percent = 0.0f;
}

bool CANHAL::is_bus_error() {
    if (!running) {
        return false;
    }

    twai_status_info_t status;
    twai_get_status_info(&status);
    return (status.state == TWAI_STATE_BUS_OFF ||
            status.state == TWAI_STATE_RECOVERING);
}

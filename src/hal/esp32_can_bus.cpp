#include "esp32_can_bus.h"

#include <Arduino.h>
#include <cstring>

#include "../config/hardware_config.h"

twai_timing_config_t Esp32CanBus::timing_config_for(CanTimingId id) {
    // The only place ESP-IDF register values appear. The enum-to-rate mapping
    // that used to live here is now in core/can_timing.h, under test.
    switch (id) {
        case CanTimingId::T_10K:  return TWAI_TIMING_CONFIG_10KBITS();
        case CanTimingId::T_20K:  return TWAI_TIMING_CONFIG_20KBITS();
        case CanTimingId::T_50K:  return TWAI_TIMING_CONFIG_50KBITS();
        case CanTimingId::T_100K: return TWAI_TIMING_CONFIG_100KBITS();
        case CanTimingId::T_125K: return TWAI_TIMING_CONFIG_125KBITS();
        case CanTimingId::T_250K: return TWAI_TIMING_CONFIG_250KBITS();
        case CanTimingId::T_500K: return TWAI_TIMING_CONFIG_500KBITS();
        case CanTimingId::T_800K: return TWAI_TIMING_CONFIG_800KBITS();
        case CanTimingId::T_1M:   return TWAI_TIMING_CONFIG_1MBITS();
    }
    return TWAI_TIMING_CONFIG_500KBITS();
}

bool Esp32CanBus::init(CANBaudRate baudrate) {
    if (initialized) {
        return true;
    }

    twai_timing_config_t t_config = timing_config_for(can_timing_for(baudrate).id);
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    g_config.rx_queue_len = CAN_RX_BUFFER_SIZE;
    g_config.tx_queue_len = CAN_TX_QUEUE_SIZE;

    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        Serial.printf("ERROR: twai_driver_install failed: %d\n", err);
        return false;
    }

    current_baudrate = baudrate;
    initialized = true;
    reset_stats();

    Serial.printf("CAN initialized at %u baud\n",
                  static_cast<unsigned>(can_timing_for(baudrate).nominal_bps));
    return true;
}

bool Esp32CanBus::start() {
    if (!initialized) {
        return false;
    }
    if (running) {
        return true;
    }

    esp_err_t err = twai_start();
    if (err != ESP_OK) {
        Serial.printf("ERROR: twai_start failed: %d\n", err);
        return false;
    }

    running = true;
    return true;
}

bool Esp32CanBus::stop() {
    if (!running) {
        return true;
    }
    if (twai_stop() != ESP_OK) {
        return false;
    }
    running = false;
    return true;
}

bool Esp32CanBus::set_baudrate(CANBaudRate baudrate) {
    if (running) {
        stop();
    }
    if (initialized) {
        twai_driver_uninstall();
        initialized = false;
    }
    return init(baudrate);
}

bool Esp32CanBus::transmit(const CANMessage& msg) {
    if (!running) {
        return false;
    }

    // Zero-initialized: the flag bits ss, self, and dlc_non_comp share this
    // struct. Leaving them as stack residue transmits single-shot or
    // self-reception frames by accident.
    twai_message_t twai_msg = {};
    twai_msg.identifier       = msg.id;
    twai_msg.data_length_code = msg.dlc;
    twai_msg.rtr              = msg.rtr;
    twai_msg.extd             = (msg.type == CANFrameType::EXTENDED);
    if (msg.dlc <= 8) {
        std::memcpy(twai_msg.data, msg.data, msg.dlc);
    }

    if (twai_transmit(&twai_msg, pdMS_TO_TICKS(10)) != ESP_OK) {
        return false;
    }

    stats.tx_count++;
    return true;
}

bool Esp32CanBus::receive(CANMessage& msg) {
    if (!running) {
        return false;
    }

    twai_message_t twai_msg = {};
    if (twai_receive(&twai_msg, 0) != ESP_OK) {   // non-blocking
        return false;
    }

    msg.id           = twai_msg.identifier;
    msg.dlc          = twai_msg.data_length_code > 8 ? 8 : twai_msg.data_length_code;
    msg.rtr          = twai_msg.rtr;
    msg.type         = twai_msg.extd ? CANFrameType::EXTENDED : CANFrameType::STANDARD;
    msg.timestamp_ms = millis();

    // Zero the tail so the sniffer table never shows stack residue as bus data.
    std::memset(msg.data, 0, sizeof(msg.data));
    std::memcpy(msg.data, twai_msg.data, msg.dlc);

    stats.rx_count++;
    return true;
}

uint32_t Esp32CanBus::available() const {
    if (!running) {
        return 0;
    }
    twai_status_info_t status;
    twai_get_status_info(&status);
    return status.msgs_to_rx;
}

CANStats Esp32CanBus::get_stats() const {
    if (running) {
        twai_status_info_t status;
        twai_get_status_info(&status);
        stats.error_count = status.tx_error_counter + status.rx_error_counter;
        // bus_off_count is incremented by recover(), not read from the driver.
        // status.bus_error_count counts error frames, which is a different
        // thing entirely - conflating them was the original bug.
    }
    return stats;
}

void Esp32CanBus::reset_stats() {
    stats = {};
}

bool Esp32CanBus::is_bus_error() const {
    if (!running) {
        return false;
    }
    twai_status_info_t status;
    twai_get_status_info(&status);
    return status.state == TWAI_STATE_BUS_OFF;
}

bool Esp32CanBus::recover() {
    twai_status_info_t status;
    twai_get_status_info(&status);

    if (status.state == TWAI_STATE_RECOVERING) {
        return true;   // already under way
    }
    if (status.state != TWAI_STATE_BUS_OFF) {
        return false;  // nothing to recover from
    }

    if (twai_initiate_recovery() != ESP_OK) {
        return false;
    }

    stats.bus_off_count++;
    running = false;   // the driver returns to STOPPED once recovery completes
    return true;
}

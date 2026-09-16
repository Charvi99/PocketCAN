#pragma once

/**
 * Transmit Screen
 * Configure and send CAN messages (single-shot and periodic)
 */

#include "lvgl.h"
#include "../../core/can_transmitter.h"
#include <vector>

// Message being edited
struct TransmitMessage {
    uint32_t id;
    CANFrameType type;
    uint8_t dlc;
    uint8_t data[8];
    uint32_t interval_ms;  // For periodic transmission
    int periodic_index;    // Index in transmitter (-1 if not periodic)
};

class ScreenTransmit {
public:
    /**
     * Create transmit screen
     * @return Screen object
     */
    static lv_obj_t* create();

    /**
     * Update transmit screen
     * @param screen Screen object
     */
    static void update(lv_obj_t* screen);

    /**
     * Cleanup when screen is destroyed
     */
    static void cleanup();

private:
    // UI Components
    static lv_obj_t* message_table;
    static lv_obj_t* message_dialog;
    static lv_obj_t* keyboard;

    // Dialog input fields
    static lv_obj_t* id_input;
    static lv_obj_t* dlc_dropdown;
    static lv_obj_t* type_dropdown;
    static lv_obj_t* data_inputs[8];
    static lv_obj_t* interval_input;

    // State
    static std::vector<TransmitMessage> configured_messages;
    static int editing_index;  // -1 for new message, >= 0 for editing
    static TransmitMessage temp_message;
    static uint32_t last_table_update;

    // Layout creation
    static lv_obj_t* create_left_column(lv_obj_t* parent);
    static lv_obj_t* create_right_column(lv_obj_t* parent);
    static lv_obj_t* create_message_table(lv_obj_t* parent);
    static void update_message_table();

    // Dialog
    static void create_message_dialog(int edit_index);
    static void close_message_dialog();

    // Message management
    static void add_message(const TransmitMessage& msg);
    static void update_message(int index, const TransmitMessage& msg);
    static void delete_message(int index);
    static void clear_all_messages();
    static void fire_message_once(int index);
    static void toggle_periodic(int index);

    // Event callbacks
    static void add_button_cb(lv_event_t* e);
    static void clear_button_cb(lv_event_t* e);
    static void message_row_cb(lv_event_t* e);
    static void fire_once_cb(lv_event_t* e);
    static void fire_periodic_cb(lv_event_t* e);
    static void dialog_add_cb(lv_event_t* e);
    static void dialog_delete_cb(lv_event_t* e);
    static void dialog_cancel_cb(lv_event_t* e);
    static void dlc_changed_cb(lv_event_t* e);

    // Helpers
    static void load_message_to_dialog(const TransmitMessage& msg);
    static bool validate_and_save_from_dialog();
    static uint8_t parse_hex_byte(const char* str);
};

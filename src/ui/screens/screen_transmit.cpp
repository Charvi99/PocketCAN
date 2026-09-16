#include "screen_transmit.h"
#include "../widgets/status_bar.h"
#include "../themes/theme_colors.h"
#include "../../config/app_config.h"
#include "../ui_manager.h"
#include <Arduino.h>

// Static member initialization
lv_obj_t* ScreenTransmit::message_table = nullptr;
lv_obj_t* ScreenTransmit::message_dialog = nullptr;
lv_obj_t* ScreenTransmit::keyboard = nullptr;
lv_obj_t* ScreenTransmit::id_input = nullptr;
lv_obj_t* ScreenTransmit::dlc_dropdown = nullptr;
lv_obj_t* ScreenTransmit::type_dropdown = nullptr;
lv_obj_t* ScreenTransmit::data_inputs[8] = {nullptr};
lv_obj_t* ScreenTransmit::interval_input = nullptr;
std::vector<TransmitMessage> ScreenTransmit::configured_messages;
int ScreenTransmit::editing_index = -1;
TransmitMessage ScreenTransmit::temp_message;
uint32_t ScreenTransmit::last_table_update = 0;

extern CANTransmitter transmitter;

lv_obj_t* ScreenTransmit::create() {
    Serial.println("Creating Transmit screen...");

    // Create main screen
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    // Create content container (below status bar)
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_height(content, 720 - STATUS_BAR_HEIGHT);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_color(content, lv_color_hex(THEME_COLOR_BACKGROUND), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 10, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Create main container with horizontal layout
    lv_obj_t* main_container = lv_obj_create(content);
    lv_obj_set_size(main_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(main_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(main_container, 0, 0);
    lv_obj_set_style_pad_all(main_container, 0, 0);
    lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(main_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(main_container, LV_OBJ_FLAG_SCROLLABLE);

    // Create left column (message table) - 75% width
    lv_obj_t* left_col = create_left_column(main_container);
    lv_obj_set_flex_grow(left_col, 3);

    // Create right column (buttons) - 25% width
    lv_obj_t* right_col = create_right_column(main_container);
    lv_obj_set_flex_grow(right_col, 1);

    Serial.println("Transmit screen created");
    return screen;
}

lv_obj_t* ScreenTransmit::create_left_column(lv_obj_t* parent) {
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_height(container, LV_PCT(100));
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 5, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* title = lv_label_create(container);
    lv_label_set_text(title, LV_SYMBOL_UPLOAD " CAN Transmitter");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_pad_bottom(title, 10, 0);

    // Message table (scrollable)
    message_table = create_message_table(container);
    lv_obj_set_flex_grow(message_table, 1);

    return container;
}

lv_obj_t* ScreenTransmit::create_right_column(lv_obj_t* parent) {
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_height(container, LV_PCT(100));
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 5, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(container, 10, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // Add button
    lv_obj_t* add_btn = lv_btn_create(container);
    lv_obj_set_size(add_btn, LV_PCT(100), 80);
    lv_obj_set_style_bg_color(add_btn, lv_color_hex(THEME_COLOR_SUCCESS), 0);
    lv_obj_set_style_radius(add_btn, 8, 0);
    lv_obj_add_event_cb(add_btn, add_button_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* add_label = lv_label_create(add_btn);
    lv_label_set_text(add_label, LV_SYMBOL_PLUS " Add");
    lv_obj_set_style_text_font(add_label, &lv_font_montserrat_24, 0);
    lv_obj_center(add_label);

    // Clear button
    lv_obj_t* clear_btn = lv_btn_create(container);
    lv_obj_set_size(clear_btn, LV_PCT(100), 80);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(THEME_COLOR_ERROR), 0);
    lv_obj_set_style_radius(clear_btn, 8, 0);
    lv_obj_add_event_cb(clear_btn, clear_button_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, LV_SYMBOL_TRASH " Clear");
    lv_obj_set_style_text_font(clear_label, &lv_font_montserrat_24, 0);
    lv_obj_center(clear_label);

    return container;
}

lv_obj_t* ScreenTransmit::create_message_table(lv_obj_t* parent) {
    // Scrollable container
    lv_obj_t* scroll_cont = lv_obj_create(parent);
    lv_obj_set_width(scroll_cont, LV_PCT(100));
    lv_obj_set_style_bg_color(scroll_cont, lv_color_hex(THEME_COLOR_SURFACE), 0);
    lv_obj_set_style_border_width(scroll_cont, 1, 0);
    lv_obj_set_style_border_color(scroll_cont, lv_color_hex(THEME_COLOR_SURFACE_VARIANT), 0);
    lv_obj_set_style_radius(scroll_cont, 8, 0);
    lv_obj_set_style_pad_all(scroll_cont, 10, 0);
    lv_obj_set_flex_flow(scroll_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(scroll_cont, 8, 0);
    lv_obj_set_scroll_dir(scroll_cont, LV_DIR_VER);

    return scroll_cont;
}

void ScreenTransmit::update_message_table() {
    if (!message_table) return;

    // Clear existing content
    lv_obj_clean(message_table);

    if (configured_messages.empty()) {
        // Show empty state
        lv_obj_t* empty_label = lv_label_create(message_table);
        lv_label_set_text(empty_label, "No messages configured.\nClick '+ Add' to create a message.");
        lv_obj_set_style_text_color(empty_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
        lv_obj_set_style_text_align(empty_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(empty_label);
        return;
    }

    // Create row for each message
    for (size_t i = 0; i < configured_messages.size(); i++) {
        const TransmitMessage& msg = configured_messages[i];

        // Message row container
        lv_obj_t* row = lv_obj_create(message_table);
        lv_obj_set_width(row, LV_PCT(100));
        lv_obj_set_height(row, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(row, lv_color_hex(THEME_COLOR_BACKGROUND), 0);
        lv_obj_set_style_radius(row, 6, 0);
        lv_obj_set_style_pad_all(row, 8, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(row, message_row_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);

        // Flex layout: vertical
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(row, 5, 0);

        // Top section: Message info (clickable area)
        lv_obj_t* info_cont = lv_obj_create(row);
        lv_obj_set_width(info_cont, LV_PCT(100));
        lv_obj_set_height(info_cont, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(info_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(info_cont, 0, 0);
        lv_obj_set_style_pad_all(info_cont, 0, 0);
        lv_obj_clear_flag(info_cont, LV_OBJ_FLAG_CLICKABLE);

        // Message info text
        char info_buf[128];
        char data_str[32] = "";
        for (int j = 0; j < msg.dlc; j++) {
            char byte_str[4];
            snprintf(byte_str, sizeof(byte_str), "%02X ", msg.data[j]);
            strcat(data_str, byte_str);
        }

        snprintf(info_buf, sizeof(info_buf), "ID: 0x%03X [%s] | DLC: %d\nData: %s",
                 msg.id,
                 msg.type == CANFrameType::STANDARD ? "STD" : "EXT",
                 msg.dlc,
                 data_str);

        lv_obj_t* info_label = lv_label_create(info_cont);
        lv_label_set_text(info_label, info_buf);
        lv_obj_set_style_text_font(info_label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(info_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
        lv_obj_clear_flag(info_label, LV_OBJ_FLAG_CLICKABLE);

        // Bottom section: Action buttons
        lv_obj_t* btn_cont = lv_obj_create(row);
        lv_obj_set_width(btn_cont, LV_PCT(100));
        lv_obj_set_height(btn_cont, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn_cont, 0, 0);
        lv_obj_set_style_pad_all(btn_cont, 0, 0);
        lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_CLICKABLE);

        // Fire Once button
        lv_obj_t* fire_btn = lv_btn_create(btn_cont);
        lv_obj_set_size(fire_btn, 180, 40);
        lv_obj_set_style_bg_color(fire_btn, lv_color_hex(THEME_COLOR_PRIMARY), 0);
        lv_obj_set_style_radius(fire_btn, 4, 0);
        lv_obj_add_event_cb(fire_btn, fire_once_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);

        lv_obj_t* fire_label = lv_label_create(fire_btn);
        lv_label_set_text(fire_label, LV_SYMBOL_PLAY " Fire Once");
        lv_obj_set_style_text_font(fire_label, &lv_font_montserrat_14, 0);
        lv_obj_center(fire_label);

        // Periodic button
        lv_obj_t* periodic_btn = lv_btn_create(btn_cont);
        lv_obj_set_size(periodic_btn, 180, 40);

        bool is_periodic = (msg.periodic_index >= 0);
        if (is_periodic) {
            lv_obj_set_style_bg_color(periodic_btn, lv_color_hex(THEME_COLOR_WARNING), 0);
        } else {
            lv_obj_set_style_bg_color(periodic_btn, lv_color_hex(THEME_COLOR_SURFACE_VARIANT), 0);
        }
        lv_obj_set_style_radius(periodic_btn, 4, 0);
        lv_obj_add_event_cb(periodic_btn, fire_periodic_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);

        lv_obj_t* periodic_label = lv_label_create(periodic_btn);
        if (is_periodic) {
            char periodic_text[32];
            snprintf(periodic_text, sizeof(periodic_text), LV_SYMBOL_LOOP " %lums", msg.interval_ms);
            lv_label_set_text(periodic_label, periodic_text);
        } else {
            lv_label_set_text(periodic_label, LV_SYMBOL_LOOP " Periodic");
        }
        lv_obj_set_style_text_font(periodic_label, &lv_font_montserrat_14, 0);
        lv_obj_center(periodic_label);
    }
}

void ScreenTransmit::create_message_dialog(int edit_index) {
    editing_index = edit_index;

    // Create modal background
    message_dialog = lv_obj_create(lv_layer_top());
    lv_obj_set_size(message_dialog, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(message_dialog, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(message_dialog, LV_OPA_50, 0);
    lv_obj_set_style_border_width(message_dialog, 0, 0);
    lv_obj_clear_flag(message_dialog, LV_OBJ_FLAG_SCROLLABLE);

    // Dialog box
    lv_obj_t* dialog_box = lv_obj_create(message_dialog);
    lv_obj_set_size(dialog_box, 700, 580);
    lv_obj_center(dialog_box);
    lv_obj_set_style_bg_color(dialog_box, lv_color_hex(THEME_COLOR_SURFACE), 0);
    lv_obj_set_style_radius(dialog_box, 12, 0);
    lv_obj_set_style_pad_all(dialog_box, 20, 0);
    lv_obj_set_flex_flow(dialog_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(dialog_box, LV_DIR_VER);

    // Title
    lv_obj_t* title = lv_label_create(dialog_box);
    lv_label_set_text(title, edit_index >= 0 ? "Edit Message" : "New Message");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_pad_bottom(title, 15, 0);

    // Form container
    lv_obj_t* form = lv_obj_create(dialog_box);
    lv_obj_set_width(form, LV_PCT(100));
    lv_obj_set_height(form, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(form, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(form, 0, 0);
    lv_obj_set_style_pad_all(form, 0, 0);
    lv_obj_set_flex_flow(form, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(form, 10, 0);

    // CAN ID input
    lv_obj_t* id_label = lv_label_create(form);
    lv_label_set_text(id_label, "CAN ID (hex):");
    lv_obj_set_style_text_color(id_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    id_input = lv_textarea_create(form);
    lv_obj_set_width(id_input, LV_PCT(100));
    lv_textarea_set_one_line(id_input, true);
    lv_textarea_set_max_length(id_input, 8);
    lv_textarea_set_placeholder_text(id_input, "e.g., 123 or 18DA10F1");

    // Type dropdown
    lv_obj_t* type_label = lv_label_create(form);
    lv_label_set_text(type_label, "Frame Type:");
    lv_obj_set_style_text_color(type_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    type_dropdown = lv_dropdown_create(form);
    lv_dropdown_set_options(type_dropdown, "Standard (11-bit)\nExtended (29-bit)");
    lv_obj_set_width(type_dropdown, LV_PCT(100));

    // DLC dropdown
    lv_obj_t* dlc_label = lv_label_create(form);
    lv_label_set_text(dlc_label, "DLC (Data Length):");
    lv_obj_set_style_text_color(dlc_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    dlc_dropdown = lv_dropdown_create(form);
    lv_dropdown_set_options(dlc_dropdown, "0\n1\n2\n3\n4\n5\n6\n7\n8");
    lv_dropdown_set_selected(dlc_dropdown, 8);
    lv_obj_set_width(dlc_dropdown, LV_PCT(100));
    lv_obj_add_event_cb(dlc_dropdown, dlc_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Data bytes (8 inputs in 2 rows)
    lv_obj_t* data_label = lv_label_create(form);
    lv_label_set_text(data_label, "Data Bytes (hex):");
    lv_obj_set_style_text_color(data_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    for (int row = 0; row < 2; row++) {
        lv_obj_t* byte_row = lv_obj_create(form);
        lv_obj_set_width(byte_row, LV_PCT(100));
        lv_obj_set_height(byte_row, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(byte_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(byte_row, 0, 0);
        lv_obj_set_style_pad_all(byte_row, 0, 0);
        lv_obj_set_flex_flow(byte_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(byte_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        for (int col = 0; col < 4; col++) {
            int index = row * 4 + col;
            data_inputs[index] = lv_textarea_create(byte_row);
            lv_obj_set_size(data_inputs[index], 70, 40);
            lv_textarea_set_one_line(data_inputs[index], true);
            lv_textarea_set_max_length(data_inputs[index], 2);
            lv_textarea_set_placeholder_text(data_inputs[index], "00");
            lv_obj_set_style_text_align(data_inputs[index], LV_TEXT_ALIGN_CENTER, 0);
        }
    }

    // Interval input (for periodic)
    lv_obj_t* interval_label = lv_label_create(form);
    lv_label_set_text(interval_label, "Periodic Interval (ms):");
    lv_obj_set_style_text_color(interval_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    interval_input = lv_textarea_create(form);
    lv_obj_set_width(interval_input, LV_PCT(100));
    lv_textarea_set_one_line(interval_input, true);
    lv_textarea_set_max_length(interval_input, 6);
    lv_textarea_set_placeholder_text(interval_input, "100 (0 = fire once only)");

    // Button row
    lv_obj_t* btn_row = lv_obj_create(dialog_box);
    lv_obj_set_width(btn_row, LV_PCT(100));
    lv_obj_set_height(btn_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(btn_row, 10, 0);

    // Cancel button
    lv_obj_t* cancel_btn = lv_btn_create(btn_row);
    lv_obj_set_size(cancel_btn, 150, 50);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(THEME_COLOR_SURFACE_VARIANT), 0);
    lv_obj_add_event_cb(cancel_btn, dialog_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, LV_SYMBOL_CLOSE " Cancel");
    lv_obj_center(cancel_label);

    // Delete button (only for editing)
    if (edit_index >= 0) {
        lv_obj_t* delete_btn = lv_btn_create(btn_row);
        lv_obj_set_size(delete_btn, 150, 50);
        lv_obj_set_style_bg_color(delete_btn, lv_color_hex(THEME_COLOR_ERROR), 0);
        lv_obj_add_event_cb(delete_btn, dialog_delete_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t* delete_label = lv_label_create(delete_btn);
        lv_label_set_text(delete_label, LV_SYMBOL_TRASH " Delete");
        lv_obj_center(delete_label);
    }

    // Add/Save button
    lv_obj_t* add_btn = lv_btn_create(btn_row);
    lv_obj_set_size(add_btn, 150, 50);
    lv_obj_set_style_bg_color(add_btn, lv_color_hex(THEME_COLOR_SUCCESS), 0);
    lv_obj_add_event_cb(add_btn, dialog_add_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* add_label = lv_label_create(add_btn);
    lv_label_set_text(add_label, edit_index >= 0 ? LV_SYMBOL_SAVE " Save" : LV_SYMBOL_PLUS " Add");
    lv_obj_center(add_label);

    // Load existing message data if editing
    if (edit_index >= 0 && edit_index < (int)configured_messages.size()) {
        load_message_to_dialog(configured_messages[edit_index]);
    } else {
        // Set defaults for new message
        temp_message.id = 0x123;
        temp_message.type = CANFrameType::STANDARD;
        temp_message.dlc = 8;
        memset(temp_message.data, 0, 8);
        temp_message.interval_ms = 0;
        temp_message.periodic_index = -1;
        load_message_to_dialog(temp_message);
    }
}

void ScreenTransmit::load_message_to_dialog(const TransmitMessage& msg) {
    // Set ID
    char id_str[16];
    snprintf(id_str, sizeof(id_str), "%X", msg.id);
    lv_textarea_set_text(id_input, id_str);

    // Set type
    lv_dropdown_set_selected(type_dropdown, msg.type == CANFrameType::STANDARD ? 0 : 1);

    // Set DLC
    lv_dropdown_set_selected(dlc_dropdown, msg.dlc);

    // Set data bytes
    for (int i = 0; i < 8; i++) {
        char byte_str[4];
        snprintf(byte_str, sizeof(byte_str), "%02X", msg.data[i]);
        lv_textarea_set_text(data_inputs[i], byte_str);
    }

    // Set interval
    char interval_str[16];
    snprintf(interval_str, sizeof(interval_str), "%lu", msg.interval_ms);
    lv_textarea_set_text(interval_input, interval_str);
}

bool ScreenTransmit::validate_and_save_from_dialog() {
    // Parse ID
    const char* id_text = lv_textarea_get_text(id_input);
    temp_message.id = strtoul(id_text, NULL, 16);

    // Get type
    temp_message.type = (lv_dropdown_get_selected(type_dropdown) == 0) ?
                        CANFrameType::STANDARD : CANFrameType::EXTENDED;

    // Validate ID range
    if (temp_message.type == CANFrameType::STANDARD && temp_message.id > 0x7FF) {
        Serial.println("ERROR: Standard ID must be <= 0x7FF");
        UIManager::show_notification("Error: Standard ID must be <= 0x7FF");
        return false;
    }
    if (temp_message.type == CANFrameType::EXTENDED && temp_message.id > 0x1FFFFFFF) {
        Serial.println("ERROR: Extended ID must be <= 0x1FFFFFFF");
        UIManager::show_notification("Error: Extended ID must be <= 0x1FFFFFFF");
        return false;
    }

    // Get DLC
    temp_message.dlc = lv_dropdown_get_selected(dlc_dropdown);

    // Parse data bytes
    for (int i = 0; i < 8; i++) {
        const char* byte_text = lv_textarea_get_text(data_inputs[i]);
        temp_message.data[i] = parse_hex_byte(byte_text);
    }

    // Parse interval
    const char* interval_text = lv_textarea_get_text(interval_input);
    temp_message.interval_ms = atoi(interval_text);

    return true;
}

uint8_t ScreenTransmit::parse_hex_byte(const char* str) {
    if (!str || strlen(str) == 0) return 0;
    return (uint8_t)strtoul(str, NULL, 16);
}

void ScreenTransmit::close_message_dialog() {
    if (message_dialog) {
        lv_obj_del(message_dialog);
        message_dialog = nullptr;
        id_input = nullptr;
        dlc_dropdown = nullptr;
        type_dropdown = nullptr;
        interval_input = nullptr;
        for (int i = 0; i < 8; i++) {
            data_inputs[i] = nullptr;
        }
    }
}

void ScreenTransmit::add_message(const TransmitMessage& msg) {
    configured_messages.push_back(msg);
    update_message_table();
    Serial.printf("Message added: ID=0x%X, DLC=%d\n", msg.id, msg.dlc);
}

void ScreenTransmit::update_message(int index, const TransmitMessage& msg) {
    if (index >= 0 && index < (int)configured_messages.size()) {
        // If it was periodic, remove from transmitter
        if (configured_messages[index].periodic_index >= 0) {
            transmitter.remove_periodic(configured_messages[index].periodic_index);
        }

        configured_messages[index] = msg;
        update_message_table();
        Serial.printf("Message updated: index=%d, ID=0x%X\n", index, msg.id);
    }
}

void ScreenTransmit::delete_message(int index) {
    if (index >= 0 && index < (int)configured_messages.size()) {
        // If it was periodic, remove from transmitter
        if (configured_messages[index].periodic_index >= 0) {
            transmitter.remove_periodic(configured_messages[index].periodic_index);
        }

        configured_messages.erase(configured_messages.begin() + index);
        update_message_table();
        Serial.printf("Message deleted: index=%d\n", index);
    }
}

void ScreenTransmit::clear_all_messages() {
    transmitter.clear_periodic();
    configured_messages.clear();
    update_message_table();
    Serial.println("All messages cleared");
}

void ScreenTransmit::fire_message_once(int index) {
    if (index < 0 || index >= (int)configured_messages.size()) return;

    const TransmitMessage& msg = configured_messages[index];

    CANMessage can_msg;
    can_msg.id = msg.id;
    can_msg.type = msg.type;
    can_msg.dlc = msg.dlc;
    memcpy(can_msg.data, msg.data, 8);
    can_msg.rtr = false;
    can_msg.timestamp_ms = millis();

    if (transmitter.send(can_msg)) {
        Serial.printf("Message sent: ID=0x%X\n", msg.id);
        UIManager::show_notification("Message sent!");
    } else {
        Serial.println("ERROR: Failed to send message");
        UIManager::show_notification("Error: Failed to send");
    }
}

void ScreenTransmit::toggle_periodic(int index) {
    if (index < 0 || index >= (int)configured_messages.size()) return;

    TransmitMessage& msg = configured_messages[index];

    if (msg.periodic_index >= 0) {
        // Stop periodic transmission
        transmitter.remove_periodic(msg.periodic_index);
        msg.periodic_index = -1;
        Serial.printf("Periodic transmission stopped: ID=0x%X\n", msg.id);
        UIManager::show_notification("Periodic stopped");
    } else {
        // Start periodic transmission
        if (msg.interval_ms == 0) {
            UIManager::show_notification("Error: Interval is 0");
            return;
        }

        CANMessage can_msg;
        can_msg.id = msg.id;
        can_msg.type = msg.type;
        can_msg.dlc = msg.dlc;
        memcpy(can_msg.data, msg.data, 8);
        can_msg.rtr = false;
        can_msg.timestamp_ms = millis();

        int slot = transmitter.add_periodic(can_msg, msg.interval_ms);
        if (slot >= 0) {
            msg.periodic_index = slot;
            Serial.printf("Periodic transmission started: ID=0x%X, interval=%lums\n", msg.id, msg.interval_ms);
            UIManager::show_notification("Periodic started");
        } else {
            Serial.println("ERROR: Failed to add periodic message");
            UIManager::show_notification("Error: Periodic slots full");
        }
    }

    update_message_table();
}

// Event Callbacks

void ScreenTransmit::add_button_cb(lv_event_t* e) {
    Serial.println("Add button clicked");
    create_message_dialog(-1);  // -1 for new message
}

void ScreenTransmit::clear_button_cb(lv_event_t* e) {
    Serial.println("Clear button clicked");
    clear_all_messages();
    UIManager::show_notification("All messages cleared");
}

void ScreenTransmit::message_row_cb(lv_event_t* e) {
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    Serial.printf("Message row clicked: index=%d\n", index);
    create_message_dialog(index);
}

void ScreenTransmit::fire_once_cb(lv_event_t* e) {
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    Serial.printf("Fire once clicked: index=%d\n", index);
    fire_message_once(index);
}

void ScreenTransmit::fire_periodic_cb(lv_event_t* e) {
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    Serial.printf("Fire periodic clicked: index=%d\n", index);
    toggle_periodic(index);
}

void ScreenTransmit::dialog_add_cb(lv_event_t* e) {
    Serial.println("Dialog Add/Save clicked");

    if (!validate_and_save_from_dialog()) {
        return;  // Validation failed
    }

    if (editing_index >= 0) {
        // Update existing message
        update_message(editing_index, temp_message);
        UIManager::show_notification("Message updated");
    } else {
        // Add new message
        temp_message.periodic_index = -1;  // Not periodic initially
        add_message(temp_message);
        UIManager::show_notification("Message added");
    }

    close_message_dialog();
}

void ScreenTransmit::dialog_delete_cb(lv_event_t* e) {
    Serial.println("Dialog Delete clicked");

    if (editing_index >= 0) {
        delete_message(editing_index);
        UIManager::show_notification("Message deleted");
    }

    close_message_dialog();
}

void ScreenTransmit::dialog_cancel_cb(lv_event_t* e) {
    Serial.println("Dialog Cancel clicked");
    close_message_dialog();
}

void ScreenTransmit::dlc_changed_cb(lv_event_t* e) {
    // Optional: Could grey out unused byte inputs based on DLC
}

void ScreenTransmit::update(lv_obj_t* screen) {
    // Periodic table update to reflect periodic transmission status
    uint32_t now = millis();
    if (now - last_table_update >= 1000) {  // Update every second
        // Check if any periodic status changed
        bool needs_update = false;
        for (size_t i = 0; i < configured_messages.size(); i++) {
            if (configured_messages[i].periodic_index >= 0) {
                needs_update = true;
                break;
            }
        }

        if (needs_update) {
            update_message_table();
        }

        last_table_update = now;
    }
}

void ScreenTransmit::cleanup() {
    message_table = nullptr;
    message_dialog = nullptr;
    keyboard = nullptr;
    id_input = nullptr;
    dlc_dropdown = nullptr;
    type_dropdown = nullptr;
    interval_input = nullptr;
    for (int i = 0; i < 8; i++) {
        data_inputs[i] = nullptr;
    }
    editing_index = -1;
    last_table_update = 0;
}

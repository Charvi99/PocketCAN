#pragma once

/**
 * Sniffer Screen
 * Real-time CAN bus monitoring with filtering and logging capabilities
 */

#include "lvgl.h"
#include "../../core/can_sniffer.h"
#include "../../core/can_filter.h"
#include <map>

// Display modes
enum class SnifferDisplayMode {
    CHRONOLOGICAL,  // Show all messages in order (with message #)
    GROUPED_BY_ID   // Show one row per ID (with message count)
};

// Tracked message info for grouped mode
struct MessageTracker {
    CANMessage last_message;
    uint32_t count;
    uint32_t last_update_time;
};

class ScreenSniffer {
public:
    /**
     * Create sniffer screen
     * @return Screen object
     */
    static lv_obj_t* create();

    /**
     * Update sniffer screen (refresh message list, stats, etc.)
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
    static lv_obj_t* filter_indicator;
    static lv_obj_t* stats_label;
    static lv_obj_t* logging_switch;
    static lv_obj_t* display_mode_switch;
    static lv_obj_t* pause_button;
    static lv_obj_t* filter_dialog;
    static lv_obj_t* keyboard;
    static lv_obj_t* table_header;

    // State
    static bool continuous_logging_enabled;
    static uint32_t last_update_time;
    static uint32_t displayed_message_count;
    static SnifferDisplayMode display_mode;
    static std::map<uint32_t, MessageTracker> message_trackers;  // For grouped mode

    // Optimization state for chronological mode
    static uint16_t chronological_next_row;  // Next row to write (circular buffer)
    static uint32_t chronological_row_message_numbers[50];  // Track message# in each row

    // Optimization state for grouped mode
    static std::map<uint32_t, uint16_t> id_to_row_map;  // Maps CAN ID to table row

    // Layout creation
    static lv_obj_t* create_left_column(lv_obj_t* parent);
    static lv_obj_t* create_right_column(lv_obj_t* parent);
    static lv_obj_t* create_message_table(lv_obj_t* parent);
    static lv_obj_t* create_header_row(lv_obj_t* parent);
    static lv_obj_t* create_filter_indicator(lv_obj_t* parent);
    static lv_obj_t* create_stats_row(lv_obj_t* parent);

    // Dialogs
    static void create_filter_dialog();
    static void close_filter_dialog();
    static void create_export_dialog();
    static void close_export_dialog();

    // Message list management
    static void update_message_table();
    static void update_chronological_table();
    static void update_grouped_table();
    static void rebuild_table_header();
    static void add_message_to_table(const CANMessage& msg, uint16_t row);
    static void add_message_to_table_with_number(const CANMessage& msg, uint16_t row, uint32_t msg_num);
    static void add_grouped_message_to_table(uint32_t id, const MessageTracker& tracker, uint16_t row);
    static void format_message_id(char* buf, uint32_t id, CANFrameType type);
    static void format_message_data(char* buf, const uint8_t* data, uint8_t dlc);
    static void format_timestamp(char* buf, uint32_t timestamp);

    // Filter management
    static void update_filter_indicator();
    static void apply_filter_from_dialog();
    static void clear_all_filters();

    // Logging functions
    static void toggle_continuous_logging();
    static void export_to_sd_card();
    static bool write_message_to_sd(const CANMessage& msg);
    static bool create_log_file();

    // Event callbacks
    static void home_button_cb(lv_event_t* e);
    static void filter_button_cb(lv_event_t* e);
    static void logging_switch_cb(lv_event_t* e);
    static void display_mode_switch_cb(lv_event_t* e);
    static void pause_button_cb(lv_event_t* e);
    static void clear_button_cb(lv_event_t* e);
    static void export_button_cb(lv_event_t* e);
    static void filter_apply_cb(lv_event_t* e);
    static void filter_clear_cb(lv_event_t* e);
    static void filter_close_cb(lv_event_t* e);
    static void export_confirm_cb(lv_event_t* e);
    static void export_cancel_cb(lv_event_t* e);
    static void table_scroll_cb(lv_event_t* e);

    // Helper functions
    static const char* get_log_filename();
    static uint32_t get_active_filter_count();
};

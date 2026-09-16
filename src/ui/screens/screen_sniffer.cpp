#include "screen_sniffer.h"
#include "../widgets/status_bar.h"
#include "../themes/theme_colors.h"
#include "../../config/app_config.h"
#include "../../config/hardware_config.h"
#include "../../services/state_manager.h"
#include "../../services/settings_manager.h"
#include "../../hal/storage_hal.h"
#include "../../utils/hex_utils.h"
#include "../ui_manager.h"
#include <Arduino.h>
#include <SD.h>
#include <set>

// Static member initialization
lv_obj_t* ScreenSniffer::message_table = nullptr;
lv_obj_t* ScreenSniffer::filter_indicator = nullptr;
lv_obj_t* ScreenSniffer::stats_label = nullptr;
lv_obj_t* ScreenSniffer::logging_switch = nullptr;
lv_obj_t* ScreenSniffer::display_mode_switch = nullptr;
lv_obj_t* ScreenSniffer::pause_button = nullptr;
lv_obj_t* ScreenSniffer::filter_dialog = nullptr;
lv_obj_t* ScreenSniffer::keyboard = nullptr;
lv_obj_t* ScreenSniffer::table_header = nullptr;
bool ScreenSniffer::continuous_logging_enabled = false;
uint32_t ScreenSniffer::last_update_time = 0;
uint32_t ScreenSniffer::displayed_message_count = 0;
SnifferDisplayMode ScreenSniffer::display_mode = SnifferDisplayMode::GROUPED_BY_ID;
std::map<uint32_t, MessageTracker> ScreenSniffer::message_trackers;

// Optimization state
uint16_t ScreenSniffer::chronological_next_row = 0;
uint32_t ScreenSniffer::chronological_row_message_numbers[50] = {0};
std::map<uint32_t, uint16_t> ScreenSniffer::id_to_row_map;

// Configuration
#define MAX_TABLE_ROWS 50           // Maximum messages displayed in table
#define COL_WIDTH_NUM 140            // Message number / count column
#define COL_WIDTH_TIMESTAMP 170
#define COL_WIDTH_ID 150
#define COL_WIDTH_DLC 60
#define COL_WIDTH_DATA 380

lv_obj_t* ScreenSniffer::create() {
    Serial.println("Creating Sniffer screen...");

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

    // Create two-column layout
    lv_obj_t* main_container = lv_obj_create(content);
    lv_obj_set_size(main_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(main_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(main_container, 0, 0);
    lv_obj_set_style_pad_all(main_container, 0, 0);
    lv_obj_clear_flag(main_container, LV_OBJ_FLAG_SCROLLABLE);

    // Set up grid layout (75% left, 25% right)
    static lv_coord_t col_dsc[] = {LV_GRID_FR(75), LV_GRID_FR(25), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(main_container, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(main_container, 10, 0);

    // Create left column (message list)
    lv_obj_t* left_col = create_left_column(main_container);
    lv_obj_set_grid_cell(left_col, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    // Create right column (buttons)
    lv_obj_t* right_col = create_right_column(main_container);
    lv_obj_set_grid_cell(right_col, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    // Reset state
    displayed_message_count = 0;
    last_update_time = millis();
    message_trackers.clear();
    id_to_row_map.clear();
    chronological_next_row = 0;
    for (int i = 0; i < MAX_TABLE_ROWS; i++) {
        chronological_row_message_numbers[i] = 0;
    }

    // Set initial state to STOPPED (not running)
    extern CANSniffer sniffer;
    sniffer.stop();
    Serial.println("Sniffer initial state: STOPPED");

    Serial.println("Sniffer screen created");
    return screen;
}

lv_obj_t* ScreenSniffer::create_left_column(lv_obj_t* parent) {
    // Create container for left column
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_style_bg_color(container, lv_color_hex(THEME_COLOR_SURFACE), 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_radius(container, 8, 0);
    lv_obj_set_style_pad_all(container, 10, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // Create statistics row
    stats_label = create_stats_row(container);

    // Create filter indicator
    filter_indicator = create_filter_indicator(container);

    // Create table header
    table_header = create_header_row(container);

    // Create message table
    message_table = create_message_table(container);

    return container;
}

lv_obj_t* ScreenSniffer::create_stats_row(lv_obj_t* parent) {
    lv_obj_t* stats_container = lv_obj_create(parent);
    lv_obj_set_width(stats_container, LV_PCT(100));
    lv_obj_set_height(stats_container, 30);
    lv_obj_set_style_bg_color(stats_container, lv_color_hex(THEME_COLOR_SURFACE_VARIANT), 0);
    lv_obj_set_style_border_width(stats_container, 0, 0);
    lv_obj_set_style_radius(stats_container, 4, 0);
    lv_obj_set_style_pad_all(stats_container, 5, 0);

    lv_obj_t* label = lv_label_create(stats_container);
    lv_label_set_text(label, "Messages: 0 | Rate: 0.0 msg/s | Errors: 0");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    return label;
}

lv_obj_t* ScreenSniffer::create_filter_indicator(lv_obj_t* parent) {
    lv_obj_t* indicator = lv_obj_create(parent);
    lv_obj_set_width(indicator, LV_PCT(100));
    lv_obj_set_height(indicator, 30);
    lv_obj_set_style_bg_color(indicator, lv_color_hex(THEME_COLOR_WARNING), 0);
    lv_obj_set_style_border_width(indicator, 0, 0);
    lv_obj_set_style_radius(indicator, 4, 0);
    lv_obj_set_style_pad_all(indicator, 5, 0);
    lv_obj_add_flag(indicator, LV_OBJ_FLAG_HIDDEN);  // Hidden by default

    lv_obj_t* label = lv_label_create(indicator);
    lv_label_set_text(label, LV_SYMBOL_WARNING " Filters Active: 0");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(THEME_COLOR_BACKGROUND), 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    return indicator;
}

lv_obj_t* ScreenSniffer::create_header_row(lv_obj_t* parent) {
    lv_obj_t* header = lv_obj_create(parent);
    lv_obj_set_width(header, LV_PCT(100));
    lv_obj_set_height(header, 35);
    lv_obj_set_style_bg_color(header, lv_color_hex(THEME_COLOR_PRIMARY), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 4, 0);
    lv_obj_set_style_pad_all(header, 5, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    // Use flex layout for header columns
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    // Number / Count column (changes label based on mode)
    lv_obj_t* num_label = lv_label_create(header);
    lv_label_set_text(num_label, (display_mode == SnifferDisplayMode::CHRONOLOGICAL) ? "#" : "Count");
    lv_obj_set_width(num_label, COL_WIDTH_NUM);
    lv_obj_set_style_text_font(num_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(num_label, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    // Timestamp column
    lv_obj_t* ts_label = lv_label_create(header);
    lv_label_set_text(ts_label, "Time");
    lv_obj_set_width(ts_label, COL_WIDTH_TIMESTAMP);
    lv_obj_set_style_text_font(ts_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(ts_label, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    // ID column
    lv_obj_t* id_label = lv_label_create(header);
    lv_label_set_text(id_label, "ID");
    lv_obj_set_width(id_label, COL_WIDTH_ID);
    lv_obj_set_style_text_font(id_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(id_label, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    // DLC column
    lv_obj_t* dlc_label = lv_label_create(header);
    lv_label_set_text(dlc_label, "DLC");
    lv_obj_set_width(dlc_label, COL_WIDTH_DLC);
    lv_obj_set_style_text_font(dlc_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(dlc_label, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    // Data column
    lv_obj_t* data_label = lv_label_create(header);
    lv_label_set_text(data_label, "Data");
    lv_obj_set_width(data_label, COL_WIDTH_DATA);
    lv_obj_set_style_text_font(data_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(data_label, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    return header;
}

lv_obj_t* ScreenSniffer::create_message_table(lv_obj_t* parent) {
    // Create scrollable container for messages
    lv_obj_t* scroll_cont = lv_obj_create(parent);
    lv_obj_set_width(scroll_cont, LV_PCT(100));
    lv_obj_set_flex_grow(scroll_cont, 1);  // Take remaining space
    lv_obj_set_style_bg_color(scroll_cont, lv_color_hex(THEME_COLOR_BACKGROUND), 0);
    lv_obj_set_style_border_width(scroll_cont, 1, 0);
    lv_obj_set_style_border_color(scroll_cont, lv_color_hex(THEME_COLOR_SURFACE_VARIANT), 0);
    lv_obj_set_style_radius(scroll_cont, 4, 0);
    lv_obj_set_style_pad_all(scroll_cont, 5, 0);
    lv_obj_set_scroll_dir(scroll_cont, LV_DIR_VER);
    lv_obj_add_event_cb(scroll_cont, table_scroll_cb, LV_EVENT_SCROLL, NULL);

    // Create table inside scroll container
    lv_obj_t* table = lv_table_create(scroll_cont);
    lv_obj_set_width(table, LV_PCT(100));
    lv_table_set_col_cnt(table, 5);  // Number/Count, Timestamp, ID, DLC, Data
    lv_table_set_row_cnt(table, MAX_TABLE_ROWS);

    // Set column widths
    lv_table_set_col_width(table, 0, COL_WIDTH_NUM);
    lv_table_set_col_width(table, 1, COL_WIDTH_TIMESTAMP);
    lv_table_set_col_width(table, 2, COL_WIDTH_ID);
    lv_table_set_col_width(table, 3, COL_WIDTH_DLC);
    lv_table_set_col_width(table, 4, COL_WIDTH_DATA);

    // Style table
    lv_obj_set_style_bg_color(table, lv_color_hex(THEME_COLOR_SURFACE), 0);
    lv_obj_set_style_text_font(table, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(table, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_border_width(table, 0, 0);
    lv_obj_set_style_pad_all(table, 2, 0);

    // Initialize empty rows
    for (uint16_t row = 0; row < MAX_TABLE_ROWS; row++) {
        lv_table_set_cell_value(table, row, 0, "");
        lv_table_set_cell_value(table, row, 1, "");
        lv_table_set_cell_value(table, row, 2, "");
        lv_table_set_cell_value(table, row, 3, "");
        lv_table_set_cell_value(table, row, 4, "");
    }

    return scroll_cont;
}

lv_obj_t* ScreenSniffer::create_right_column(lv_obj_t* parent) {
    // Create container for right column
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 10, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(container, 10, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // ============ TOP SECTION: Settings Switches ============

    // Display Mode Switch
    lv_obj_t* mode_container = lv_obj_create(container);
    lv_obj_set_size(mode_container, LV_PCT(100), 70);
    lv_obj_set_style_bg_color(mode_container, lv_color_hex(THEME_COLOR_SURFACE), 0);
    lv_obj_set_style_radius(mode_container, 8, 0);
    lv_obj_set_style_border_width(mode_container, 0, 0);
    lv_obj_set_style_pad_all(mode_container, 10, 0);
    lv_obj_set_flex_flow(mode_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mode_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(mode_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* mode_label = lv_label_create(mode_container);
    lv_label_set_text(mode_label, "Chronological");
    lv_obj_set_style_text_font(mode_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(mode_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    display_mode_switch = lv_switch_create(mode_container);
    lv_obj_set_size(display_mode_switch, 50, 25);
    lv_obj_add_event_cb(display_mode_switch, display_mode_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_style_bg_color(display_mode_switch, lv_color_hex(THEME_COLOR_SUCCESS), LV_PART_INDICATOR);

    // SD Logging Switch
    lv_obj_t* log_container = lv_obj_create(container);
    lv_obj_set_size(log_container, LV_PCT(100), 70);
    lv_obj_set_style_bg_color(log_container, lv_color_hex(THEME_COLOR_SURFACE), 0);
    lv_obj_set_style_radius(log_container, 8, 0);
    lv_obj_set_style_border_width(log_container, 0, 0);
    lv_obj_set_style_pad_all(log_container, 10, 0);
    lv_obj_set_flex_flow(log_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(log_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(log_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* log_label = lv_label_create(log_container);
    lv_label_set_text(log_label, "SD Logging");
    lv_obj_set_style_text_font(log_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(log_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    logging_switch = lv_switch_create(log_container);
    lv_obj_set_size(logging_switch, 50, 25);
    lv_obj_add_event_cb(logging_switch, logging_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_style_bg_color(logging_switch, lv_color_hex(THEME_COLOR_SUCCESS), LV_PART_INDICATOR);

    // Spacer to push center section down
    lv_obj_t* spacer_top = lv_obj_create(container);
    lv_obj_set_style_bg_opa(spacer_top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer_top, 0, 0);
    lv_obj_set_flex_grow(spacer_top, 1);
    lv_obj_set_height(spacer_top, LV_SIZE_CONTENT);

    // ============ CENTER SECTION: Action Buttons ============

    // Filter Button
    lv_obj_t* filter_btn = lv_btn_create(container);
    lv_obj_set_size(filter_btn, LV_PCT(100), 60);
    lv_obj_set_style_bg_color(filter_btn, lv_color_hex(THEME_COLOR_PRIMARY), 0);
    lv_obj_set_style_radius(filter_btn, 8, 0);
    lv_obj_add_event_cb(filter_btn, filter_button_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* filter_label = lv_label_create(filter_btn);
    lv_label_set_text(filter_label, LV_SYMBOL_SETTINGS " Filter");
    lv_obj_set_style_text_font(filter_label, &lv_font_montserrat_24, 0);
    lv_obj_center(filter_label);

    // Export Button
    lv_obj_t* export_btn = lv_btn_create(container);
    lv_obj_set_size(export_btn, LV_PCT(100), 60);
    lv_obj_set_style_bg_color(export_btn, lv_color_hex(THEME_COLOR_ACCENT), 0);
    lv_obj_set_style_radius(export_btn, 8, 0);
    lv_obj_add_event_cb(export_btn, export_button_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* export_label = lv_label_create(export_btn);
    lv_label_set_text(export_label, LV_SYMBOL_SAVE " Export");
    lv_obj_set_style_text_font(export_label, &lv_font_montserrat_24, 0);
    lv_obj_center(export_label);

    // Clear Button
    lv_obj_t* clear_btn = lv_btn_create(container);
    lv_obj_set_size(clear_btn, LV_PCT(100), 60);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(THEME_COLOR_ERROR), 0);
    lv_obj_set_style_radius(clear_btn, 8, 0);
    lv_obj_add_event_cb(clear_btn, clear_button_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, LV_SYMBOL_TRASH " Clear");
    lv_obj_set_style_text_font(clear_label, &lv_font_montserrat_24, 0);
    lv_obj_center(clear_label);

    // Spacer to push bottom button down
    lv_obj_t* spacer_bottom = lv_obj_create(container);
    lv_obj_set_style_bg_opa(spacer_bottom, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer_bottom, 0, 0);
    lv_obj_set_flex_grow(spacer_bottom, 1);
    lv_obj_set_height(spacer_bottom, LV_SIZE_CONTENT);

    // ============ BOTTOM SECTION: Large Start/Stop Button ============

    // Start/Stop Button (Large)
    pause_button = lv_btn_create(container);
    lv_obj_set_size(pause_button, LV_PCT(100), 140);
    lv_obj_set_style_bg_color(pause_button, lv_color_hex(THEME_COLOR_SUCCESS), 0);
    lv_obj_set_style_radius(pause_button, 12, 0);
    lv_obj_add_event_cb(pause_button, pause_button_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* pause_label = lv_label_create(pause_button);
    lv_label_set_text(pause_label, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_font(pause_label, &lv_font_montserrat_48, 0);
    lv_obj_center(pause_label);

    return container;
}

void ScreenSniffer::update(lv_obj_t* screen) {
    if (!screen || !message_table) return;

    extern CANSniffer sniffer;
    uint32_t now = millis();

    // Get table update interval from settings
    const AppSettings& settings = SettingsManager::get();
    uint32_t update_interval = settings.table_update_ms;

    // Update message table periodically (only if sniffer is running)
    if (sniffer.is_running() && now - last_update_time >= update_interval) {
        update_message_table();
        update_filter_indicator();
        last_update_time = now;
    }

    // Update statistics (always update stats)
    if (stats_label) {
        const ApplicationState& state = StateManager::get_state();
        char buf[128];
        const char* status = sniffer.is_running() ? "" : " [STOPPED]";
        snprintf(buf, sizeof(buf), "Messages: %lu | Rate: %.1f msg/s | Errors: %lu%s",
                 state.message_count,
                 sniffer.get_messages_per_second(),
                 state.error_count,
                 status);
        lv_label_set_text(stats_label, buf);
    }
}

void ScreenSniffer::update_message_table() {
    if (display_mode == SnifferDisplayMode::CHRONOLOGICAL) {
        update_chronological_table();
    } else {
        update_grouped_table();
    }
}

void ScreenSniffer::update_chronological_table() {
    extern CANSniffer sniffer;
    const RingBuffer<CANMessage>& buffer = sniffer.get_buffer();
    uint32_t message_count = buffer.size();

    // Calculate how many new messages to display
    uint32_t new_messages = (message_count > displayed_message_count) ?
                            (message_count - displayed_message_count) : 0;

    if (new_messages == 0) return;

    // Get the actual table widget (child of scroll container)
    lv_obj_t* table = lv_obj_get_child(message_table, 0);
    if (!table) return;

    // Limit new messages to avoid processing too many at once
    if (new_messages > MAX_TABLE_ROWS) {
        new_messages = MAX_TABLE_ROWS;
        displayed_message_count = message_count - MAX_TABLE_ROWS;
        chronological_next_row = 0;  // Reset circular buffer
    }

    // OPTIMIZED: Write to circular buffer (no row shifting!)
    // Instead of shifting all rows up, we use rows in circular fashion
    for (uint32_t i = 0; i < new_messages; i++) {
        uint32_t msg_index = message_count - new_messages + i;
        CANMessage msg;
        if (buffer.peek(msg_index, msg)) {
            uint32_t msg_number = displayed_message_count + i + 1;

            // Write to current row position
            add_message_to_table_with_number(msg, chronological_next_row, msg_number);

            // Store message number in this row for reference
            chronological_row_message_numbers[chronological_next_row] = msg_number;

            // Move to next row (circular)
            chronological_next_row = (chronological_next_row + 1) % MAX_TABLE_ROWS;
        }
    }

    displayed_message_count += new_messages;

    // Auto-scroll to bottom if enabled in settings
    const AppSettings& settings = SettingsManager::get();
    if (settings.auto_scroll) {
        // Note: With circular buffer, newest messages wrap around but
        // scrolling to bottom gives a natural feel
        lv_obj_scroll_to_y(message_table, lv_obj_get_scroll_bottom(message_table), LV_ANIM_OFF);
    }
}

void ScreenSniffer::update_grouped_table() {
    extern CANSniffer sniffer;
    const RingBuffer<CANMessage>& buffer = sniffer.get_buffer();
    uint32_t message_count = buffer.size();

    lv_obj_t* table = lv_obj_get_child(message_table, 0);
    if (!table) return;

    // Track which IDs were updated this cycle
    std::set<uint32_t> updated_ids;

    // Process all new messages since last update
    for (uint32_t i = displayed_message_count; i < message_count; i++) {
        CANMessage msg;
        if (!buffer.peek(i, msg)) continue;

        // Update or add tracker for this ID
        auto it = message_trackers.find(msg.id);
        if (it != message_trackers.end()) {
            // Update existing tracker
            it->second.last_message = msg;
            it->second.count++;
            it->second.last_update_time = millis();
        } else {
            // Create new tracker
            MessageTracker tracker;
            tracker.last_message = msg;
            tracker.count = 1;
            tracker.last_update_time = millis();
            message_trackers[msg.id] = tracker;
        }

        updated_ids.insert(msg.id);
    }

    displayed_message_count = message_count;

    // OPTIMIZED: Only update changed rows, don't rebuild entire table
    // If we have new IDs or the row mapping is invalid, rebuild mapping
    bool needs_remap = false;
    for (const auto& pair : message_trackers) {
        if (id_to_row_map.find(pair.first) == id_to_row_map.end()) {
            needs_remap = true;
            break;
        }
    }

    if (needs_remap) {
        // Rebuild row mapping (happens only when new IDs appear)
        id_to_row_map.clear();
        uint16_t row = 0;
        for (const auto& pair : message_trackers) {
            if (row >= MAX_TABLE_ROWS) break;
            id_to_row_map[pair.first] = row;
            row++;
        }

        // Clear unused rows
        for (; row < MAX_TABLE_ROWS; row++) {
            for (uint8_t col = 0; col < 5; col++) {
                lv_table_set_cell_value(table, row, col, "");
            }
        }

        // Redraw all rows with new mapping
        for (const auto& pair : message_trackers) {
            uint16_t row_index = id_to_row_map[pair.first];
            add_grouped_message_to_table(pair.first, pair.second, row_index);
        }
    } else {
        // FAST PATH: Only update rows that changed
        for (uint32_t id : updated_ids) {
            auto row_it = id_to_row_map.find(id);
            if (row_it != id_to_row_map.end()) {
                uint16_t row_index = row_it->second;
                const MessageTracker& tracker = message_trackers[id];
                add_grouped_message_to_table(id, tracker, row_index);
            }
        }
    }
}

void ScreenSniffer::rebuild_table_header() {
    if (!table_header) return;

    // Delete old header
    lv_obj_del(table_header);

    // Get parent container
    lv_obj_t* parent = lv_obj_get_parent(message_table);
    if (!parent) return;

    // Create new header with updated mode
    table_header = create_header_row(parent);

    // Move header to correct position (before message_table)
    lv_obj_move_foreground(table_header);
}

void ScreenSniffer::add_message_to_table(const CANMessage& msg, uint16_t row) {
    add_message_to_table_with_number(msg, row, 0);  // 0 = no number
}

void ScreenSniffer::add_message_to_table_with_number(const CANMessage& msg, uint16_t row, uint32_t msg_num) {
    lv_obj_t* table = lv_obj_get_child(message_table, 0);
    if (!table || row >= MAX_TABLE_ROWS) return;

    char buf[64];

    // Message number (column 0)
    if (msg_num > 0) {
        snprintf(buf, sizeof(buf), "%lu", msg_num);
    } else {
        buf[0] = '\0';
    }
    lv_table_set_cell_value(table, row, 0, buf);

    // Timestamp (column 1)
    format_timestamp(buf, msg.timestamp_ms);
    lv_table_set_cell_value(table, row, 1, buf);

    // ID (column 2)
    format_message_id(buf, msg.id, msg.type);
    lv_table_set_cell_value(table, row, 2, buf);

    // DLC (column 3)
    snprintf(buf, sizeof(buf), "%d", msg.dlc);
    lv_table_set_cell_value(table, row, 3, buf);

    // Data (column 4)
    format_message_data(buf, msg.data, msg.dlc);
    lv_table_set_cell_value(table, row, 4, buf);

    // Color code based on frame type
    uint32_t color = (msg.rtr) ? THEME_COLOR_CAN_RTR : THEME_COLOR_TEXT_PRIMARY;
    char id_buf[16];
    format_message_id(id_buf, msg.id, msg.type);
    lv_table_set_cell_value_fmt(table, row, 2, "#%06X %s#", color, id_buf);
}

void ScreenSniffer::add_grouped_message_to_table(uint32_t id, const MessageTracker& tracker, uint16_t row) {
    lv_obj_t* table = lv_obj_get_child(message_table, 0);
    if (!table || row >= MAX_TABLE_ROWS) return;

    char buf[64];
    const CANMessage& msg = tracker.last_message;

    // Message count (column 0)
    snprintf(buf, sizeof(buf), "%lu", tracker.count);
    lv_table_set_cell_value(table, row, 0, buf);

    // Timestamp of last message (column 1)
    format_timestamp(buf, msg.timestamp_ms);
    lv_table_set_cell_value(table, row, 1, buf);

    // ID (column 2)
    format_message_id(buf, id, msg.type);
    lv_table_set_cell_value(table, row, 2, buf);

    // DLC (column 3)
    snprintf(buf, sizeof(buf), "%d", msg.dlc);
    lv_table_set_cell_value(table, row, 3, buf);

    // Data (column 4)
    format_message_data(buf, msg.data, msg.dlc);
    lv_table_set_cell_value(table, row, 4, buf);

    // Color code based on frame type
    uint32_t color = (msg.rtr) ? THEME_COLOR_CAN_RTR : THEME_COLOR_TEXT_PRIMARY;
    char id_buf[16];
    format_message_id(id_buf, id, msg.type);
    lv_table_set_cell_value_fmt(table, row, 2, "#%06X %s#", color, id_buf);
}

void ScreenSniffer::format_timestamp(char* buf, uint32_t timestamp) {
    uint32_t seconds = timestamp / 1000;
    uint32_t millis = timestamp % 1000;
    snprintf(buf, 64, "%02lu:%02lu.%03lu",
             (seconds / 60) % 60, seconds % 60, millis);
}

void ScreenSniffer::format_message_id(char* buf, uint32_t id, CANFrameType type) {
    if (type == CANFrameType::STANDARD) {
        snprintf(buf, 64, "0x%03X", id);
    } else {
        snprintf(buf, 64, "0x%08X", id);
    }
}

void ScreenSniffer::format_message_data(char* buf, const uint8_t* data, uint8_t dlc) {
    buf[0] = '\0';
    for (uint8_t i = 0; i < dlc && i < 8; i++) {
        char byte_str[4];
        snprintf(byte_str, sizeof(byte_str), "%02X ", data[i]);
        strcat(buf, byte_str);
    }
}

void ScreenSniffer::update_filter_indicator() {
    if (!filter_indicator) return;

    extern CANFilter filter;  // Global instance from main.cpp
    uint32_t active_count = get_active_filter_count();

    if (active_count > 0 && filter.is_enabled()) {
        char buf[64];
        snprintf(buf, sizeof(buf), LV_SYMBOL_WARNING " Filters Active: %lu", active_count);
        lv_obj_t* label = lv_obj_get_child(filter_indicator, 0);
        lv_label_set_text(label, buf);
        lv_obj_clear_flag(filter_indicator, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(filter_indicator, LV_OBJ_FLAG_HIDDEN);
    }
}

uint32_t ScreenSniffer::get_active_filter_count() {
    extern CANFilter filter;

    uint32_t count = 0;
    for (int i = 0; i < filter.get_rule_count(); i++) {
        const FilterRule* rule = filter.get_rule(i);
        if (rule && rule->enabled) {
            count++;
        }
    }
    return count;
}

void ScreenSniffer::create_filter_dialog() {
    // Create modal dialog
    filter_dialog = lv_obj_create(lv_scr_act());
    lv_obj_set_size(filter_dialog, 600, 500);
    lv_obj_center(filter_dialog);
    lv_obj_set_style_bg_color(filter_dialog, lv_color_hex(THEME_COLOR_SURFACE), 0);
    lv_obj_set_style_radius(filter_dialog, 12, 0);
    lv_obj_set_style_shadow_width(filter_dialog, 20, 0);
    lv_obj_set_style_shadow_opa(filter_dialog, LV_OPA_50, 0);

    // Title
    lv_obj_t* title = lv_label_create(filter_dialog);
    lv_label_set_text(title, "Filter Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_26, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // Filter ID input
    lv_obj_t* id_label = lv_label_create(filter_dialog);
    lv_label_set_text(id_label, "CAN ID (hex):");
    lv_obj_align(id_label, LV_ALIGN_TOP_LEFT, 20, 60);

    lv_obj_t* id_input = lv_textarea_create(filter_dialog);
    lv_obj_set_size(id_input, 200, 50);
    lv_obj_align(id_input, LV_ALIGN_TOP_LEFT, 20, 85);
    lv_textarea_set_placeholder_text(id_input, "e.g., 123 or 18DA10F1");
    lv_textarea_set_one_line(id_input, true);
    lv_textarea_set_max_length(id_input, 8);

    // Mask input
    lv_obj_t* mask_label = lv_label_create(filter_dialog);
    lv_label_set_text(mask_label, "Mask (hex):");
    lv_obj_align(mask_label, LV_ALIGN_TOP_LEFT, 250, 60);

    lv_obj_t* mask_input = lv_textarea_create(filter_dialog);
    lv_obj_set_size(mask_input, 200, 50);
    lv_obj_align(mask_input, LV_ALIGN_TOP_LEFT, 250, 85);
    lv_textarea_set_placeholder_text(mask_input, "e.g., 7FF");
    lv_textarea_set_one_line(mask_input, true);
    lv_textarea_set_max_length(mask_input, 8);

    // Create virtual keyboard
    keyboard = lv_keyboard_create(filter_dialog);
    lv_obj_set_size(keyboard, LV_PCT(90), LV_PCT(40));
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(keyboard, id_input);

    // Buttons
    lv_obj_t* apply_btn = lv_btn_create(filter_dialog);
    lv_obj_set_size(apply_btn, 120, 50);
    lv_obj_align(apply_btn, LV_ALIGN_BOTTOM_LEFT, 20, -10);
    lv_obj_set_style_bg_color(apply_btn, lv_color_hex(THEME_COLOR_SUCCESS), 0);
    lv_obj_add_event_cb(apply_btn, filter_apply_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* apply_label = lv_label_create(apply_btn);
    lv_label_set_text(apply_label, "Apply");
    lv_obj_center(apply_label);

    lv_obj_t* clear_btn = lv_btn_create(filter_dialog);
    lv_obj_set_size(clear_btn, 120, 50);
    lv_obj_align(clear_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(THEME_COLOR_WARNING), 0);
    lv_obj_add_event_cb(clear_btn, filter_clear_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, "Clear All");
    lv_obj_center(clear_label);

    lv_obj_t* close_btn = lv_btn_create(filter_dialog);
    lv_obj_set_size(close_btn, 120, 50);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(THEME_COLOR_ERROR), 0);
    lv_obj_add_event_cb(close_btn, filter_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "Close");
    lv_obj_center(close_label);
}

void ScreenSniffer::close_filter_dialog() {
    if (filter_dialog) {
        lv_obj_del(filter_dialog);
        filter_dialog = nullptr;
        keyboard = nullptr;
    }
}

void ScreenSniffer::create_export_dialog() {
    // Create simple confirmation dialog
    lv_obj_t* dialog = lv_obj_create(lv_scr_act());
    lv_obj_set_size(dialog, 400, 250);
    lv_obj_center(dialog);
    lv_obj_set_style_bg_color(dialog, lv_color_hex(THEME_COLOR_SURFACE), 0);
    lv_obj_set_style_radius(dialog, 12, 0);

    lv_obj_t* title = lv_label_create(dialog);
    lv_label_set_text(title, "Export to SD Card");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_26, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t* msg = lv_label_create(dialog);
    lv_label_set_text(msg, "Export current CAN messages\nto SD card as CSV file?");
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t* confirm_btn = lv_btn_create(dialog);
    lv_obj_set_size(confirm_btn, 150, 50);
    lv_obj_align(confirm_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_set_style_bg_color(confirm_btn, lv_color_hex(THEME_COLOR_SUCCESS), 0);
    lv_obj_add_event_cb(confirm_btn, export_confirm_cb, LV_EVENT_CLICKED, dialog);

    lv_obj_t* confirm_label = lv_label_create(confirm_btn);
    lv_label_set_text(confirm_label, "Export");
    lv_obj_center(confirm_label);

    lv_obj_t* cancel_btn = lv_btn_create(dialog);
    lv_obj_set_size(cancel_btn, 150, 50);
    lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(THEME_COLOR_ERROR), 0);
    lv_obj_add_event_cb(cancel_btn, export_cancel_cb, LV_EVENT_CLICKED, dialog);

    lv_obj_t* cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_center(cancel_label);
}

void ScreenSniffer::cleanup() {
    message_table = nullptr;
    filter_indicator = nullptr;
    stats_label = nullptr;
    logging_switch = nullptr;
    display_mode_switch = nullptr;
    filter_dialog = nullptr;
    keyboard = nullptr;
    table_header = nullptr;
    continuous_logging_enabled = false;
    displayed_message_count = 0;
    display_mode = SnifferDisplayMode::CHRONOLOGICAL;
    message_trackers.clear();
}

// Event Callbacks

void ScreenSniffer::home_button_cb(lv_event_t* e) {
    Serial.println("Returning to main dashboard...");
    UIManager::navigate_to(Screen::MAIN_DASHBOARD);
}

void ScreenSniffer::filter_button_cb(lv_event_t* e) {
    Serial.println("Opening filter dialog...");
    create_filter_dialog();
}

void ScreenSniffer::logging_switch_cb(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target(e);
    continuous_logging_enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if (continuous_logging_enabled) {
        Serial.println("Continuous logging enabled");
        create_log_file();
    } else {
        Serial.println("Continuous logging disabled");
    }
}

void ScreenSniffer::display_mode_switch_cb(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target(e);
    bool grouped_mode = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if (!grouped_mode) {
        display_mode = SnifferDisplayMode::GROUPED_BY_ID;
        Serial.println("Display mode: Grouped by ID");
        message_trackers.clear();  // Clear existing trackers
        displayed_message_count = 0;  // Reset to reprocess all messages
    } else {
        display_mode = SnifferDisplayMode::CHRONOLOGICAL;
        Serial.println("Display mode: Chronological");
        displayed_message_count = 0;  // Reset display counter
    }

    // Rebuild table header with new column labels
    rebuild_table_header();

    // Clear table
    lv_obj_t* table = lv_obj_get_child(message_table, 0);
    if (table) {
        for (uint16_t row = 0; row < MAX_TABLE_ROWS; row++) {
            for (uint8_t col = 0; col < 5; col++) {
                lv_table_set_cell_value(table, row, col, "");
            }
        }
    }
}

void ScreenSniffer::pause_button_cb(lv_event_t* e) {
    extern CANSniffer sniffer;

    // Toggle sniffer running state
    if (sniffer.is_running()) {
        // Stop the sniffer
        sniffer.stop();
        Serial.println("Sniffer STOPPED");

        // Update button to green "Start"
        lv_obj_set_style_bg_color(pause_button, lv_color_hex(THEME_COLOR_SUCCESS), 0);
        lv_obj_t* label = lv_obj_get_child(pause_button, 0);
        if (label) {
            lv_label_set_text(label, LV_SYMBOL_PLAY);
        }
    } else {
        // Start the sniffer
        sniffer.start();
        Serial.println("Sniffer STARTED");

        // Check if auto-log is enabled in settings
        const AppSettings& settings = SettingsManager::get();
        if (settings.auto_log_on_start && logging_switch) {
            // Enable logging automatically
            lv_obj_add_state(logging_switch, LV_STATE_CHECKED);
            continuous_logging_enabled = true;
            create_log_file();
            Serial.println("Auto-log enabled from settings");
        }

        // Update button to red "Stop"
        lv_obj_set_style_bg_color(pause_button, lv_color_hex(THEME_COLOR_ERROR), 0);
        lv_obj_t* label = lv_obj_get_child(pause_button, 0);
        if (label) {
            lv_label_set_text(label, LV_SYMBOL_STOP);
        }
    }
}

void ScreenSniffer::clear_button_cb(lv_event_t* e) {
    Serial.println("Clearing all buffers and counters...");

    extern CANSniffer sniffer;

    // Clear table
    lv_obj_t* table = lv_obj_get_child(message_table, 0);
    if (table) {
        for (uint16_t row = 0; row < MAX_TABLE_ROWS; row++) {
            for (uint8_t col = 0; col < 5; col++) {
                lv_table_set_cell_value(table, row, col, "");
            }
        }
    }

    // Reset display counters
    displayed_message_count = 0;

    // Clear grouped mode trackers
    message_trackers.clear();
    id_to_row_map.clear();

    // Clear chronological mode optimization state
    chronological_next_row = 0;
    for (int i = 0; i < MAX_TABLE_ROWS; i++) {
        chronological_row_message_numbers[i] = 0;
    }

    // Clear the sniffer buffer and counters
    sniffer.clear();

    Serial.println("All buffers cleared");
    UIManager::show_notification("All buffers cleared");
}

void ScreenSniffer::export_button_cb(lv_event_t* e) {
    Serial.println("Opening export dialog...");
    create_export_dialog();
}

void ScreenSniffer::filter_apply_cb(lv_event_t* e) {
    Serial.println("Applying filter...");
    apply_filter_from_dialog();
    close_filter_dialog();
}

void ScreenSniffer::filter_clear_cb(lv_event_t* e) {
    Serial.println("Clearing all filters...");
    clear_all_filters();
}

void ScreenSniffer::filter_close_cb(lv_event_t* e) {
    close_filter_dialog();
}

void ScreenSniffer::export_confirm_cb(lv_event_t* e) {
    lv_obj_t* dialog = (lv_obj_t*)lv_event_get_user_data(e);
    Serial.println("Exporting to SD card...");
    export_to_sd_card();
    lv_obj_del(dialog);
}

void ScreenSniffer::export_cancel_cb(lv_event_t* e) {
    lv_obj_t* dialog = (lv_obj_t*)lv_event_get_user_data(e);
    lv_obj_del(dialog);
}

void ScreenSniffer::table_scroll_cb(lv_event_t* e) {
    // Optional: Handle scroll events
}

// Filter and Logging Functions

void ScreenSniffer::apply_filter_from_dialog() {
    // TODO: Parse hex input from dialog and apply to CANFilter
    extern CANFilter filter;

    // This is a placeholder - you'll need to get values from text areas
    FilterRule rule = {
        .id = 0x123,
        .mask = 0x7FF,
        .enabled = true,
        .accept = true,
        .type = CANFrameType::STANDARD
    };

    filter.add_rule(rule);
    filter.set_enabled(true);

    UIManager::show_notification("Filter applied");
}

void ScreenSniffer::clear_all_filters() {
    extern CANFilter filter;
    filter.clear_rules();
    UIManager::show_notification("All filters cleared");
}

bool ScreenSniffer::create_log_file() {
    if (!StorageHAL::is_sd_available()) {
        UIManager::show_notification("SD card not available");
        return false;
    }

    // Create log file with timestamp
    const char* filename = get_log_filename();
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to create log file");
        return false;
    }

    // Write CSV header
    file.println("Timestamp,ID,DLC,Data");
    file.close();

    Serial.printf("Log file created: %s\n", filename);
    return true;
}

void ScreenSniffer::export_to_sd_card() {
    if (!StorageHAL::is_sd_available()) {
        UIManager::show_notification("SD card not available");
        return;
    }

    extern CANSniffer sniffer;
    const RingBuffer<CANMessage>& buffer = sniffer.get_buffer();

    const char* filename = "/can_export.csv";
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        UIManager::show_notification("Failed to create export file");
        return;
    }

    // Write header
    file.println("Timestamp,ID,DLC,Data");

    // Write all messages
    for (uint32_t i = 0; i < buffer.size(); i++) {
        CANMessage msg;
        if (!buffer.peek(i, msg)) continue;

        char id_str[16], data_str[32];
        format_message_id(id_str, msg.id, msg.type);
        format_message_data(data_str, msg.data, msg.dlc);

        file.printf("%lu,%s,%d,%s\n", msg.timestamp_ms, id_str, msg.dlc, data_str);
    }

    file.close();

    char notification[64];
    snprintf(notification, sizeof(notification), "Exported %lu messages", buffer.size());
    UIManager::show_notification(notification);

    Serial.printf("Exported %lu messages to %s\n", buffer.size(), filename);
}

bool ScreenSniffer::write_message_to_sd(const CANMessage& msg) {
    if (!continuous_logging_enabled || !StorageHAL::is_sd_available()) {
        return false;
    }

    const char* filename = get_log_filename();
    File file = SD.open(filename, FILE_APPEND);
    if (!file) {
        return false;
    }

    char id_str[16], data_str[32];
    format_message_id(id_str, msg.id, msg.type);
    format_message_data(data_str, msg.data, msg.dlc);

    file.printf("%lu,%s,%d,%s\n", msg.timestamp_ms, id_str, msg.dlc, data_str);
    file.close();

    return true;
}

const char* ScreenSniffer::get_log_filename() {
    static char filename[32];
    // TODO: Add timestamp to filename
    snprintf(filename, sizeof(filename), "/can_log_%lu.csv", millis());
    return filename;
}

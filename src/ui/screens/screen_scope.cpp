#include "screen_scope.h"
#include "../widgets/status_bar.h"
#include "../themes/theme_colors.h"
#include "../../config/app_config.h"
#include "../../services/scope_manager.h"
#include "../ui_manager.h"
#include <Arduino.h>

// Static member initialization
lv_obj_t* ScreenScope::output_textarea = nullptr;
lv_obj_t* ScreenScope::stats_label = nullptr;
lv_obj_t* ScreenScope::status_indicator = nullptr;
uint32_t ScreenScope::last_update_time = 0;
bool ScreenScope::auto_scroll = true;

lv_obj_t* ScreenScope::create() {
    Serial.println("Creating Scope screen...");

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
    lv_obj_set_style_pad_all(content, 15, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 10, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Header
    lv_obj_t* header = create_header(content);
    lv_obj_set_height(header, LV_SIZE_CONTENT);

    // Output area
    lv_obj_t* output = create_output_area(content);
    lv_obj_set_flex_grow(output, 1);

    // Control buttons
    lv_obj_t* controls = create_control_buttons(content);
    lv_obj_set_height(controls, LV_SIZE_CONTENT);

    last_update_time = 0;
    auto_scroll = true;

    Serial.println("Scope screen created");
    return screen;
}

lv_obj_t* ScreenScope::create_header(lv_obj_t* parent) {
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_width(container, LV_PCT(100));
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(container, 8, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* title = lv_label_create(container);
    lv_label_set_text(title, LV_SYMBOL_EYE_OPEN " CAN Scope - Debug Output");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    // Status row
    lv_obj_t* status_row = lv_obj_create(container);
    lv_obj_set_width(status_row, LV_PCT(100));
    lv_obj_set_height(status_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(status_row, lv_color_hex(THEME_COLOR_SURFACE), 0);
    lv_obj_set_style_radius(status_row, 6, 0);
    lv_obj_set_style_pad_all(status_row, 10, 0);
    lv_obj_set_flex_flow(status_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(status_row, LV_OBJ_FLAG_SCROLLABLE);

    // Status indicator
    status_indicator = lv_label_create(status_row);
    lv_label_set_text(status_indicator, LV_SYMBOL_WIFI " Waiting...");
    lv_obj_set_style_text_color(status_indicator, lv_color_hex(THEME_COLOR_WARNING), 0);
    lv_obj_set_style_text_font(status_indicator, &lv_font_montserrat_18, 0);

    // Stats label
    stats_label = lv_label_create(status_row);
    lv_label_set_text(stats_label, "Bytes: 0 | Rate: 0 B/s");
    lv_obj_set_style_text_color(stats_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_text_font(stats_label, &lv_font_montserrat_18, 0);

    return container;
}

lv_obj_t* ScreenScope::create_output_area(lv_obj_t* parent) {
    // Container
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_width(container, LV_PCT(100));
    lv_obj_set_style_bg_color(container, lv_color_hex(THEME_COLOR_SURFACE), 0);
    lv_obj_set_style_border_width(container, 1, 0);
    lv_obj_set_style_border_color(container, lv_color_hex(THEME_COLOR_SURFACE_VARIANT), 0);
    lv_obj_set_style_radius(container, 8, 0);
    lv_obj_set_style_pad_all(container, 10, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // Text area for output
    output_textarea = lv_textarea_create(container);
    lv_obj_set_size(output_textarea, LV_PCT(100), LV_PCT(100));
    lv_textarea_set_text(output_textarea, "Waiting for data from Nucleo...\n");
    lv_obj_set_style_text_font(output_textarea, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(output_textarea, lv_color_hex(0x00FF00), 0);  // Green terminal color
    lv_obj_set_style_bg_color(output_textarea, lv_color_hex(0x000000), 0);    // Black background

    // Make textarea read-only (no cursor)
    lv_obj_clear_flag(output_textarea, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(output_textarea, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    return container;
}

lv_obj_t* ScreenScope::create_control_buttons(lv_obj_t* parent) {
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_width(container, LV_PCT(100));
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // Clear button
    lv_obj_t* clear_btn = lv_btn_create(container);
    lv_obj_set_size(clear_btn, 200, 60);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(THEME_COLOR_ERROR), 0);
    lv_obj_set_style_radius(clear_btn, 8, 0);
    lv_obj_add_event_cb(clear_btn, clear_button_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, LV_SYMBOL_TRASH " Clear");
    lv_obj_set_style_text_font(clear_label, &lv_font_montserrat_20, 0);
    lv_obj_center(clear_label);

    // Auto-scroll switch
    lv_obj_t* scroll_cont = lv_obj_create(container);
    lv_obj_set_size(scroll_cont, 250, 60);
    lv_obj_set_style_bg_color(scroll_cont, lv_color_hex(THEME_COLOR_SURFACE), 0);
    lv_obj_set_style_radius(scroll_cont, 8, 0);
    lv_obj_set_style_pad_all(scroll_cont, 10, 0);
    lv_obj_set_flex_flow(scroll_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scroll_cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(scroll_cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* scroll_label = lv_label_create(scroll_cont);
    lv_label_set_text(scroll_label, "Auto-Scroll");
    lv_obj_set_style_text_color(scroll_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(scroll_label, &lv_font_montserrat_18, 0);

    lv_obj_t* scroll_switch = lv_switch_create(scroll_cont);
    lv_obj_set_size(scroll_switch, 50, 25);
    lv_obj_add_state(scroll_switch, LV_STATE_CHECKED);  // On by default
    lv_obj_set_style_bg_color(scroll_switch, lv_color_hex(THEME_COLOR_SUCCESS), LV_PART_INDICATOR);
    lv_obj_add_event_cb(scroll_switch, autoscroll_cb, LV_EVENT_VALUE_CHANGED, NULL);

    return container;
}

void ScreenScope::update(lv_obj_t* screen) {
    if (!output_textarea || !stats_label || !status_indicator) {
        Serial.println("[SCOPE UI] ERROR: UI components not initialized!");
        return;
    }

    uint32_t now = millis();

    // Update every 500ms (2 times per second) to reduce load
    if (now - last_update_time >= 500) {
        // Get data from ScopeManager
        const char* buffer = ScopeManager::get_buffer();
        uint32_t total_bytes = ScopeManager::get_bytes_received();
        float bytes_per_sec = ScopeManager::get_bytes_per_second();
        bool connected = ScopeManager::is_connected();

        // Debug output
        size_t buffer_len = buffer ? strlen(buffer) : 0;
        Serial.printf("[SCOPE UI] Update: total=%lu, rate=%.0f, buflen=%u, connected=%d\n",
                      total_bytes, bytes_per_sec, buffer_len, connected);

        // Update status indicator
        if (connected) {
            lv_label_set_text(status_indicator, LV_SYMBOL_WIFI " Connected");
            lv_obj_set_style_text_color(status_indicator, lv_color_hex(THEME_COLOR_SUCCESS), 0);
        } else if (total_bytes > 0) {
            lv_label_set_text(status_indicator, LV_SYMBOL_WARNING " No Data");
            lv_obj_set_style_text_color(status_indicator, lv_color_hex(THEME_COLOR_WARNING), 0);
        } else {
            lv_label_set_text(status_indicator, LV_SYMBOL_CLOSE " Waiting");
            lv_obj_set_style_text_color(status_indicator, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
        }

        // Update stats
        char stats_buf[64];
        snprintf(stats_buf, sizeof(stats_buf), "Bytes: %lu | Rate: %.0f B/s", total_bytes, bytes_per_sec);
        lv_label_set_text(stats_label, stats_buf);

        // Update text area with buffer content (with safety checks)
        uint32_t buffer_size = ScopeManager::get_buffer_size();

        if (buffer && buffer_size > 0 && buffer_size < 512) {
            Serial.printf("[SCOPE UI] Setting textarea with %lu bytes\n", buffer_size);

            // Print first 20 bytes as HEX to Serial for debugging
            Serial.print("[SCOPE UI] First 20 bytes (HEX): ");
            for (uint32_t i = 0; i < (buffer_size < 20 ? buffer_size : 20); i++) {
                Serial.printf("%02X ", (uint8_t)buffer[i]);
            }
            Serial.println();

            // Create HEX display string (3 chars per byte: "FF ")
            // Limit to first 170 bytes to fit in display (170*3 = 510 chars)
            uint32_t display_bytes = (buffer_size < 170) ? buffer_size : 170;
            char* display_buffer = (char*)malloc(display_bytes * 3 + 1);
            if (!display_buffer) {
                Serial.println("[SCOPE UI] ERROR: malloc failed!");
                return;
            }

            // Convert to HEX string
            char* ptr = display_buffer;
            for (uint32_t i = 0; i < display_bytes; i++) {
                sprintf(ptr, "%02X ", (uint8_t)buffer[i]);
                ptr += 3;
            }
            *ptr = '\0';

            // Update textarea with HEX display
            lv_textarea_set_text(output_textarea, display_buffer);

            // Free allocated memory
            free(display_buffer);

            // Auto-scroll to bottom if enabled
            if (auto_scroll) {
                lv_textarea_set_cursor_pos(output_textarea, LV_TEXTAREA_CURSOR_LAST);
            }
        } else {
            Serial.printf("[SCOPE UI] Buffer issue: size=%lu\n", buffer_size);
        }

        last_update_time = now;
    }
}

void ScreenScope::cleanup() {
    output_textarea = nullptr;
    stats_label = nullptr;
    status_indicator = nullptr;
    last_update_time = 0;
    auto_scroll = true;
}

// Event Callbacks

void ScreenScope::clear_button_cb(lv_event_t* e) {
    Serial.println("Clear button clicked");
    ScopeManager::clear_buffer();
    if (output_textarea) {
        lv_textarea_set_text(output_textarea, "Buffer cleared.\n");
    }
    UIManager::show_notification("Buffer cleared");
}

void ScreenScope::autoscroll_cb(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target(e);
    auto_scroll = lv_obj_has_state(sw, LV_STATE_CHECKED);
    Serial.printf("Auto-scroll %s\n", auto_scroll ? "enabled" : "disabled");
}

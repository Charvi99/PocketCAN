#include "screen_settings.h"
#include "../widgets/status_bar.h"
#include "../themes/theme_colors.h"
#include "../../config/app_config.h"
#include "../ui_manager.h"
#include "../../hal/can_hal.h"
#include "../../hal/display_hal.h"
#include <Arduino.h>

// Static member initialization
lv_obj_t* ScreenSettings::can_baud_dropdown = nullptr;
lv_obj_t* ScreenSettings::brightness_slider = nullptr;
lv_obj_t* ScreenSettings::brightness_label = nullptr;
lv_obj_t* ScreenSettings::theme_switch = nullptr;
lv_obj_t* ScreenSettings::auto_scroll_switch = nullptr;
lv_obj_t* ScreenSettings::table_update_slider = nullptr;
lv_obj_t* ScreenSettings::table_update_label = nullptr;
lv_obj_t* ScreenSettings::auto_log_switch = nullptr;
lv_obj_t* ScreenSettings::splash_switch = nullptr;
AppSettings ScreenSettings::temp_settings;

lv_obj_t* ScreenSettings::create() {
    Serial.println("Creating Settings screen...");

    // Load current settings into temporary copy
    temp_settings = SettingsManager::get();

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
    lv_obj_set_style_pad_all(content, 20, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Title
    lv_obj_t* title = lv_label_create(content);
    lv_label_set_text(title, LV_SYMBOL_SETTINGS " Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_pad_bottom(title, 20, 0);

    // Settings list (scrollable)
    lv_obj_t* settings_list = create_settings_list(content);
    lv_obj_set_flex_grow(settings_list, 1);

    // Button row at bottom
    lv_obj_t* button_row = create_button_row(content);

    Serial.println("Settings screen created");
    return screen;
}

lv_obj_t* ScreenSettings::create_settings_list(lv_obj_t* parent) {
    // Scrollable container for settings
    lv_obj_t* list = lv_obj_create(parent);
    lv_obj_set_width(list, LV_PCT(100));
    lv_obj_set_style_bg_color(list, lv_color_hex(THEME_COLOR_SURFACE), 0);
    lv_obj_set_style_radius(list, 8, 0);
    lv_obj_set_style_pad_all(list, 15, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 15, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);

    // Create all settings
    create_can_baud_setting(list);
    create_brightness_setting(list);
    create_theme_setting(list);
    create_table_update_setting(list);
    create_auto_scroll_setting(list);
    create_auto_log_setting(list);
    create_splash_setting(list);

    return list;
}

lv_obj_t* ScreenSettings::create_setting_row(lv_obj_t* parent, const char* title, const char* description) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 5, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // Text container (left side)
    lv_obj_t* text_cont = lv_obj_create(row);
    lv_obj_set_style_bg_opa(text_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(text_cont, 0, 0);
    lv_obj_set_style_pad_all(text_cont, 0, 0);
    lv_obj_set_flex_flow(text_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(text_cont, 1);
    lv_obj_clear_flag(text_cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title_label = lv_label_create(text_cont);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    if (description) {
        lv_obj_t* desc_label = lv_label_create(text_cont);
        lv_label_set_text(desc_label, description);
        lv_obj_set_style_text_font(desc_label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(desc_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    }

    return row;
}

void ScreenSettings::create_can_baud_setting(lv_obj_t* parent) {
    lv_obj_t* row = create_setting_row(parent, "CAN Speed", "Bus communication speed");

    can_baud_dropdown = lv_dropdown_create(row);
    lv_dropdown_set_options(can_baud_dropdown,
        "1 Mbps\n"
        "800 kbps\n"
        "500 kbps\n"
        "250 kbps\n"
        "125 kbps\n"
        "100 kbps\n"
        "50 kbps\n"
        "20 kbps"
    );
    lv_obj_set_width(can_baud_dropdown, 150);
    lv_dropdown_set_selected(can_baud_dropdown, baud_rate_to_index(temp_settings.can_baudrate));
    lv_obj_add_event_cb(can_baud_dropdown, can_baud_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

void ScreenSettings::create_brightness_setting(lv_obj_t* parent) {
    lv_obj_t* row = create_setting_row(parent, "Screen Brightness", nullptr);

    lv_obj_t* slider_cont = lv_obj_create(row);
    lv_obj_set_size(slider_cont, 200, 50);
    lv_obj_set_style_bg_opa(slider_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(slider_cont, 0, 0);
    lv_obj_set_style_pad_all(slider_cont, 0, 0);
    lv_obj_set_flex_flow(slider_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(slider_cont, LV_OBJ_FLAG_SCROLLABLE);

    brightness_slider = lv_slider_create(slider_cont);
    lv_obj_set_width(brightness_slider, LV_PCT(100));
    lv_slider_set_range(brightness_slider, 50, 255);
    lv_slider_set_value(brightness_slider, temp_settings.brightness, LV_ANIM_OFF);
    lv_obj_add_event_cb(brightness_slider, brightness_cb, LV_EVENT_VALUE_CHANGED, NULL);

    brightness_label = lv_label_create(slider_cont);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", (temp_settings.brightness * 100) / 255);
    lv_label_set_text(brightness_label, buf);
    lv_obj_set_style_text_font(brightness_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(brightness_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
}

void ScreenSettings::create_theme_setting(lv_obj_t* parent) {
    lv_obj_t* row = create_setting_row(parent, "Dark Mode", "Use dark color theme");

    theme_switch = lv_switch_create(row);
    if (temp_settings.theme == ThemeMode::DARK) {
        lv_obj_add_state(theme_switch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(theme_switch, theme_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

void ScreenSettings::create_table_update_setting(lv_obj_t* parent) {
    lv_obj_t* row = create_setting_row(parent, "Table Update Speed", "Lower = faster updates");

    lv_obj_t* slider_cont = lv_obj_create(row);
    lv_obj_set_size(slider_cont, 200, 50);
    lv_obj_set_style_bg_opa(slider_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(slider_cont, 0, 0);
    lv_obj_set_style_pad_all(slider_cont, 0, 0);
    lv_obj_set_flex_flow(slider_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(slider_cont, LV_OBJ_FLAG_SCROLLABLE);

    table_update_slider = lv_slider_create(slider_cont);
    lv_obj_set_width(table_update_slider, LV_PCT(100));
    lv_slider_set_range(table_update_slider, 50, 500);
    lv_slider_set_value(table_update_slider, temp_settings.table_update_ms, LV_ANIM_OFF);
    lv_obj_add_event_cb(table_update_slider, table_update_cb, LV_EVENT_VALUE_CHANGED, NULL);

    table_update_label = lv_label_create(slider_cont);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d ms", temp_settings.table_update_ms);
    lv_label_set_text(table_update_label, buf);
    lv_obj_set_style_text_font(table_update_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(table_update_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
}

void ScreenSettings::create_auto_scroll_setting(lv_obj_t* parent) {
    lv_obj_t* row = create_setting_row(parent, "Auto Scroll", "Scroll to new messages");

    auto_scroll_switch = lv_switch_create(row);
    if (temp_settings.auto_scroll) {
        lv_obj_add_state(auto_scroll_switch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(auto_scroll_switch, auto_scroll_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

void ScreenSettings::create_auto_log_setting(lv_obj_t* parent) {
    lv_obj_t* row = create_setting_row(parent, "Auto-Log on Start", "Enable logging when starting");

    auto_log_switch = lv_switch_create(row);
    if (temp_settings.auto_log_on_start) {
        lv_obj_add_state(auto_log_switch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(auto_log_switch, auto_log_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

void ScreenSettings::create_splash_setting(lv_obj_t* parent) {
    lv_obj_t* row = create_setting_row(parent, "Show Splash Screen", "Display splash on boot");

    splash_switch = lv_switch_create(row);
    if (temp_settings.show_splash) {
        lv_obj_add_state(splash_switch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(splash_switch, splash_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

lv_obj_t* ScreenSettings::create_button_row(lv_obj_t* parent) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, 80);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // Cancel button
    lv_obj_t* cancel_btn = lv_btn_create(row);
    lv_obj_set_size(cancel_btn, 250, 60);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(THEME_COLOR_ERROR), 0);
    lv_obj_add_event_cb(cancel_btn, cancel_button_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, LV_SYMBOL_CLOSE " Cancel");
    lv_obj_set_style_text_font(cancel_label, &lv_font_montserrat_24, 0);
    lv_obj_center(cancel_label);

    // Save button
    lv_obj_t* save_btn = lv_btn_create(row);
    lv_obj_set_size(save_btn, 250, 60);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(THEME_COLOR_SUCCESS), 0);
    lv_obj_add_event_cb(save_btn, save_button_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, LV_SYMBOL_SAVE " Save");
    lv_obj_set_style_text_font(save_label, &lv_font_montserrat_24, 0);
    lv_obj_center(save_label);

    return row;
}

void ScreenSettings::update(lv_obj_t* screen) {
    // No periodic updates needed for settings screen
}

void ScreenSettings::cleanup() {
    can_baud_dropdown = nullptr;
    brightness_slider = nullptr;
    brightness_label = nullptr;
    theme_switch = nullptr;
    auto_scroll_switch = nullptr;
    table_update_slider = nullptr;
    table_update_label = nullptr;
    auto_log_switch = nullptr;
    splash_switch = nullptr;
}

// Event Callbacks


void ScreenSettings::save_button_cb(lv_event_t* e) {
    Serial.println("Saving settings...");

    // Save settings to persistent storage
    if (SettingsManager::save(temp_settings)) {
        // Apply settings
        SettingsManager::apply(temp_settings);

        UIManager::show_notification("Settings saved");
        Serial.println("Settings saved successfully");

        // Note: CAN baud rate change requires restart or manual CAN bus restart
        // We'll just save it for next boot
    } else {
        UIManager::show_notification("Failed to save settings");
        Serial.println("ERROR: Failed to save settings");
    }
    UIManager::navigate_to(Screen::MAIN_DASHBOARD);

}

void ScreenSettings::cancel_button_cb(lv_event_t* e) {
    Serial.println("Cancelling settings changes");
    UIManager::show_notification("Changes discarded");
    UIManager::navigate_to(Screen::MAIN_DASHBOARD);
}

void ScreenSettings::can_baud_cb(lv_event_t* e) {
    uint16_t selected = lv_dropdown_get_selected(can_baud_dropdown);
    temp_settings.can_baudrate = index_to_baud_rate(selected);
    Serial.printf("CAN baud changed to: %lu\n", static_cast<uint32_t>(temp_settings.can_baudrate));
}

void ScreenSettings::brightness_cb(lv_event_t* e) {
    int32_t value = lv_slider_get_value(brightness_slider);
    temp_settings.brightness = (uint8_t)value;

    // Update label
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", (value * 100) / 255);
    lv_label_set_text(brightness_label, buf);

    // Apply immediately for preview
    DisplayHAL::set_brightness((uint8_t)value);
}

void ScreenSettings::theme_cb(lv_event_t* e) {
    bool checked = lv_obj_has_state(theme_switch, LV_STATE_CHECKED);
    temp_settings.theme = checked ? ThemeMode::DARK : ThemeMode::LIGHT;
    Serial.printf("Theme changed to: %s\n", checked ? "Dark" : "Light");
    // Note: Theme change requires screen recreation to take effect
}

void ScreenSettings::table_update_cb(lv_event_t* e) {
    int32_t value = lv_slider_get_value(table_update_slider);
    temp_settings.table_update_ms = (uint16_t)value;

    // Update label
    char buf[16];
    snprintf(buf, sizeof(buf), "%d ms", value);
    lv_label_set_text(table_update_label, buf);
}

void ScreenSettings::auto_scroll_cb(lv_event_t* e) {
    temp_settings.auto_scroll = lv_obj_has_state(auto_scroll_switch, LV_STATE_CHECKED);
}

void ScreenSettings::auto_log_cb(lv_event_t* e) {
    temp_settings.auto_log_on_start = lv_obj_has_state(auto_log_switch, LV_STATE_CHECKED);
}

void ScreenSettings::splash_cb(lv_event_t* e) {
    temp_settings.show_splash = lv_obj_has_state(splash_switch, LV_STATE_CHECKED);
}

// Helper functions

uint8_t ScreenSettings::baud_rate_to_index(CANBaudRate baud) {
    switch (baud) {
        case CANBaudRate::BAUD_1M:    return 0;
        case CANBaudRate::BAUD_800K:  return 1;
        case CANBaudRate::BAUD_500K:  return 2;
        case CANBaudRate::BAUD_250K:  return 3;
        case CANBaudRate::BAUD_125K:  return 4;
        case CANBaudRate::BAUD_100K:  return 5;
        case CANBaudRate::BAUD_50K:   return 6;
        case CANBaudRate::BAUD_20K:   return 7;
        default: return 3; // Default to 250K
    }
}

CANBaudRate ScreenSettings::index_to_baud_rate(uint8_t index) {
    switch (index) {
        case 0: return CANBaudRate::BAUD_1M;
        case 1: return CANBaudRate::BAUD_800K;
        case 2: return CANBaudRate::BAUD_500K;
        case 3: return CANBaudRate::BAUD_250K;
        case 4: return CANBaudRate::BAUD_125K;
        case 5: return CANBaudRate::BAUD_100K;
        case 6: return CANBaudRate::BAUD_50K;
        case 7: return CANBaudRate::BAUD_20K;
        default: return CANBaudRate::BAUD_250K;
    }
}

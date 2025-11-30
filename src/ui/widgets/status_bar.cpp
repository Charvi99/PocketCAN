#include "status_bar.h"
#include "../themes/theme_colors.h"
#include "../../config/app_config.h"
#include <Arduino.h>

lv_obj_t* StatusBar::create(lv_obj_t* parent) {
    // Create container for status bar
    lv_obj_t* bar = lv_obj_create(parent);
    lv_obj_set_size(bar, LV_PCT(100), STATUS_BAR_HEIGHT);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(THEME_COLOR_SURFACE), 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 8, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    // Create flex layout (horizontal)
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Time label (left) - 50% larger: 16 -> 24
    lv_obj_t* time_label = lv_label_create(bar);
    lv_label_set_text(time_label, "--:--");
    lv_obj_set_style_text_color(time_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_24, 0);

    // CAN status (center) - 50% larger: 14 -> 20
    lv_obj_t* can_status = lv_label_create(bar);
    lv_label_set_text(can_status, LV_SYMBOL_USB " CAN: --");
    lv_obj_set_style_text_color(can_status, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_text_font(can_status, &lv_font_montserrat_20, 0);

    // Battery indicator (right) - 50% larger: 14 -> 20
    lv_obj_t* battery_label = lv_label_create(bar);
    lv_label_set_text(battery_label, LV_SYMBOL_BATTERY_FULL " --%");
    lv_obj_set_style_text_color(battery_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_20, 0);

    return bar;
}

void StatusBar::update(lv_obj_t* bar) {
    if (!bar) return;

    // Get current time from system
    uint32_t uptime_sec = millis() / 1000;
    uint8_t hours = (uptime_sec / 3600) % 24;
    uint8_t minutes = (uptime_sec / 60) % 60;
    set_time(bar, hours, minutes);

    // TODO: Update battery from hardware
    // TODO: Update CAN status from state manager
}

void StatusBar::set_battery(lv_obj_t* bar, uint8_t percent, bool charging) {
    if (!bar) return;
    lv_obj_t* battery_label = lv_obj_get_child(bar, IDX_BATTERY);
    if (!battery_label) return;

    const char* battery_icon;
    if (charging) {
        battery_icon = LV_SYMBOL_CHARGE;
    } else if (percent > 75) {
        battery_icon = LV_SYMBOL_BATTERY_FULL;
    } else if (percent > 50) {
        battery_icon = LV_SYMBOL_BATTERY_3;
    } else if (percent > 25) {
        battery_icon = LV_SYMBOL_BATTERY_2;
    } else if (percent > 10) {
        battery_icon = LV_SYMBOL_BATTERY_1;
    } else {
        battery_icon = LV_SYMBOL_BATTERY_EMPTY;
    }

    lv_label_set_text_fmt(battery_label, "%s %d%%", battery_icon, percent);

    // Change color if low battery
    if (percent < 20 && !charging) {
        lv_obj_set_style_text_color(battery_label, lv_color_hex(THEME_COLOR_WARNING), 0);
    } else {
        lv_obj_set_style_text_color(battery_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    }
}

void StatusBar::set_can_status(lv_obj_t* bar, bool connected, uint32_t baudrate) {
    if (!bar) return;
    lv_obj_t* can_label = lv_obj_get_child(bar, IDX_CAN_STATUS);
    if (!can_label) return;

    if (connected) {
        lv_label_set_text_fmt(can_label, LV_SYMBOL_USB " CAN: %dK", baudrate / 1000);
        lv_obj_set_style_text_color(can_label, lv_color_hex(THEME_COLOR_SUCCESS), 0);
    } else {
        lv_label_set_text(can_label, LV_SYMBOL_USB " CAN: OFF");
        lv_obj_set_style_text_color(can_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    }
}

void StatusBar::set_time(lv_obj_t* bar, uint8_t hours, uint8_t minutes) {
    if (!bar) return;
    lv_obj_t* time_label = lv_obj_get_child(bar, IDX_TIME);
    if (!time_label) return;
    lv_label_set_text_fmt(time_label, "%02d:%02d", hours, minutes);
}

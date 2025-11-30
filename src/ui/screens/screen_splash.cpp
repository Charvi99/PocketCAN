#include "screen_splash.h"
#include "../themes/theme_colors.h"
#include "../../config/app_config.h"
#include <Arduino.h>

lv_obj_t* ScreenSplash::create(uint32_t duration_ms) {
    // Create splash screen
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    // App name (centered)
    lv_obj_t* name = lv_label_create(screen);
    lv_label_set_text(name, "PocketCAN");
    lv_obj_set_style_text_font(name, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(name, lv_color_hex(THEME_COLOR_PRIMARY), 0);
    lv_obj_align(name, LV_ALIGN_CENTER, 0, -60);

    // Tagline
    lv_obj_t* tagline = lv_label_create(screen);
    lv_label_set_text(tagline, "Professional CAN Bus Analyzer");
    lv_obj_set_style_text_font(tagline, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(tagline, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_align(tagline, LV_ALIGN_CENTER, 0, 0);

    // Version
    char version_str[32];
    snprintf(version_str, sizeof(version_str), "v%d.%d.%d",
             APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);
    lv_obj_t* version = lv_label_create(screen);
    lv_label_set_text(version, version_str);
    lv_obj_set_style_text_font(version, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(version, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_align(version, LV_ALIGN_CENTER, 0, 50);

    // Load screen
    lv_scr_load(screen);

    Serial.println("Splash screen displayed");
    return screen;
}

void ScreenSplash::dismiss(lv_obj_t* splash) {
    if (splash) {
        Serial.println("Dismissing splash screen");
        // Splash will be deleted by UI manager when loading next screen
    }
}

void ScreenSplash::timer_cb(lv_timer_t* timer) {
    lv_obj_t* splash = (lv_obj_t*)timer->user_data;
    dismiss(splash);
    // Note: UIManager should transition to main screen
}

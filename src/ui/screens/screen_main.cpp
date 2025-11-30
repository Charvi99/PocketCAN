#include "screen_main.h"
#include "../widgets/status_bar.h"
#include "../themes/theme_colors.h"
#include "../../config/app_config.h"
#include "../../services/state_manager.h"
#include <Arduino.h>

lv_obj_t* ScreenMain::menu_panel = nullptr;

lv_obj_t* ScreenMain::create() {
    // Create main screen
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    // Create content container (below status bar)
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_height(content, 720 - STATUS_BAR_HEIGHT);  // 680px
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_color(content, lv_color_hex(THEME_COLOR_BACKGROUND), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 20, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Create navigation grid
    lv_obj_t* grid = create_nav_grid(content);
    lv_obj_align(grid, LV_ALIGN_CENTER, 0, 0);

    return screen;
}

void ScreenMain::update(lv_obj_t* screen) {

}


lv_obj_t* ScreenMain::create_nav_grid(lv_obj_t* parent) {
    // Create container for grid
    lv_obj_t* grid_cont = lv_obj_create(parent);
    lv_obj_set_size(grid_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(grid_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid_cont, 0, 0);
    lv_obj_set_style_pad_all(grid_cont, 10, 0);
    lv_obj_clear_flag(grid_cont, LV_OBJ_FLAG_SCROLLABLE);

    // Set up grid layout (2 columns)
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid_cont, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(grid_cont, 20, 0);
    lv_obj_set_style_pad_row(grid_cont, 20, 0);

    // Create navigation tiles
    lv_obj_t* tile1 = create_nav_tile(grid_cont, LV_SYMBOL_EYE_OPEN, "CAN Sniffer",
                                       "Monitor bus", THEME_COLOR_PRIMARY, sniffer_tile_cb);
    lv_obj_set_grid_cell(tile1, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    lv_obj_t* tile2 = create_nav_tile(grid_cont, LV_SYMBOL_UPLOAD, "Transmit",
                                       "Send messages", THEME_COLOR_ACCENT, transmit_tile_cb);
    lv_obj_set_grid_cell(tile2, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    lv_obj_t* tile3 = create_nav_tile(grid_cont, LV_SYMBOL_SHUFFLE, "Emulator",
                                       "Device simulation", THEME_COLOR_SUCCESS, emulator_tile_cb);
    lv_obj_set_grid_cell(tile3, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    lv_obj_t* tile4 = create_nav_tile(grid_cont, LV_SYMBOL_SETTINGS, "Settings",
                                       "Configuration", THEME_COLOR_TEXT_SECONDARY, settings_tile_cb);
    lv_obj_set_grid_cell(tile4, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    return grid_cont;
}

lv_obj_t* ScreenMain::create_nav_tile(lv_obj_t* parent, const char* icon, const char* title,
                                       const char* subtitle, uint32_t color, lv_event_cb_t callback) {
    // Create tile button
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 300, 300);
    lv_obj_set_style_bg_color(btn, lv_color_hex(THEME_COLOR_SURFACE), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(THEME_COLOR_SURFACE_VARIANT), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn, 12, 0);
    lv_obj_set_style_shadow_width(btn, 10, 0);
    lv_obj_set_style_shadow_color(btn, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);
    lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, NULL);


    // Create content container
    lv_obj_t* cont = lv_obj_create(btn);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 20, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_CLICKABLE);

    // Icon (50% larger)
    lv_obj_t* icon_label = lv_label_create(cont);
    lv_label_set_text(icon_label, icon);
    lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(icon_label, lv_color_hex(color), 0);
    lv_obj_clear_flag(icon_label, LV_OBJ_FLAG_CLICKABLE);

    // Title (50% larger: 22 -> 32)
    lv_obj_t* title_label = lv_label_create(cont);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_pad_top(title_label, 10, 0);
    lv_obj_clear_flag(title_label, LV_OBJ_FLAG_CLICKABLE);

    // Subtitle (50% larger: 14 -> 20)
    lv_obj_t* subtitle_label = lv_label_create(cont);
    lv_label_set_text(subtitle_label, subtitle);
    lv_obj_set_style_text_font(subtitle_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(subtitle_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_clear_flag(subtitle_label, LV_OBJ_FLAG_CLICKABLE);

    return btn;
}

// Event callbacks
void ScreenMain::menu_button_cb(lv_event_t* e) {
    // TODO: Show side menu
    Serial.println("Menu button clicked");
    UIManager::navigate_to(Screen::MAIN_DASHBOARD);

}

void ScreenMain::sniffer_tile_cb(lv_event_t* e) {
    Serial.println("Navigating to Sniffer...");
    // TODO: Navigate to sniffer screen
    UIManager::navigate_to(Screen::SNIFFER);
}

void ScreenMain::transmit_tile_cb(lv_event_t* e) {
    Serial.println("Navigating to Transmit...");
    // TODO: Navigate to transmit screen
}

void ScreenMain::emulator_tile_cb(lv_event_t* e) {
    Serial.println("Navigating to Emulator...");
    // TODO: Navigate to emulator screen
}

void ScreenMain::settings_tile_cb(lv_event_t* e) {
    Serial.println("Navigating to Settings...");
    // TODO: Navigate to settings screen
}

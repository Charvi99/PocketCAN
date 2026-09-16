#include "demo_ui.h"

// Helper function to create a styled button with a label
static lv_obj_t * create_styled_button(lv_obj_t * parent, const char * text) {
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, lv_pct(48), lv_pct(31)); // ~48% width, ~31% height to fit 2x3 grid with spacing
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x2c2c2c), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btn, 12, LV_STATE_DEFAULT);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

void create_dashboard(lv_obj_t * parent) {
    // Create a top status bar
    lv_obj_t * top_bar = lv_obj_create(parent);
    lv_obj_set_size(top_bar, lv_pct(100), 30);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x1a1a1a), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(top_bar, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(top_bar, 0, LV_STATE_DEFAULT);


    lv_obj_t * title = lv_label_create(top_bar);
    lv_label_set_text(title, "PocketCAN");
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_t * status_label = lv_label_create(top_bar);
    lv_label_set_text(status_label, "Status: Disconnected");
    lv_obj_align(status_label, LV_ALIGN_RIGHT_MID, -10, 0);

    // Create a container for the buttons with a flex layout
    lv_obj_t * content = lv_obj_create(parent);
    lv_obj_set_size(content, lv_pct(100), lv_pct(100));
    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_top(content, 40, 0); // Add padding to not overlap with status bar
    lv_obj_set_style_pad_bottom(content, 10, 0);
    lv_obj_set_style_pad_hor(content, 10, 0);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x111111), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(content, 0, LV_STATE_DEFAULT);


    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Create the 6 buttons
    create_styled_button(content, "Sniffer");
    create_styled_button(content, "Transmit");
    create_styled_button(content, "Filters");
    create_styled_button(content, "Emulator");
    create_styled_button(content, "Scope");
    create_styled_button(content, "Settings");
}

void demo_ui_init(void) {
    lv_disp_t * disp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
    lv_disp_set_theme(disp, theme);

    lv_obj_t * screen = lv_disp_get_scr_act(disp);
    lv_obj_clean(screen); // Clean any existing objects
    create_dashboard(screen);
}

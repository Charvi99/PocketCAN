#include "ui_manager.h"
#include "themes/theme_colors.h"
#include "screens/screen_main.h"
#include "screens/screen_sniffer.h"
#include "screens/screen_splash.h"
#include "screens/screen_settings.h"
#include "screens/screen_transmit.h"
#include "screens/screen_scope.h"
#include "../services/settings_manager.h"
#include <Arduino.h>

Screen UIManager::current_screen = Screen::MAIN_DASHBOARD;
lv_obj_t* UIManager::active_screen_obj = nullptr;
lv_obj_t* UIManager::status_bar = nullptr;
uint32_t UIManager::splash_start_time = 0;
bool UIManager::splash_shown = false;

bool UIManager::init()
{
    // Create status bar on the top layer (always visible)
    status_bar = StatusBar::create(lv_layer_top());

    // Create home button in status bar
    lv_obj_t* home_btn = lv_btn_create(status_bar);
    lv_obj_set_size(home_btn, 48, 48);
    lv_obj_align(home_btn, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_set_style_bg_color(home_btn, lv_color_hex(THEME_COLOR_SURFACE_VARIANT), 0);
    lv_obj_set_style_radius(home_btn, 8, 0);
    lv_obj_add_event_cb(home_btn, home_button_cb, LV_EVENT_CLICKED, NULL);

    // Home icon
    lv_obj_t* icon = lv_label_create(home_btn);
    lv_label_set_text(icon, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(THEME_COLOR_PRIMARY), 0);
    lv_obj_center(icon);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);

    // Check if splash screen should be shown
    const AppSettings& settings = SettingsManager::get();
    if (settings.show_splash) {
        // Show splash screen for 2 seconds
        create_splash_screen();
        splash_start_time = millis();
        splash_shown = true;
        Serial.println("Showing splash screen");
    } else {
        // Show main dashboard directly
        create_main_dashboard();
        splash_shown = false;
        Serial.println("Skipping splash screen (disabled in settings)");
    }

    Serial.println("UI Manager initialized");
    return true;
}

// Event callback for home button
void UIManager::home_button_cb(lv_event_t* e) {
    Serial.println("Home button clicked. Navigating to Main Dashboard.");
    UIManager::navigate_to(Screen::MAIN_DASHBOARD);
}


void UIManager::navigate_to(Screen screen)
{
    current_screen = screen;
    lv_obj_t * old_screen = lv_scr_act(); // Get a pointer to the current screen


    // Create new screen
    switch (screen)
    {
    case Screen::MAIN_DASHBOARD:
        create_main_dashboard();
        break;
    case Screen::SNIFFER:
        create_sniffer_screen();

        break;
    case Screen::TRANSMIT:
        create_transmit_screen();
        break;
    case Screen::SETTINGS:
        create_settings_screen();
        break;
    case Screen::SCOPE:
        create_scope_screen();
        break;
    default:
        Serial.println("WARNING: Unknown screen");
        create_main_dashboard();
        break;
    }
    if(old_screen != nullptr && old_screen != lv_scr_act())
    {
        lv_obj_del(old_screen);
    }
}

Screen UIManager::get_current_screen()
{
    return current_screen;
}

void UIManager::update()
{
    // Check if splash screen timeout has elapsed
    if (splash_shown && (millis() - splash_start_time >= 2000)) {
        Serial.println("Splash screen timeout - transitioning to dashboard");
        splash_shown = false;
        navigate_to(Screen::MAIN_DASHBOARD);
        return;
    }

    // Update global status bar
    if (status_bar) {
        StatusBar::update(status_bar);

        // Update CAN status from state manager
        const ApplicationState& state = StateManager::get_state();
        bool can_connected = (state.can_state == CANBusState::RUNNING);
        StatusBar::set_can_status(status_bar, can_connected, static_cast<uint32_t>(state.can_baudrate));

        // Update battery (mock for now)
        StatusBar::set_battery(status_bar, 85, false);
    }

    // Update current screen
    if (active_screen_obj)
    {
        switch (current_screen)
        {
        case Screen::MAIN_DASHBOARD:
            ScreenMain::update(active_screen_obj);
            break;

        case Screen::SNIFFER:
            ScreenSniffer::update(active_screen_obj);
            break;
        case Screen::TRANSMIT:
            ScreenTransmit::update(active_screen_obj);
            break;
        case Screen::EMULATOR:

            break;
        case Screen::FILTER:

            break;
        case Screen::SETTINGS:
            ScreenSettings::update(active_screen_obj);
            break;
        case Screen::SCOPE:
            ScreenScope::update(active_screen_obj);
            break;

        default:
            break;
        }
    }
}

void UIManager::show_notification(const char *message, uint32_t duration_ms)
{
    // TODO: Implement toast-style notifications
    Serial.printf("NOTIFICATION: %s\n", message);
}

void UIManager::create_splash_screen()
{
    active_screen_obj = ScreenSplash::create(0); // 0 = manual dismiss via update()
}

void UIManager::create_main_dashboard()
{
    active_screen_obj = ScreenMain::create();
    lv_scr_load(active_screen_obj);
}

void UIManager::create_sniffer_screen()
{
    active_screen_obj = ScreenSniffer::create();
    lv_scr_load(active_screen_obj);
}

void UIManager::create_transmit_screen()
{
    active_screen_obj = ScreenTransmit::create();
    lv_scr_load(active_screen_obj);
}

void UIManager::create_settings_screen()
{
    active_screen_obj = ScreenSettings::create();
    lv_scr_load(active_screen_obj);
}

void UIManager::create_scope_screen()
{
    active_screen_obj = ScreenScope::create();
    lv_scr_load(active_screen_obj);
}

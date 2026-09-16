#include "theme_manager.h"
#include <Arduino.h>

ThemeMode ThemeManager::current_theme = ThemeMode::DARK;

void ThemeManager::init(ThemeMode mode) {
    current_theme = mode;
    Serial.printf("Theme Manager initialized: %s mode\n",
                  mode == ThemeMode::DARK ? "Dark" : "Light");
}

void ThemeManager::set_theme(ThemeMode mode) {
    if (current_theme != mode) {
        current_theme = mode;
        Serial.printf("Theme changed to: %s mode\n",
                      mode == ThemeMode::DARK ? "Dark" : "Light");
    }
}

ThemeMode ThemeManager::get_current_theme() {
    return current_theme;
}

// Background Colors
uint32_t ThemeManager::get_background() {
    return current_theme == ThemeMode::DARK ? DarkTheme::BACKGROUND : LightTheme::BACKGROUND;
}

uint32_t ThemeManager::get_surface() {
    return current_theme == ThemeMode::DARK ? DarkTheme::SURFACE : LightTheme::SURFACE;
}

uint32_t ThemeManager::get_surface_variant() {
    return current_theme == ThemeMode::DARK ? DarkTheme::SURFACE_VARIANT : LightTheme::SURFACE_VARIANT;
}

// Primary Colors
uint32_t ThemeManager::get_primary() {
    return current_theme == ThemeMode::DARK ? DarkTheme::PRIMARY : LightTheme::PRIMARY;
}

uint32_t ThemeManager::get_primary_dark() {
    return current_theme == ThemeMode::DARK ? DarkTheme::PRIMARY_DARK : LightTheme::PRIMARY_DARK;
}

uint32_t ThemeManager::get_primary_light() {
    return current_theme == ThemeMode::DARK ? DarkTheme::PRIMARY_LIGHT : LightTheme::PRIMARY_LIGHT;
}

// Accent Colors
uint32_t ThemeManager::get_accent() {
    return current_theme == ThemeMode::DARK ? DarkTheme::ACCENT : LightTheme::ACCENT;
}

uint32_t ThemeManager::get_warning() {
    return current_theme == ThemeMode::DARK ? DarkTheme::WARNING : LightTheme::WARNING;
}

uint32_t ThemeManager::get_error() {
    return current_theme == ThemeMode::DARK ? DarkTheme::ERROR : LightTheme::ERROR;
}

uint32_t ThemeManager::get_success() {
    return current_theme == ThemeMode::DARK ? DarkTheme::SUCCESS : LightTheme::SUCCESS;
}

// Text Colors
uint32_t ThemeManager::get_text_primary() {
    return current_theme == ThemeMode::DARK ? DarkTheme::TEXT_PRIMARY : LightTheme::TEXT_PRIMARY;
}

uint32_t ThemeManager::get_text_secondary() {
    return current_theme == ThemeMode::DARK ? DarkTheme::TEXT_SECONDARY : LightTheme::TEXT_SECONDARY;
}

uint32_t ThemeManager::get_text_disabled() {
    return current_theme == ThemeMode::DARK ? DarkTheme::TEXT_DISABLED : LightTheme::TEXT_DISABLED;
}

// CAN-specific Colors
uint32_t ThemeManager::get_can_tx() {
    return current_theme == ThemeMode::DARK ? DarkTheme::CAN_TX : LightTheme::CAN_TX;
}

uint32_t ThemeManager::get_can_rx() {
    return current_theme == ThemeMode::DARK ? DarkTheme::CAN_RX : LightTheme::CAN_RX;
}

uint32_t ThemeManager::get_can_error() {
    return current_theme == ThemeMode::DARK ? DarkTheme::CAN_ERROR : LightTheme::CAN_ERROR;
}

uint32_t ThemeManager::get_can_rtr() {
    return current_theme == ThemeMode::DARK ? DarkTheme::CAN_RTR : LightTheme::CAN_RTR;
}

// Chart/Graph Colors
uint32_t ThemeManager::get_chart_line1() {
    return current_theme == ThemeMode::DARK ? DarkTheme::CHART_LINE1 : LightTheme::CHART_LINE1;
}

uint32_t ThemeManager::get_chart_line2() {
    return current_theme == ThemeMode::DARK ? DarkTheme::CHART_LINE2 : LightTheme::CHART_LINE2;
}

uint32_t ThemeManager::get_chart_line3() {
    return current_theme == ThemeMode::DARK ? DarkTheme::CHART_LINE3 : LightTheme::CHART_LINE3;
}

uint32_t ThemeManager::get_chart_grid() {
    return current_theme == ThemeMode::DARK ? DarkTheme::CHART_GRID : LightTheme::CHART_GRID;
}

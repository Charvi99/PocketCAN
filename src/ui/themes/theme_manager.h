#pragma once

/**
 * Theme Manager
 * Manages light and dark theme switching
 */

#include <cstdint>
#include "../../services/settings_manager.h"

class ThemeManager {
public:
    /**
     * Initialize theme manager
     * @param mode Initial theme mode
     */
    static void init(ThemeMode mode = ThemeMode::DARK);

    /**
     * Set current theme
     * @param mode Theme mode to apply
     */
    static void set_theme(ThemeMode mode);

    /**
     * Get current theme mode
     * @return Current theme mode
     */
    static ThemeMode get_current_theme();

    // Background Colors
    static uint32_t get_background();
    static uint32_t get_surface();
    static uint32_t get_surface_variant();

    // Primary Colors
    static uint32_t get_primary();
    static uint32_t get_primary_dark();
    static uint32_t get_primary_light();

    // Accent Colors
    static uint32_t get_accent();
    static uint32_t get_warning();
    static uint32_t get_error();
    static uint32_t get_success();

    // Text Colors
    static uint32_t get_text_primary();
    static uint32_t get_text_secondary();
    static uint32_t get_text_disabled();

    // CAN-specific Colors
    static uint32_t get_can_tx();
    static uint32_t get_can_rx();
    static uint32_t get_can_error();
    static uint32_t get_can_rtr();

    // Chart/Graph Colors
    static uint32_t get_chart_line1();
    static uint32_t get_chart_line2();
    static uint32_t get_chart_line3();
    static uint32_t get_chart_grid();

private:
    static ThemeMode current_theme;

    // Dark theme colors
    struct DarkTheme {
        static constexpr uint32_t BACKGROUND = 0x1a1a1a;
        static constexpr uint32_t SURFACE = 0x2d2d2d;
        static constexpr uint32_t SURFACE_VARIANT = 0x424242;

        static constexpr uint32_t PRIMARY = 0x00BCD4;
        static constexpr uint32_t PRIMARY_DARK = 0x0097A7;
        static constexpr uint32_t PRIMARY_LIGHT = 0x4DD0E1;

        static constexpr uint32_t ACCENT = 0xFF5722;
        static constexpr uint32_t WARNING = 0xFFC107;
        static constexpr uint32_t ERROR = 0xF44336;
        static constexpr uint32_t SUCCESS = 0x4CAF50;

        static constexpr uint32_t TEXT_PRIMARY = 0xFFFFFF;
        static constexpr uint32_t TEXT_SECONDARY = 0xB0B0B0;
        static constexpr uint32_t TEXT_DISABLED = 0x606060;

        static constexpr uint32_t CAN_TX = 0xFF9800;
        static constexpr uint32_t CAN_RX = 0x2196F3;
        static constexpr uint32_t CAN_ERROR = 0xF44336;
        static constexpr uint32_t CAN_RTR = 0x9C27B0;

        static constexpr uint32_t CHART_LINE1 = 0x00BCD4;
        static constexpr uint32_t CHART_LINE2 = 0xFF5722;
        static constexpr uint32_t CHART_LINE3 = 0x4CAF50;
        static constexpr uint32_t CHART_GRID = 0x404040;
    };

    // Light theme colors
    struct LightTheme {
        static constexpr uint32_t BACKGROUND = 0xF5F5F5;
        static constexpr uint32_t SURFACE = 0xFFFFFF;
        static constexpr uint32_t SURFACE_VARIANT = 0xE0E0E0;

        static constexpr uint32_t PRIMARY = 0x0097A7;
        static constexpr uint32_t PRIMARY_DARK = 0x00838F;
        static constexpr uint32_t PRIMARY_LIGHT = 0x00BCD4;

        static constexpr uint32_t ACCENT = 0xFF5722;
        static constexpr uint32_t WARNING = 0xFF9800;
        static constexpr uint32_t ERROR = 0xD32F2F;
        static constexpr uint32_t SUCCESS = 0x388E3C;

        static constexpr uint32_t TEXT_PRIMARY = 0x212121;
        static constexpr uint32_t TEXT_SECONDARY = 0x757575;
        static constexpr uint32_t TEXT_DISABLED = 0xBDBDBD;

        static constexpr uint32_t CAN_TX = 0xF57C00;
        static constexpr uint32_t CAN_RX = 0x1976D2;
        static constexpr uint32_t CAN_ERROR = 0xD32F2F;
        static constexpr uint32_t CAN_RTR = 0x7B1FA2;

        static constexpr uint32_t CHART_LINE1 = 0x0097A7;
        static constexpr uint32_t CHART_LINE2 = 0xFF5722;
        static constexpr uint32_t CHART_LINE3 = 0x388E3C;
        static constexpr uint32_t CHART_GRID = 0xE0E0E0;
    };
};

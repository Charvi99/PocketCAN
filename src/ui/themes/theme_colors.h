#pragma once

/**
 * Theme Colors
 * Dynamic theme color system with light/dark mode support
 * Uses ThemeManager for runtime theme switching
 */

#include "theme_manager.h"

// Background Colors
#define THEME_COLOR_BACKGROUND      ThemeManager::get_background()
#define THEME_COLOR_SURFACE         ThemeManager::get_surface()
#define THEME_COLOR_SURFACE_VARIANT ThemeManager::get_surface_variant()

// Primary Colors
#define THEME_COLOR_PRIMARY         ThemeManager::get_primary()
#define THEME_COLOR_PRIMARY_DARK    ThemeManager::get_primary_dark()
#define THEME_COLOR_PRIMARY_LIGHT   ThemeManager::get_primary_light()

// Accent Colors
#define THEME_COLOR_ACCENT          ThemeManager::get_accent()
#define THEME_COLOR_WARNING         ThemeManager::get_warning()
#define THEME_COLOR_ERROR           ThemeManager::get_error()
#define THEME_COLOR_SUCCESS         ThemeManager::get_success()

// Text Colors
#define THEME_COLOR_TEXT_PRIMARY    ThemeManager::get_text_primary()
#define THEME_COLOR_TEXT_SECONDARY  ThemeManager::get_text_secondary()
#define THEME_COLOR_TEXT_DISABLED   ThemeManager::get_text_disabled()

// CAN-specific Colors
#define THEME_COLOR_CAN_TX          ThemeManager::get_can_tx()
#define THEME_COLOR_CAN_RX          ThemeManager::get_can_rx()
#define THEME_COLOR_CAN_ERROR       ThemeManager::get_can_error()
#define THEME_COLOR_CAN_RTR         ThemeManager::get_can_rtr()

// Chart/Graph Colors
#define THEME_COLOR_CHART_LINE1     ThemeManager::get_chart_line1()
#define THEME_COLOR_CHART_LINE2     ThemeManager::get_chart_line2()
#define THEME_COLOR_CHART_LINE3     ThemeManager::get_chart_line3()
#define THEME_COLOR_CHART_GRID      ThemeManager::get_chart_grid()

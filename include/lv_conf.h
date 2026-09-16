/**
 * LVGL Configuration for PocketCAN
 * Based on LVGL v8.3
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888) */
#define LV_COLOR_DEPTH 16

/* Swap the 2 bytes of RGB565 color. Useful if the display has an 8-bit interface */
#define LV_COLOR_16_SWAP 1

/* Enable more complex drawing routines to manage screens transparency */
#define LV_COLOR_SCREEN_TRANSP 0

/* Adjust color mix functions rounding */
#define LV_COLOR_MIX_ROUND_OFS (LV_COLOR_DEPTH == 32 ? 0: 128)

/* Support big-endian (not needed for ESP32) */
#define LV_BIG_ENDIAN_SYSTEM 0

/*=========================
   MEMORY SETTINGS
 *=========================*/

/* Use custom malloc/free, use heap_caps_malloc */
#define LV_MEM_CUSTOM 1
#if LV_MEM_CUSTOM == 0
    /* Size of the memory available for `lv_mem_alloc()` in bytes */
    #define LV_MEM_SIZE (64U * 1024U)
    /* Compiler prefix for a big array declaration in RAM */
    #define LV_MEM_ATTR
    /* Set an address for the memory pool instead of allocating it as a normal array */
    #define LV_MEM_ADR 0
    /* Instead of an address give a memory allocator that will be called to get a memory pool for LVGL */
    #define LV_MEM_POOL_INCLUDE your_alloc_library.h
    #define LV_MEM_POOL_ALLOC   your_alloc
#else
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif

/* Number of the intermediate memory buffer used during rendering */
#define LV_MEM_BUF_MAX_NUM 16

/*====================
   HAL SETTINGS
 *====================*/

/* Default display refresh period. LVG will redraw changed areas with this period time */
#define LV_DISP_DEF_REFR_PERIOD 30      /* [ms] */

/* Input device read period in milliseconds */
#define LV_INDEV_DEF_READ_PERIOD 30     /* [ms] */

/* Use a custom tick source */
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#endif

/* Default Dot Per Inch */
#define LV_DPI_DEF 130

/*=================
 * OPERATING SYSTEM
 *==================*/

/* Select an operating system to use */
#define LV_USE_OS   LV_OS_NONE

/*=======================
 * FONT USAGE
 *=======================*/

/* Montserrat fonts with ASCII range and some symbols */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 0
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 1

/* Demonstrate special features */
#define LV_FONT_MONTSERRAT_12_SUBPX      0
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0
#define LV_FONT_SIMSUN_16_CJK            0

/* Pixel perfect monospace fonts */
#define LV_FONT_UNSCII_8  1
#define LV_FONT_UNSCII_16 0

/* Optionally declare custom fonts here */
#define LV_FONT_CUSTOM_DECLARE

/* Always set a default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_24

/*===================
 * TEXT SETTINGS
 *===================*/

/* Enable support for bidirectional text */
#define LV_USE_BIDI 0

/* Enable Arabic/Persian processing */
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*=================
 * WIDGET USAGE
 *=================*/

/* Documentation: https://docs.lvgl.io/master/widgets/index.html */

#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMG        1
#define LV_USE_LABEL      1
#if LV_USE_LABEL
    #define LV_LABEL_TEXT_SELECTION 1
    #define LV_LABEL_LONG_TXT_HINT 1
#endif

#define LV_USE_LINE       1
#define LV_USE_ROLLER     1
#define LV_USE_SLIDER     1
#define LV_USE_SWITCH     1
#define LV_USE_TEXTAREA   1
#if LV_USE_TEXTAREA != 0
    #define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500
#endif

#define LV_USE_TABLE      1

/*==================
 * EXTRA COMPONENTS
 *==================*/

/* Grid layout */
#define LV_USE_GRID 1

/* Flex layout */
#define LV_USE_FLEX 1

/*==================
 * THEMES
 *==================*/

/* A simple, impressive and very complete theme */
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    /* 0: Light mode; 1: Dark mode */
    #define LV_THEME_DEFAULT_DARK 1

    /* 1: Enable grow on press */
    #define LV_THEME_DEFAULT_GROW 1

    /* Default transition time in [ms] */
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif

/* A very simple theme that is a good starting point for a custom theme */
#define LV_USE_THEME_BASIC 1

/* A theme designed for monochrome displays */
#define LV_USE_THEME_MONO 0

/*==================
 * LAYOUTS
 *==================*/

/* A layout similar to Flexbox in CSS */
#define LV_USE_FLEX 1

/* A layout similar to Grid in CSS */
#define LV_USE_GRID 1

/*---------------------
 * EXTRA WIDGETS
 *---------------------*/

#define LV_USE_ANIMIMG    1
#define LV_USE_CALENDAR   0
#define LV_USE_CHART      1
#define LV_USE_COLORWHEEL 0
#define LV_USE_IMGBTN     1
#define LV_USE_KEYBOARD   1
#define LV_USE_LED        1
#define LV_USE_LIST       1
#define LV_USE_MENU       1
#define LV_USE_METER      1
#define LV_USE_MSGBOX     1
#define LV_USE_SPAN       1
#define LV_USE_SPINBOX    1
#define LV_USE_SPINNER    1
#define LV_USE_TABVIEW    1
#define LV_USE_TILEVIEW   1
#define LV_USE_WIN        1

/*==================
 * EXAMPLES
 *==================*/

/* Enable the examples to be built with the library */
#define LV_BUILD_EXAMPLES 0

/*--END OF LV_CONF_H--*/

#endif /*LV_CONF_H*/

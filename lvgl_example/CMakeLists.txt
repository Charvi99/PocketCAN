SET(SOURCES ui_Screen1.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    fonts/ui_font_conthrax24.c
    fonts/ui_font_Font1.c
    fonts/ui_font_sevenSeg.c)

add_library(ui ${SOURCES})

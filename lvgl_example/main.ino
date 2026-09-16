/*
M5Stack Tab5 LVGL / SLS Initial Test - 23.05.25 - nikthefix

This preliminary demo just uses the display and touch features.
Peripherals like audio, gyro, camera etc will be added soon.

Dependencies:

ESP-Arduino >= V3.2 (tested also working with 3.3.0-alpha1)
M5GFX >= V0.2.8
LVGL = V8.3.11

lv_conf.h:

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 1
#define LV_MEM_CUSTOM 1
#define LV_TICK_CUSTOM 1

Build Options:

Board: "ESP32P4 Dev Module"
USB CDC on boot: "Enabled"
Flash Size: "16MB (128Mb)"
Partition Scheme: "Custom" (the supplied partitions.csv file allows almost full use of the flash for the main app)
PSRAM: "Enabled"
Upload Mode: "UART / Hardware CDC"
USB Mode: "Hardware CDC and JTAG"

Notes:

This demo uses a software rotate in order to give us landscape mode (disp_drv.rotated = LV_DISP_ROT_90;)
I've not been able to find the make and model of the display unit used in the Tab5 so I can't be sure if it supports a hardware accelerated rotation.
The display driver chip is ili9881c with native portrait orientation.

A Square Line Studio V1.5.1 project is included so that you can experiment with your own exported UI files.

*/



#include <M5GFX.h>
#include "lvgl.h"
#include "demo_ui.h" // Changed from "ui.h"
#include "pins_config.h"

M5GFX display;

// uint16_t count = 0; // No longer needed
// bool automate = false; // No longer needed
 
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf;



void lv_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    display.pushImageDMA(area->x1, area->y1, w, h, (uint16_t *)&color_p->full); 
    lv_disp_flush_ready(disp);
}



void my_rounder(lv_disp_drv_t *disp_drv, lv_area_t *area)
{
    if (area->x1 % 2 != 0)
        area->x1 += 1;
    if (area->y1 % 2 != 0)
        area->y1 += 1;

    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    if (w % 2 != 0)
        area->x2 -= 1;
    if (h % 2 != 0)
        area->y2 -= 1;
}



static void lv_indev_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{  
     lgfx::touch_point_t tp[3];
     uint8_t touchpad = display.getTouchRaw(tp,3);
       if (touchpad > 0)
       {
          data->state = LV_INDEV_STATE_PR;
          data->point.x = tp[0].x;
          data->point.y = tp[0].y;
          //Serial.printf("X: %d   Y: %d\n", tp[0].x, tp[0].y); //for testing
       }
       else
       {
        data->state = LV_INDEV_STATE_REL;
       }
}



void setup()
{
    /*Initialize Tab5 MIPI-DSI display*/
    display.init();

    Serial.begin(115200);//for debug

    /*Initialize LVGL*/
    lv_init();
    buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * LVGL_LCD_BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, LVGL_LCD_BUF_SIZE);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    //disp_drv.rounder_cb = my_rounder; 
    disp_drv.flush_cb = lv_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.sw_rotate = 1;  
    disp_drv.rotated = LV_DISP_ROT_90; 
    lv_disp_drv_register(&disp_drv);



    /*Initialize touch*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lv_indev_read;
    lv_indev_drv_register(&indev_drv);     



    /*Start UI*/
    demo_ui_init();   // Replaced ui_init()
    display.setBrightness(255);  
}



void loop()
{
  lv_timer_handler();
  delay(1);  

  // All the logic below was related to the old UI and is no longer needed.
  /*
    if(lv_obj_has_state(ui_Button1, LV_STATE_CHECKED)) automate = true;
    else automate = false;
    
    if(automate == true)
    {
    lv_arc_set_value(ui_Arc1, count);
    lv_label_set_text_fmt(ui_Label1, "%d", count);
    count++;
    if (count == 1000) count = 0; 
    }

    uint8_t brightness = lv_slider_get_value(ui_Slider1) ;
    display.setBrightness(brightness);
  */
}
>point.y = tp[0].y;
          //Serial.printf("X: %d   Y: %d\n", tp[0].x, tp[0].y); //for testing
       }
       else
       {
        data->state = LV_INDEV_STATE_REL;
       }
}



void setup()
{
    /*Initialize Tab5 MIPI-DSI display*/
    display.init();

    Serial.begin(115200);//for debug

    /*Initialize LVGL*/
    lv_init();
    buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * LVGL_LCD_BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, LVGL_LCD_BUF_SIZE);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    //disp_drv.rounder_cb = my_rounder; 
    disp_drv.flush_cb = lv_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.sw_rotate = 1;  
    disp_drv.rotated = LV_DISP_ROT_90; 
    lv_disp_drv_register(&disp_drv);



    /*Initialize touch*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lv_indev_read;
    lv_indev_drv_register(&indev_drv);     



    /*Start UI*/
    ui_init();   
    display.setBrightness(255);  
}



void loop()
{
  lv_timer_handler();
  delay(1);  

    if(lv_obj_has_state(ui_Button1, LV_STATE_CHECKED)) automate = true;
    else automate = false;
    
    if(automate == true)
    {
    lv_arc_set_value(ui_Arc1, count);
    lv_label_set_text_fmt(ui_Label1, "%d", count);
    count++;
    if (count == 1000) count = 0; 
    }

    uint8_t brightness = lv_slider_get_value(ui_Slider1) ;
    display.setBrightness(brightness);

}

#ifndef LV_PORT_DISP_H_
#define LV_PORT_DISP_H_

#include "lvgl.h"
#include "esp_log.h"
#include "esp_err.h"
//串口屏
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"

#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (10 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  0
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_DATA0          1
#define EXAMPLE_PIN_NUM_DATA1          2
#define EXAMPLE_PIN_NUM_DATA2          7
#define EXAMPLE_PIN_NUM_DATA3          8
#define EXAMPLE_PIN_NUM_DATA4          3
#define EXAMPLE_PIN_NUM_DATA5          18
#define EXAMPLE_PIN_NUM_DATA6          17
#define EXAMPLE_PIN_NUM_DATA7          16
#define EXAMPLE_PIN_NUM_PCLK           15
#define EXAMPLE_PIN_NUM_CS             10
#define EXAMPLE_PIN_NUM_DC             9
#define EXAMPLE_PIN_NUM_RST            14
#define EXAMPLE_PIN_NUM_BK_LIGHT       4

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              320
#define EXAMPLE_LCD_V_RES              480
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define EXAMPLE_LVGL_TICK_PERIOD_MS    2

#define LV_HOR_RES_MAX 320
#define LV_VER_RES_MAX 480

#define DISP_BUF_SIZE LV_HOR_RES_MAX*40

void lv_port_disp_init(void);

#endif
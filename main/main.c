#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lvgl.h"

//驱动
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "sd.h"

//移植ui
// #include "demos/lv_demos.h"
#include "ui.h"

#define TAG "main"
 
void lv_tick_task(void *arg)
{
    lv_tick_inc(1);
}

static void lvgl_driver_init(){
    lv_port_disp_init();
    gt911_init(0x5D);
    sd_init();//初始化SD卡，对接文件系统在menuconfig内
} 

void app_main(void)
{
    lv_init();
    /* Initialize SPI or I2C bus used by the drivers */
    lvgl_driver_init();

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1 * 1000));
 
    // lvgl demo演示
    // lv_demo_music();
    // lv_demo_stress();
    // lv_demo_widgets(); 
    ui_init();
   
    while (1)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_task_handler();
    }
}

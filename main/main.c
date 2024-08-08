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
#include "audio_player.h"

//移植ui
// #include "demos/lv_demos.h"
#include "ui.h"

#define TAG "main"
 
void lv_tick_task(void *arg)
{
    lv_tick_inc(1);
}

static void lvgl_driver_init()
{
    lv_port_disp_init();
    gt911_init(GT911_I2C_SLAVE_ADDR);
    sd_init();//初始化SD卡，对接文件系统在menuconfig内
} 

void lv_task(void *param)
{
    lv_init();
    /* Initialize SCREEN TOUCH and SD */
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
    vTaskDelete(NULL);
}

void print_task_info(void) {
    char buffer[1024];
    vTaskList(buffer);
    ESP_LOGI(TAG, "\nTask list:\nName          State   Prio    Stack   Num\n%s", buffer);
}

void print_task(void *param){
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        print_task_info();
    }
    
}

void app_main(void)
{
    xTaskCreate(lv_task,"lvgl",128*1024,NULL,5,NULL);
    xTaskCreate(audio_task,"audio",32*1024,NULL,5,NULL);
    
    // vTaskDelay(pdMS_TO_TICKS(1000));
    // xTaskCreate(print_task,"print",8*1024,NULL,5,NULL);
}

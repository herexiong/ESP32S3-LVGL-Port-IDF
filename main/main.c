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
//写入字库命令
//python C:\esp-idf\esp-idf_v4.4\esp-idf\components\esptool_py\esptool\esptool.py --chip esp32s3 --port COM5 --baud 115200 write_flash -z 0x410000 C:\esp-idf\esp-idf_v4.4\esp-idf\components\esptool_py\esptool\PINGFANG.bin
#include "ff.h"
#include <dirent.h>

#define MAX_PLAY_FILE_NUM 20

/* LVGL Object */
static lv_obj_t *current_music;
static lv_obj_t *button[3];
static lv_obj_t *img[3];
static lv_obj_t *list_music[MAX_PLAY_FILE_NUM];

/* Image and music resource */
// static uint8_t filecount = 0;
int filecount = 0;
static char directory[MAX_PLAY_FILE_NUM][100];
static bool play = false;
// void *img_src[] = {SYMBOL_PREV, SYMBOL_PLAY, SYMBOL_NEXT, SYMBOL_PAUSE};

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

static int read_content(const char *path, uint8_t *filecount)
{
    int ret = 0;
    char nextpath[300];
    struct dirent *de;
    // *filecount = 0;
    DIR *dir = opendir(path);
    //递归读取整个目录
    while (1) {
        de = readdir(dir);
        if (!de) {
            break;
        } else if (de->d_type == DT_REG) {//如果是文件
            if (strstr(de->d_name, ".mp3") || strstr(de->d_name, ".MP3")) {
                sprintf(directory[*filecount], "%s/%s", path, de->d_name);
                printf("filecount = %d :%s\n", (*filecount) , directory[*filecount]);
                if (++(*filecount) >= MAX_PLAY_FILE_NUM) {
                    ESP_LOGE(TAG,"break %d",(*filecount));
                    ret = -1;
                    break;
                }
            }
        } else if (de->d_type == DT_DIR) {//目录则进入递归
            sprintf(nextpath, "%s/%s", path, de->d_name);
            ret = read_content(nextpath, filecount);
            if(ret == -1){
                break;
            }
        }
    }
    closedir(dir);
    return ret;
}

// static FILE *get_file(int next_file)
// {
//     static FILE *file;
//     static int file_index = 1;

//     if (next_file != CONTROL_CURRENT) {
//         if (next_file == CONTROL_NEXT) {
//             // advance to the next file
//             if (++file_index > filecount - 1) {
//                 file_index = 0;
//             }
//         } else if (next_file == CONTROL_PREV) {
//             // advance to the prev file
//             if (--file_index < 0) {
//                 file_index = filecount - 1;
//             }
//         } else if (next_file >= 0 && next_file < filecount) {
//             file_index = next_file;
//         }
// #if USE_ADF_TO_PLAY
//         if (file != NULL) {
//             fclose(file);
//             file = NULL;
//         }
// #endif
//         ESP_LOGI(TAG, "[ * ] File index %d", file_index);
//     }
//     // return a handle to the current file
//     if (file == NULL) {
//         lv_label_set_text(current_music, strstr(directory[file_index], "d/") + 2);
// #if USE_ADF_TO_PLAY
//         file = fopen(directory[file_index], "r");
//         if (!file) {
//             ESP_LOGE(TAG, "Error opening file");
//             return NULL;
//         }
// #endif
//     }
//     return file;
// }

// static lv_res_t audio_next_prev(lv_obj_t *obj)
// {
//     if (obj == button[0]) {
//         // prev song
// #if USE_ADF_TO_PLAY
//         ESP_LOGI(TAG, "[ * ] [Set] touch tap event");
//         audio_pipeline_terminate(pipeline);
//         ESP_LOGI(TAG, "[ * ] Stopped, advancing to the prev song");
// #endif
//         get_file(CONTROL_PREV);
// #if USE_ADF_TO_PLAY
//         audio_pipeline_run(pipeline);
// #endif
//         lv_img_set_src(img[1], img_src[3]);
//         play = true;
//     } else if (obj == button[1]) {
//     } else if (obj == button[2]) {
//         // next song
// #if USE_ADF_TO_PLAY
//         ESP_LOGI(TAG, "[ * ] [Set] touch tap event");
//         audio_pipeline_terminate(pipeline);
//         ESP_LOGI(TAG, "[ * ] Stopped, advancing to the next song");
// #endif
//         get_file(CONTROL_NEXT);
// #if USE_ADF_TO_PLAY
//         audio_pipeline_run(pipeline);
// #endif
//         lv_img_set_src(img[1], img_src[3]);
//         play = true;
//     }
//     return LV_RES_OK;
// }

//"Night theme\nAlien theme\nMaterial theme\nZen theme\nMono theme\nNemo theme"
static void theme_change_action(lv_event_t *e)
{
    lv_obj_t *roller = lv_event_get_target(e);
    lv_theme_t *th;
    switch (lv_roller_get_selected(roller)) {
    case 0:
        th = lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
        break;

    case 1:
        th = lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_GREEN), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
        break;

    case 2:
        th = lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_ORANGE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
        break;

    case 3:
        th = lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_PURPLE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
        break;

    case 4:
        th = lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_GREY), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
        break;

    case 5:
        th = lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_CYAN), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
        break;

    default:
        th = lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
        break;
    }
    lv_disp_set_theme(NULL, th);
}

static void littlevgl_mp3(void)
{
    LV_FONT_DECLARE(PINGFANG);//引入字库
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_scr_load(scr);

    lv_theme_t *th = lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
    lv_disp_set_theme(NULL, th);

    lv_obj_t *tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);

    lv_obj_t *tab1 = lv_tabview_add_tab(tabview, "Audio");
    lv_obj_t *tab2 = lv_tabview_add_tab(tabview, "List");
    lv_obj_t *tab3 = lv_tabview_add_tab(tabview, "Settings");
    lv_tabview_set_act(tabview, 0, LV_ANIM_OFF);

    lv_obj_t *cont = lv_obj_create(tab1);
    lv_obj_set_size(cont, LV_HOR_RES - 20, LV_VER_RES - 85);
    lv_obj_center(cont);

    lv_obj_t *current_music = lv_label_create(cont);
    lv_label_set_long_mode(current_music, LV_LABEL_LONG_SCROLL);
    lv_obj_set_width(current_music, 200);
    lv_obj_align(current_music, LV_ALIGN_TOP_MID, 0, 20);
    lv_label_set_text(current_music, strstr(directory[1], "d/") + 2);
    lv_obj_set_style_text_font(current_music,&PINGFANG,0);// 设置风格的字体
    // lv_obj_set_style_text_font()


    lv_obj_t *button[3];
    lv_obj_t *label[3];
    char* text[3] = {"prev","play","next"};
    lv_obj_t *img[3];
    for (uint8_t i = 0; i < 3; i++) {
        button[i] = lv_btn_create(cont);
        lv_obj_set_size(button[i], 80, 50);
        label[i] = lv_label_create(button[i]);
        lv_label_set_text(label[i],text[i]);
        // img[i] = lv_img_create(button[i]);
        // lv_img_set_src(img[i], img_src[i]);
    }
    // lv_obj_align(button[0], LV_ALIGN_LEFT_MID, 35, 20);
    for (uint8_t i = 0; i < 3; i++) {
        lv_obj_align(button[i], LV_ALIGN_BOTTOM_LEFT, (15+80)*i, -15);
        lv_obj_align(label[i], LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_font(label[i],&PINGFANG,0);// 设置风格的字体
    }
    // lv_obj_add_event_cb(button[0], audio_next_prev, LV_EVENT_CLICKED, NULL);
    // lv_obj_add_event_cb(button[1], audio_control, LV_EVENT_CLICKED, NULL);
    // lv_obj_add_event_cb(button[2], audio_next_prev, LV_EVENT_CLICKED, NULL);

    lv_obj_t *list = lv_list_create(tab2);
    lv_obj_set_size(list, LV_HOR_RES - 20, LV_VER_RES - 85);
    for (uint8_t i = 0; i < filecount; i++) {
        lv_list_add_btn(list, LV_SYMBOL_AUDIO, strstr(directory[i], "d/") + 2);
    }
    lv_obj_set_style_text_font(list,&PINGFANG,0);// 设置风格的字体

    lv_obj_t *theme_label = lv_label_create(tab3);
    lv_label_set_text(theme_label, "Theme:");

    lv_obj_t *theme_roller = lv_roller_create(tab3);
    lv_obj_align(theme_roller, LV_ALIGN_OUT_RIGHT_MID, 80, 0);
    lv_roller_set_options(theme_roller, "Night theme\nAlien theme\nMaterial theme\nZen theme\nMono theme\nNemo theme", LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(theme_roller, 1, LV_ANIM_OFF);
    lv_roller_set_visible_row_count(theme_roller, 3);
    lv_obj_add_event_cb(theme_roller, theme_change_action, LV_EVENT_VALUE_CHANGED, NULL);

    ESP_LOGI("LvGL", "app_main last: %d", esp_get_free_heap_size());
}

void lv_task(void *param)
{
    lv_init();
    /* Initialize SCREEN TOUCH and SD */
    lvgl_driver_init();

    read_content("/sdcard", &filecount);

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
    // ui_init();

    littlevgl_mp3();

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

void task(void *param){
    sd_init();
    read_content(LV_FS_PATH, &filecount);
    ESP_LOGE(TAG,"begin to printf list num:%d",filecount);
    for(int i = 0; i<filecount ;i++){
        printf("%s\n",directory[i]);
    }
    ESP_LOGE(TAG,"end to printf list");
    vTaskDelete(NULL);
}

void app_main(void)
{

    xTaskCreate(lv_task,"lvgl",128*1024,NULL,5,NULL);
    // xTaskCreate(task,"test",128*1024,NULL,5,NULL);

    // xTaskCreate(audio_task,"audio",32*1024,NULL,5,NULL);
    
    // vTaskDelay(pdMS_TO_TICKS(1000));
    // xTaskCreate(print_task,"print",8*1024,NULL,5,NULL);
}

#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
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
//字库
#include "lv_fonts.h"
//playlist
#include "sd_playlist.h"

/* LVGL Object */
static lv_obj_t *current_music;
static lv_obj_t *button[3];
lv_obj_t *label[3];
char* text[3] = {LV_SYMBOL_PREV,LV_SYMBOL_PAUSE,LV_SYMBOL_NEXT};
// static lv_obj_t *img[3];
// static lv_obj_t *list_music[MAX_PLAY_FILE_NUM];

lv_obj_t *lv_list;
lv_obj_t *tabview;
lv_obj_t * tab1;
lv_obj_t *tab2;

lv_obj_t *current_music_bar;
lv_obj_t *current_music_time;
lv_obj_t *current_music_totaltime;

//sd_playlist
extern SemaphoreHandle_t read_semphr;
int current_music_num;//current music对应的索引
extern sd_playlist_t list;
////

#define TAG "main"
 
void lv_tick_task(void *arg)
{
    lv_tick_inc(1);
}

static void lvgl_driver_init()
{
    lv_port_disp_init();
    lv_port_indev_init();
} 

void change_btn_icon(bool isplaying){
    if (isplaying)
    {
        lv_label_set_text(label[1],LV_SYMBOL_PLAY);
    }else{
        lv_label_set_text(label[1],LV_SYMBOL_PAUSE);
    }
    
}

void change_audio_bar(char *current_time,char *total_time,int value){
    lv_label_set_text(current_music_time,current_time);
    lv_label_set_text(current_music_totaltime,total_time);
    lv_slider_set_value(current_music_bar,value,LV_ANIM_ON);
}

static lv_res_t audio_btn_cb(lv_obj_t *event)
{
    lv_obj_t* obj = lv_event_get_target(event);
    char buffer[MAX_SONG_NAME_LEN+1];

    if (obj == button[0]) {
        //上一首按钮
        current_music_num = current_music_num == 0? list.playlist_count :current_music_num-1;
        ESP_LOGI(TAG,"Previous Song");
    } else if (obj == button[1]) {
        buffer[0] = Audio_Control;
        buffer[1] = '\0';
        xQueueSendFromISR(audio_queue,buffer,0);
        return LV_RES_OK;
    } else if (obj == button[2]) {
        //下一首按钮
        current_music_num = current_music_num == list.playlist_count ? 0 :current_music_num+1;
        ESP_LOGI(TAG,"Next Song");
    }
    if (list.playlist[current_music_num])
    {
        sprintf(buffer,"%c%s",Audio_Resource,list.playlist[current_music_num]);
        xQueueSend(audio_queue,buffer,pdMS_TO_TICKS(10));
        lv_label_set_text(current_music,list.show_playlist[current_music_num]);
    }else{
        ESP_LOGE(TAG,"list set song is not existed");
        lv_label_set_text(current_music,"列表歌曲不存在");
    }
    return LV_RES_OK;
}

static void list_btn_cb(lv_obj_t *event){
    lv_obj_t* obj = lv_event_get_target(event);
    //不知道为啥list内的btn不掉回调函数
    // lv_obj_t* lv_list = lv_event_get_current_target(event);
    // const char * text = lv_list_get_btn_text(lv_list, obj);
    int cnt = lv_obj_get_index(obj);
    char str[100]; // 确保字符数组足够大以容纳文本
    if (list.playlist[cnt] != NULL)
    {
        current_music_num = cnt;
        sprintf(str,"%c%s",Audio_Resource,list.playlist[cnt]);
        xQueueSend(audio_queue,str,0);
        lv_label_set_text(current_music,list.show_playlist[cnt]);
        lv_tabview_set_act(tabview,0,LV_ANIM_ON);
        ESP_LOGI(TAG,"list set song name:%s",str);
    }else{
        ESP_LOGE(TAG,"list set song is not existed");
        lv_label_set_text(current_music,"列表歌曲不存在");
    }
}

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

void current_music_bar_cb(lv_obj_t *event){
    lv_obj_t* obj = lv_event_get_target(event);
    int value = lv_slider_get_value(obj);
    player_set_bar(value);
    ESP_LOGI(TAG,"slide value is %d",value);
}

void show_palylist(char **playlist,int playlist_count){
    lv_obj_t * lab;
    int i;
    char str[MAX_SONG_NAME_LEN]; // 确保字符数组足够大以容纳文本
    if (lv_list == NULL)
    {
        current_music_num = 0;
        lv_list = lv_list_create(tab2);
        lv_obj_set_size(lv_list, LV_HOR_RES - 20, LV_VER_RES - 85);


    }else{
        i = 0;
        while ((lab = lv_obj_get_child(lv_list,i)) != NULL)
        {
            lv_obj_del(lab);
            i++;
        }
    }
    //刷新播放页面
    if (playlist[current_music_num] != NULL)
    {
        lv_label_set_text(current_music, list.show_playlist[0]);
        sprintf(str,"%c%s",Audio_Resource,playlist[0]);
        xQueueSend(audio_queue,str,0);
    }else{
        lv_label_set_text(current_music,"播放列表为空");
    }
    //刷新列表页面
    for (i = 0; i <=playlist_count && list.show_playlist[0] != NULL; i++) {
        // btn = lv_btn_create(lv_list);
        // lv_obj_set_width(btn, lv_pct(100));
        // lv_obj_add_event_cb(btn, list_btn_cb , LV_EVENT_CLICKED, NULL);
        lab = lv_label_create(lv_list);
        // lv_label_set_text_fmt(lab, strstr(playlist[i], "d/") + 2);
        lv_label_set_text(lab, list.show_playlist[i]);
        lv_label_set_long_mode(lab, LV_LABEL_LONG_SCROLL);
        lv_obj_add_flag(lab, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(lab, list_btn_cb , LV_EVENT_CLICKED, NULL);
        //由于中文字体里无符号，故取消图标
        // lv_list_add_btn(lv_list, LV_SYMBOL_AUDIO, strstr(directory[i], "d/") + 2);
        // lv_list_add_btn(lv_list, NULL, strstr(directory[i], "d/") + 2);
    }
    // lv_obj_add_event_cb(lv_list, list_btn_cb , LV_EVENT_PRESSED, NULL);
    // lv_obj_set_style_text_font(lv_list, &PINGFANG, 0);
    lv_obj_set_style_text_font(lv_list, &font_harmony_sans_20, 0);
}

static void lvgl_mp3_ui(void)
{
    // lv_port_font_pingfang_sans_16_load("font_pingfang16");
    lv_port_font_harmony_sans_20_load("font_hs20");
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_scr_load(scr);

    lv_theme_t *th = lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
    lv_disp_set_theme(NULL, th);

    tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);

    tab1 = lv_tabview_add_tab(tabview, LV_SYMBOL_AUDIO);
    tab2 = lv_tabview_add_tab(tabview, LV_SYMBOL_LIST);
    lv_obj_t *tab3 = lv_tabview_add_tab(tabview, LV_SYMBOL_SETTINGS);
    lv_tabview_set_act(tabview, 0, LV_ANIM_OFF);

    lv_obj_t *cont = lv_obj_create(tab1);
    lv_obj_set_size(cont, LV_HOR_RES - 20, LV_VER_RES - 85);
    lv_obj_center(cont);

    current_music = lv_label_create(cont);
    lv_label_set_long_mode(current_music, LV_LABEL_LONG_SCROLL);
    lv_obj_set_width(current_music, 200);
    lv_obj_align(current_music, LV_ALIGN_TOP_MID, 0, 20);
    lv_label_set_text(current_music," ");
    // lv_obj_set_style_text_font(current_music, &PINGFANG, 0);
    lv_obj_set_style_text_font(current_music, &font_harmony_sans_20, 0);

    current_music_bar = lv_slider_create(cont);
    lv_obj_set_size(current_music_bar, LV_HOR_RES-20-115, 12);
    lv_obj_align(current_music_bar, LV_ALIGN_BOTTOM_MID, 0, -90);
    lv_slider_set_range(current_music_bar, 0, 100);
    lv_obj_add_event_cb(current_music_bar, current_music_bar_cb, LV_EVENT_RELEASED , NULL);

    current_music_time = lv_label_create(cont);
    lv_obj_align(current_music_time, LV_ALIGN_BOTTOM_MID, -120, -87);
    lv_label_set_text(current_music_time,"0:00");

    current_music_totaltime = lv_label_create(cont);
    lv_obj_align(current_music_totaltime, LV_ALIGN_BOTTOM_MID, 120, -87);
    lv_label_set_text(current_music_totaltime,"0:00");

    // lv_obj_t *img[3];
    for (uint8_t i = 0; i < 3; i++) {
        button[i] = lv_btn_create(cont);
        lv_obj_set_size(button[i], 80, 50);
        label[i] = lv_label_create(button[i]);
        lv_label_set_text(label[i],text[i]);
        lv_obj_align(button[i], LV_ALIGN_BOTTOM_LEFT, (15+80)*i, -15);
        lv_obj_align(label[i], LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_event_cb(button[i], audio_btn_cb, LV_EVENT_CLICKED, NULL);
        // img[i] = lv_img_create(button[i]);
        // lv_img_set_src(img[i], img_src[i]);
    }
    // lv_obj_align(button[0], LV_ALIGN_LEFT_MID, 35, 20);

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
    //将显示和触摸注册到LVGL
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
    //squline 移植UI
    // ui_init();
    //MP3 UI
    lvgl_mp3_ui();
    lv_port_disp_backlight(true);//在UI初始化后打开背光避免花屏
    while (1)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_task_handler();
        if (xSemaphoreTake(read_semphr,pdMS_TO_TICKS(10)))
        {
            show_palylist(list.playlist,list.playlist_count);
        }
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
        vTaskDelay(pdMS_TO_TICKS(5000));
        print_task_info();
    }
    
}

static void hardware_init(void){
    disp_8080_init();
    gt911_init(GT911_I2C_SLAVE_ADDR);
    sd_init();//初始化SD卡，LVGL对接文件系统在menuconfig内
}

void app_main(void)
{
    //初始化硬件
    hardware_init();
    //歌曲异步扫描
    xTaskCreate(playlist_start_task,"playlist_start_task",16*1024,PLAYLIST_PATH,5,NULL);
    //实现播放功能
    // xTaskCreate(audio_task,"audio",32*1024,NULL,5,NULL);
    xTaskCreate(esp_audio_task,"esp_audio_task",32*1024,NULL,5,NULL);
    //UI
    xTaskCreate(lv_task,"lvgl_task",128*1024,NULL,5,NULL);

    
    // vTaskDelay(pdMS_TO_TICKS(1000));
    // xTaskCreate(print_task,"print",4*1024,NULL,4,NULL);
}

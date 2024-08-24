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
//sd_playlist
#include "sd_playlist.h"
//audio metadata
#include "audio_metadata.h"

/* LVGL Object */
static lv_obj_t *current_music;
static lv_obj_t *button[3];
lv_obj_t *label[3];

// static lv_obj_t *img[3];
// static lv_obj_t *list_music[MAX_PLAY_FILE_NUM];

lv_obj_t *lv_list;
lv_obj_t *tabview;
lv_obj_t * tab1;
lv_obj_t *tab2;

lv_obj_t *current_music_bar;
lv_obj_t *current_music_time;
lv_obj_t *current_music_totaltime;
lv_obj_t *current_music_info[6];

lv_obj_t *volume_control_bar;
lv_obj_t *volume_control_label;

lv_obj_t *lv_album_img;


//sd_playlist
extern SemaphoreHandle_t read_semphr;
int current_music_num;//current music对应的索引
extern sd_playlist_t list;
////

SemaphoreHandle_t album_semphr;

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

//更改播放按钮的图标
void change_btn_icon(bool isplaying){
    if (isplaying)
    {
        lv_label_set_text(label[1],LV_SYMBOL_PLAY);
    }else{
        lv_label_set_text(label[1],LV_SYMBOL_PAUSE);
    }
}

//更改进度条的值，总时间和已播时间
void change_audio_bar(char *current_time,char *total_time,int value){
    lv_label_set_text(current_music_time,current_time);
    lv_label_set_text(current_music_totaltime,total_time);
    lv_slider_set_value(current_music_bar,value,LV_ANIM_ON);
}

//按钮回调函数
static lv_res_t audio_btn_cb(lv_obj_t *event)
{
    lv_obj_t* obj = lv_event_get_target(event);
    char buffer[MAX_SONG_NAME_LEN+1];

    //上一首按钮
    if (obj == button[0]) {
        current_music_num = current_music_num == 0? list.playlist_count :current_music_num-1;
        ESP_LOGI(TAG,"Previous Song");
    }
    //播放按钮
    else if (obj == button[1]) {
        buffer[0] = Audio_Control;
        buffer[1] = '\0';
        xQueueSendFromISR(audio_queue,buffer,0);
        return LV_RES_OK;
    }
    //下一首按钮
    else if (obj == button[2]) {
        
        current_music_num = current_music_num == list.playlist_count ? 0 :current_music_num+1;
        ESP_LOGI(TAG,"Next Song");
    }
    if (list.playlist[current_music_num])
    {
        sprintf(buffer,"%c%s",Audio_Resource,list.playlist[current_music_num]);
        xQueueSend(audio_queue,buffer,pdMS_TO_TICKS(10));
        lv_label_set_text(current_music,list.show_playlist[current_music_num]);
        //扫描专辑信息
        xTaskCreate(audio_ReadMetaData_task,"audio_ReadMetaData_task",8*1024,list.playlist[current_music_num],5,NULL);
    }else{
        ESP_LOGE(TAG,"list set song is not existed");
        lv_label_set_text(current_music,"列表歌曲不存在");
    }
    return LV_RES_OK;
}

//列表回调函数
static void list_list_cb(lv_obj_t *event){
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
        //扫描专辑信息
        xTaskCreate(audio_ReadMetaData_task,"audio_ReadMetaData_task",8*1024,list.playlist[cnt],5,NULL);
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

//拖动进度条回调函数
void current_music_bar_cb(lv_obj_t *event){
    lv_obj_t* obj = lv_event_get_target(event);
    int value = lv_slider_get_value(obj);
    player_set_time(value);
    // ESP_LOGI(TAG,"slide value is %d",value);
}

//音量条回调函数
void volume_control_bar_cb(lv_obj_t *event){
    lv_obj_t* obj = lv_event_get_target(event);
    int value = lv_slider_get_value(obj);
    char buffer[10];
    sprintf(buffer,"Volume:%02d",value);
    player_set_volume(value);
    lv_label_set_text(volume_control_label,buffer);
    // ESP_LOGI(TAG,"slide value is %d",value);
}    

//刷新播放列表
void show_palylist(char **playlist,int playlist_count){
    lv_obj_t * lab;
    int i;
    char str[MAX_SONG_NAME_LEN]; // 确保字符数组足够大以容纳文本
    if (lv_list == NULL)//首次刷新，创建lv_list对象
    {
        current_music_num = 0;
        lv_list = lv_list_create(tab2);
        lv_obj_set_size(lv_list, LV_HOR_RES - 20, LV_VER_RES - 85);
    }else{//非首次刷新,需要删除前面创建的lv_list内的子对象
        while ((lab = lv_obj_get_child(lv_list,0)) != NULL)
        {
            lv_obj_del(lab);
        }
    }
    //刷新当前播放音乐的label页面
    if (playlist[0] != NULL)
    {
        lv_label_set_text(current_music, list.show_playlist[0]);
        sprintf(str,"%c%s",Audio_Resource,playlist[0]);
        xQueueSend(audio_queue,str,0);
    }else{
        lv_label_set_text(current_music,"播放列表为空");
    }
    //刷新列表页面
    for (i = 0; i <= playlist_count && list.show_playlist[i] != NULL; i++) {
        lab = lv_label_create(lv_list);
        // lv_label_set_text_fmt(lab, strstr(playlist[i], "d/") + 2);
        lv_label_set_text(lab, list.show_playlist[i]);
        // ESP_LOGE(TAG,"show_palylist lv_label_set_text:%s",list.show_playlist[i]);
        lv_label_set_long_mode(lab, LV_LABEL_LONG_SCROLL);
        lv_obj_add_flag(lab, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(lab, list_list_cb , LV_EVENT_CLICKED, NULL);
        //由于中文字体里无符号，故取消图标
        // lv_list_add_btn(lv_list, LV_SYMBOL_AUDIO, strstr(directory[i], "d/") + 2);
        // lv_list_add_btn(lv_list, NULL, strstr(directory[i], "d/") + 2);
    }
    // ESP_LOGE(TAG,"show_palylist draw playlist done! num:%d",playlist_count);
    lv_obj_set_style_text_font(lv_list, &font_harmony_sans_20, 0);
}

//UI界面初始化函数
static void lvgl_mp3_ui(void)
{
    lv_port_font_harmony_sans_20_load("font_hs20");
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_scr_load(scr);

    lv_theme_t *th = lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), CONFIG_LV_USE_THEME_DEFAULT, LV_FONT_DEFAULT);
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

    // lv_album_img = lv_img_create(cont); // 在当前tab上创建一个图像对象
    // lv_obj_align(lv_album_img, LV_ALIGN_CENTER, 0, -30);

    for (uint8_t i = 0; i < 6; i++)
    {
        current_music_info[i] = lv_label_create(cont);
        lv_obj_align(current_music_info[i], LV_ALIGN_TOP_LEFT, 0, 40+30*i);
        lv_label_set_text(current_music_info[i]," ");
        lv_obj_set_style_text_font(current_music_info[i], &font_harmony_sans_20, 0);
    }
    

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
    char* text[3] = {LV_SYMBOL_PREV,LV_SYMBOL_PAUSE,LV_SYMBOL_NEXT};
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

    volume_control_label = lv_label_create(tab3);
    lv_label_set_text(volume_control_label, "Volume:20");
    lv_obj_align(volume_control_label, LV_ALIGN_TOP_LEFT, 0, 130);

    volume_control_bar = lv_slider_create(tab3);
    lv_obj_set_size(volume_control_bar, 170, 12);
    lv_obj_align(volume_control_bar, LV_ALIGN_TOP_LEFT, 100, 132);
    lv_slider_set_range(volume_control_bar, 0 , 100);
    lv_slider_set_value(volume_control_bar,20,LV_ANIM_OFF);
    lv_obj_add_event_cb(volume_control_bar, volume_control_bar_cb, LV_EVENT_RELEASED , NULL);

    ESP_LOGI("LVGL", "app_main last: %d", esp_get_free_heap_size());
}

void lv_task(void *param)
{
    lv_init();
    //将显示和触摸注册到LVGL
    lvgl_driver_init();
    album_semphr = xSemaphoreCreateBinary();
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
        if (xSemaphoreTake(read_semphr,0))
        {
            show_palylist(list.playlist,list.playlist_count);
        }
        if (xSemaphoreTake(album_semphr,0))
        {
            if(id3v2_data.title != NULL) lv_label_set_text(current_music_info[0],id3v2_data.title);     else lv_label_set_text(current_music_info[0]," ");
            if(id3v2_data.album != NULL) lv_label_set_text(current_music_info[1],id3v2_data.album);     else lv_label_set_text(current_music_info[1]," ");
            if(id3v2_data.artist != NULL) lv_label_set_text(current_music_info[2],id3v2_data.artist);   else lv_label_set_text(current_music_info[2]," ");
            if(id3v2_data.trck != NULL) lv_label_set_text(current_music_info[3],id3v2_data.trck);       else lv_label_set_text(current_music_info[3]," ");
            if(id3v2_data.year != NULL) lv_label_set_text(current_music_info[4],id3v2_data.year);       else lv_label_set_text(current_music_info[4]," ");
            if(id3v2_data.comment != NULL) lv_label_set_text(current_music_info[5],id3v2_data.comment); else lv_label_set_text(current_music_info[5]," ");
            //刷新专辑图的代码,已弃用
            // if (img_src != NULL)
            // {
                // ESP_LOGI(TAG,"load img size:%d",img_size);
                // lv_img_header_t header;
                // lv_img_decoder_get_info(img_src, &header);
                // ESP_LOGI(TAG,"zero:%d,w:%d,h:%d,cf:%d",header.always_zero,header.w,header.h,header.cf);
                // lv_img_src_t src_type = lv_img_src_get_type(img_src);//解析成2了
                // ESP_LOGI(TAG,"TYPE:%d",src_type);
                // lv_img_dsc_t img_dsc = {
                //     .header.always_zero = 0,
                //     .header.w = 0,
                //     .header.h = 0,//LV_SIZE_CONTENT
                //     .header.cf = LV_IMG_CF_TRUE_COLOR,//LV_IMG_CF_RAW
                //     .data = img_src,        // 指向SPIRAM中的JPG图像数据
                //     .data_size = img_size   // 图像数据的大小
                // };
                // lv_img_set_src(lv_album_img, &img_dsc);
                // lv_obj_align(lv_album_img, LV_ALIGN_CENTER, 0, 0);
                // // lv_img_set_zoom(lv_album_img, 512);//256代表一倍 256*N
            // }
        }
    }
    vTaskDelete(NULL);
}

//任务调试代码
///////////////////////////////////////////
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
///////////////////////////////////////////

//初始化硬件
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
    xTaskCreate(esp_audio_task,"esp_audio_task",32*1024,NULL,5,NULL);
    //UI
    xTaskCreate(lv_task,"lvgl_task",16*1024,NULL,5,NULL);

    
    // vTaskDelay(pdMS_TO_TICKS(1000));
    // xTaskCreate(print_task,"print",4*1024,NULL,4,NULL);
}

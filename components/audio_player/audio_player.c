#include "audio_player.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2s_stream.h"
#include "fatfs_stream.h"
#include "http_stream.h"

#include "sd.h"
#include "dev_board.h"
//esp-audio框架
#include "esp_audio.h"
#include "esp_decoder.h"

#include "audio_dlna.h"

static const char *TAG = "AUDIO_PLAYER";

esp_audio_handle_t audio_player;
audio_element_handle_t i2s_stream_writer, fatfs_stream_reader;
extern void change_btn_icon(bool isplaying);
extern void change_audio_bar(char *current_time,char *total_time,int value);

int current,total,current_min_time,current_second_time,total_min_time,total_second_time;
char url[100+4] = {0};

// 配置 I2S 引脚
void i2s_setPin(void){
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = AUDIO_I2S_PIN_BCK,      // BCLK
        .ws_io_num = AUDIO_I2S_PIN_WS,        // LRC
        .data_out_num = AUDIO_I2S_PIN_DATA,   // DIN
        .data_in_num = I2S_PIN_NO_CHANGE      // 无输入
        
    };
    esp_err_t err = i2s_set_pin(I2S_NUM, &pin_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pin: %s", esp_err_to_name(err));
        i2s_driver_uninstall(I2S_NUM);
        return;
    }
}

//用于lvgl界面和audio组件间的通讯
void message_task(void *param){
    char message[100];
    
    audio_queue = xQueueCreate(10,sizeof(message));
    ESP_LOGI(TAG,"queue created");
    // audio_element_state_t el_state;
    esp_audio_state_t audio_state;
    while (1)
    {
        if (xQueueReceive(audio_queue,message,pdMS_TO_TICKS(50)))
        {
            ESP_LOGI(TAG,"message:%s",message);
            if(message[0] == Audio_Control){
                //旧的element和pipeline代码,下一版本删除
                // el_state = audio_element_get_state(i2s_stream_writer);
                // ESP_LOGI(TAG,"message: Audio_Control state:%d",el_state);
                // switch (el_state) {
                //     case AEL_STATE_INIT:
                //         // audio_element_set_uri(fatfs_stream_reader, url);
                //         // audio_pipeline_run(pipeline);
                //         break;
                //     case AEL_STATE_RUNNING:
                //         // audio_pipeline_pause(pipeline);
                //         esp_audio_pause(audio_player);
                //         break;
                //     case AEL_STATE_PAUSED:
                //         // audio_pipeline_resume(pipeline);
                //         esp_audio_resume(audio_player);
                //         break;
                //     case AEL_STATE_FINISHED:
                //         // audio_element_set_uri(fatfs_stream_reader, url);
                //         // audio_pipeline_reset_ringbuffer(pipeline);
                //         // audio_pipeline_reset_elements(pipeline);
                //         // audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
                //         // audio_pipeline_run(pipeline);
                //         esp_audio_play(audio_player, AUDIO_CODEC_TYPE_DECODER, url, 0);
                //         break;
                //     default:
                //         break;

                esp_audio_state_get(audio_player,&audio_state);
                ESP_LOGI(TAG,"Audio_Control state:%d",audio_state.status);
                switch (audio_state.status) {
                    case AUDIO_STATUS_RUNNING:
                        esp_audio_pause(audio_player);
                        break;
                    case AUDIO_STATUS_PAUSED:
                        esp_audio_resume(audio_player);
                        break;
                    case AUDIO_STATUS_FINISHED:
                        esp_audio_play(audio_player, AUDIO_CODEC_TYPE_DECODER, url, 0);
                        break;
                    case AUDIO_STATUS_ERROR:
                        esp_audio_play(audio_player, AUDIO_CODEC_TYPE_DECODER, url, 0);
                    default:
                        break;
                }
            }
            else if(message[0] == Audio_Resource){
                sprintf(url,"file:/%s",message+1);
                ESP_LOGI(TAG,"Audio_Resource resource:%s",url);
                // audio_pipeline_pause(pipeline);
                // audio_pipeline_reset_ringbuffer(pipeline);
                // audio_pipeline_reset_elements(pipeline);
                // audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
                // audio_element_set_uri(fatfs_stream_reader, url);
                // audio_pipeline_run(pipeline);'
                // audio_termination_type_t type;
                esp_audio_state_get(audio_player,&audio_state);
                ESP_LOGI(TAG,"Audio_Control state:%d",audio_state.status);
                switch (audio_state.status) {
                    case AUDIO_STATUS_RUNNING:
                        esp_audio_stop(audio_player,TERMINATION_TYPE_NOW);
                        break;
                    case AUDIO_STATUS_PAUSED:
                        esp_audio_stop(audio_player,TERMINATION_TYPE_NOW);
                        break;
                    case AUDIO_STATUS_FINISHED:
                        break;
                    case AUDIO_STATUS_ERROR:
                        esp_audio_stop(audio_player,TERMINATION_TYPE_NOW);
                        break;
                    default:
                        break;
                }
                esp_audio_play(audio_player, AUDIO_CODEC_TYPE_DECODER, url, 0);
            }
        }
    }
    vQueueDelete(audio_queue);
    vTaskDelete(NULL);
}

//进度条任务
static void player_bar_task(void *param){
    ESP_LOGE(TAG,"player_bar_task has been created");
    //关掉esp_audio_duration_get查询的日志输出
    esp_log_level_set("ESP_AUDIO_CTRL",ESP_LOG_NONE);
    char current_time_str[20];
    char total_time_str[20];
    while (1)
    {
        if (esp_audio_duration_get(audio_player, &total) == ESP_ERR_AUDIO_NO_ERROR && esp_audio_time_get(audio_player, &current) == ESP_ERR_AUDIO_NO_ERROR)
        {
            // ESP_LOGE(TAG,"current %d,total;%d",current,total);
            total_min_time = total/60000;
            total_second_time = (total%60000)/1000;

            current_min_time = current/60000;
            current_second_time = (current%60000)/1000;
            
            // ESP_LOGE(TAG,"total:%s current:%s",total_time_str,current_time_str);
            
            int total_time = total_min_time*60+total_second_time;
            int current_time = current_min_time*60+current_second_time;
            if (total_time ==0 ||current_time == 0)
            {
                change_audio_bar("0:00","0:00",0);
            }else{
                sprintf(current_time_str,"%d:%02d",current_min_time,current_second_time);
                sprintf(total_time_str,"%d:%02d",total_min_time,total_second_time);                
                change_audio_bar(current_time_str,total_time_str,current_time*100/total_time);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(900));
    }
    vTaskDelete(NULL);
}

//根据拖动的任务条的值设置音频播放位置
void player_set_time(int value){
    int total_ms = total;
    int target_seconds = (value * total_ms) / 100000;
    esp_audio_state_t audio_state;
    esp_audio_state_get(audio_player,&audio_state);
    if (audio_state.status == AUDIO_STATUS_FINISHED)
    {
        esp_audio_play(audio_player, AUDIO_CODEC_TYPE_DECODER, url, 0);
    }
    esp_audio_seek(audio_player,target_seconds);
    
    ESP_LOGI(TAG,"player_set_time seek:%d",target_seconds);
}

//根据拖动的任务条的值设置音量
void player_set_volume(int value){
    esp_audio_vol_set(audio_player,value);
}

//esp_audio事件处理函数
static void player_event_handler(esp_audio_state_t *state, void *ctx)
{
    ESP_LOGE(TAG,"player_event_handler state:%d",state->status);
    if (audio_player == NULL) {
        return ;
    }
    int duration,current_time;
    esp_audio_music_info_t info;
    switch (state->status) {
        case AUDIO_STATUS_RUNNING:
            esp_audio_music_info_get(audio_player,&info);
            ESP_LOGI(TAG,"rate:%d bits:%d channel:%d, ", info.sample_rates,info.bits,info.channels);
            //ToDo->在esp_audio中设置时钟
            i2s_stream_set_clk(i2s_stream_writer, info.sample_rates, info.bits, info.channels);
            change_btn_icon(true);
            break;
        case AUDIO_STATUS_PAUSED:
            change_btn_icon(false);
            break;
        case AUDIO_STATUS_STOPPED:
            ESP_LOGI(TAG,"AUDIO_STATUS_STOPPED");
            change_btn_icon(false);
            break;
        case AUDIO_STATUS_FINISHED:
            change_btn_icon(false);
            break;
        case AUDIO_STATUS_ERROR:
            ESP_LOGE(TAG,"AUDIO_STATUS_ERROR");
            esp_audio_stop(audio_player,TERMINATION_TYPE_NOW);
            // esp_audio_play(audio_player, AUDIO_CODEC_TYPE_DECODER, url, 0);
            break;
        default:
            break;
    }
}

//player 事件处理函数
void dlna_player_event_handler(esp_audio_state_t *state, void *ctx)
{
    if (audio_player == NULL) {
        return ;
    }

    switch (state->status) {
        case AUDIO_STATUS_RUNNING:
            trans_state = "PLAYING";
            break;
        case AUDIO_STATUS_PAUSED:
            trans_state = "PAUSED_PLAYBACK";
            break;
        case AUDIO_STATUS_STOPPED:
        case AUDIO_STATUS_FINISHED:
            trans_state = "STOPPED";
            break;
        default:
            break;
    }
    // dlna_notify();
}

esp_err_t volume_set(void *hd, int vol){
    // 用户输入的是0-100，然而alc接受的音量是range(-64，64)
    return i2s_alc_volume_set(*(audio_element_handle_t *)hd,(vol * 128 / 100) - 64);
}

esp_err_t volume_get(void *hd, int *vol){
    // 用户接受的是0-100，然而alc输出的音量是range(-64，64)
    esp_err_t ret = i2s_alc_volume_get(*(audio_element_handle_t *)hd,vol);
    *vol = (*vol +64)*100/128;
    return ret;
}

//http流 事件处理函数
static int _http_stream_event_handle(http_stream_event_msg_t *msg)
{
    if (msg->event_id == HTTP_STREAM_RESOLVE_ALL_TRACKS) {
        return ESP_OK;
    }
    if (msg->event_id == HTTP_STREAM_FINISH_TRACK) {
        return http_stream_next_track(msg->el);
    }
    if (msg->event_id == HTTP_STREAM_FINISH_PLAYLIST) {
        return http_stream_restart(msg->el);
    }
    return ESP_OK;
}

//播放切换
void player_create(uint8_t mode){
    if (audio_player != NULL)
    {
        esp_audio_stop(audio_player,TERMINATION_TYPE_NOW);
        esp_audio_destroy(audio_player);
        audio_player = NULL;
    }
    // 定义配置结构体
    esp_audio_cfg_t cfg = DEFAULT_ESP_AUDIO_CONFIG();
    cfg.prefer_type = ESP_AUDIO_PREFER_MEM;
    // cfg.resample_rate = 48000;
    // cfg.cb_func = esp_audio_event_callback;//设置回调函数
    cfg.vol_handle = &i2s_stream_writer;
    cfg.vol_set = volume_set;
    cfg.vol_get = volume_get;
    // 创建音频实例
    audio_player = esp_audio_create(&cfg);
    if (audio_player == NULL) {
        ESP_LOGE("AUDIO", "Failed to create audio handle");
        return;
    }
    // Create writers and add to esp_audio
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    cfg.in_stream_buf_size = 10 * 1024;
    cfg.out_stream_buf_size = 10 * 1024;
    i2s_cfg.i2s_port = I2S_NUM;
    i2s_cfg.i2s_config.use_apll = false;//关闭apll时钟
    i2s_cfg.use_alc = true;//使用音量控制
    i2s_cfg.stack_in_ext = true;
    i2s_cfg.task_core = 1;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);
    i2s_setPin();

    audio_decoder_t auto_decode[] = {
        DEFAULT_ESP_MP3_DECODER_CONFIG(),
        DEFAULT_ESP_WAV_DECODER_CONFIG(),
        DEFAULT_ESP_AAC_DECODER_CONFIG(),
        DEFAULT_ESP_M4A_DECODER_CONFIG(),
        DEFAULT_ESP_TS_DECODER_CONFIG(),
        DEFAULT_ESP_FLAC_DECODER_CONFIG(),
    };
    esp_decoder_cfg_t auto_dec_cfg = DEFAULT_ESP_DECODER_CONFIG();
    esp_audio_codec_lib_add(audio_player, AUDIO_CODEC_TYPE_DECODER, esp_decoder_init(&auto_dec_cfg, auto_decode, 6));

    esp_audio_output_stream_add(audio_player, i2s_stream_writer);
    
    if (mode == PLAYER_MODE)//普通模式
    {
        fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
        fatfs_cfg.type = AUDIO_STREAM_READER;
        fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);

        // 将输入流和输出流添加到esp_audio_handle_t
        esp_audio_input_stream_add(audio_player, fatfs_stream_reader);

        esp_audio_callback_set(audio_player, player_event_handler, NULL);

        if(url != NULL) esp_audio_play(audio_player, AUDIO_CODEC_TYPE_DECODER, url, 0);
    }else if (mode == DLNA_MODE)//DLNA模式
    {

        // Create readers and add to esp_audio
        http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
        http_cfg.event_handle = _http_stream_event_handle;
        http_cfg.type = AUDIO_STREAM_READER;
        http_cfg.enable_playlist_parser = true;
        http_cfg.task_prio = 12;
        http_cfg.stack_in_ext = true;
        audio_element_handle_t http_stream_reader = http_stream_init(&http_cfg);
        
        esp_audio_input_stream_add(audio_player, http_stream_reader);

        esp_audio_callback_set(audio_player, dlna_player_event_handler, NULL);
    }

    //设置初始音量为20
    esp_audio_vol_set(audio_player,20);
}

//使用esp_audio高封装库的音频播放任务
void esp_audio_task(void *param){
    player_create(1);

    xTaskCreate(message_task,"message",8*1024,NULL,5,NULL);
    xTaskCreate(player_bar_task,"player_bar_task",8*1024,NULL,4,NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
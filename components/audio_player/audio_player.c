#include "audio_player.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "fatfs_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "filter_resample.h"

#include "esp_peripherals.h"
#include "periph_sdcard.h"
#include "periph_touch.h"
#include "periph_button.h"
#include "input_key_service.h"
#include "periph_adc_button.h"
#include "board.h"

#include "sdcard_list.h"
#include "sdcard_scan.h"

#include "sd.h"
#include "dev_board.h"

static const char *TAG = "AUDIO_PLAYER";

audio_pipeline_handle_t pipeline;
audio_element_handle_t i2s_stream_writer, mp3_decoder, fatfs_stream_reader, rsp_handle;
playlist_operator_handle_t sdcard_list_handle = NULL;

extern void change_btn_icon(bool isplaying);

static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx)
{
    /* Handle touch pad events
           to start, pause, resume, finish current song and adjust volume
        */
    audio_board_handle_t board_handle = (audio_board_handle_t) ctx;
    int player_volume;
    audio_hal_get_volume(board_handle->audio_hal, &player_volume);

    if (evt->type == INPUT_KEY_SERVICE_ACTION_CLICK_RELEASE) {
        ESP_LOGI(TAG, "[ * ] input key id is %d", (int)evt->data);
        switch ((int)evt->data) {
            case INPUT_KEY_USER_ID_PLAY:
                ESP_LOGI(TAG, "[ * ] [Play] input key event");
                audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
                switch (el_state) {
                    case AEL_STATE_INIT :
                        ESP_LOGI(TAG, "[ * ] Starting audio pipeline");
                        audio_pipeline_run(pipeline);
                        break;
                    case AEL_STATE_RUNNING :
                        ESP_LOGI(TAG, "[ * ] Pausing audio pipeline");
                        audio_pipeline_pause(pipeline);
                        break;
                    case AEL_STATE_PAUSED :
                        ESP_LOGI(TAG, "[ * ] Resuming audio pipeline");
                        audio_pipeline_resume(pipeline);
                        break;
                    default :
                        ESP_LOGI(TAG, "[ * ] Not supported state %d", el_state);
                }
                break;
            case INPUT_KEY_USER_ID_SET:
                ESP_LOGI(TAG, "[ * ] [Set] input key event");
                ESP_LOGI(TAG, "[ * ] Stopped, advancing to the next song");
                char *url = NULL;
                audio_pipeline_stop(pipeline);
                audio_pipeline_wait_for_stop(pipeline);
                audio_pipeline_terminate(pipeline);
                sdcard_list_next(sdcard_list_handle, 1, &url);
                ESP_LOGW(TAG, "URL: %s", url);
                audio_element_set_uri(fatfs_stream_reader, url);
                audio_pipeline_reset_ringbuffer(pipeline);
                audio_pipeline_reset_elements(pipeline);
                audio_pipeline_run(pipeline);
                break;
            case INPUT_KEY_USER_ID_VOLUP:
                ESP_LOGI(TAG, "[ * ] [Vol+] input key event");
                player_volume += 10;
                if (player_volume > 100) {
                    player_volume = 100;
                }
                audio_hal_set_volume(board_handle->audio_hal, player_volume);
                ESP_LOGI(TAG, "[ * ] Volume set to %d %%", player_volume);
                break;
            case INPUT_KEY_USER_ID_VOLDOWN:
                ESP_LOGI(TAG, "[ * ] [Vol-] input key event");
                player_volume -= 10;
                if (player_volume < 0) {
                    player_volume = 0;
                }
                audio_hal_set_volume(board_handle->audio_hal, player_volume);
                ESP_LOGI(TAG, "[ * ] Volume set to %d %%", player_volume);
                break;
        }
    }

    return ESP_OK;
}

//将扫描到的URL资源保存到播放列表
// void sdcard_url_save_cb(void *user_data, char *url)
// {
//     //传入的是扫描到的文件路径
//     playlist_operator_handle_t sdcard_handle = (playlist_operator_handle_t)user_data;
//     //保存播放列表是在sdcard_handle->playlist->offset_file与sdcard_handle->playlist->save_file
//     esp_err_t ret = sdcard_list_save(sdcard_handle, url);
//     // ESP_LOGE("sdcard_url_save_cb", "%s",url);
//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "Fail to save sdcard url to sdcard playlist");
//     }
// }

void i2s_setPin(void){
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = AUDIO_I2S_PIN_BCK,      // BCLK
        .ws_io_num = AUDIO_I2S_PIN_WS,        // LRC
        .data_out_num = AUDIO_I2S_PIN_DATA,   // DIN
        .data_in_num = I2S_PIN_NO_CHANGE// 无输入
        
    };
    // 配置 I2S 引脚
    esp_err_t err = i2s_set_pin(I2S_NUM, &pin_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pin: %s", esp_err_to_name(err));
        i2s_driver_uninstall(I2S_NUM);
        return;
    }
}

void message_task(void *param){
    char message[100];
    char url[100] = {0};
    audio_queue = xQueueCreate(10,sizeof(message));
    ESP_LOGI(TAG,"queue created");
    audio_element_state_t el_state;
    while (1)
    {
        if (xQueueReceive(audio_queue,message,pdMS_TO_TICKS(50)))
        {
            ESP_LOGI(TAG,"message:%s",message);
            if(message[0] == Audio_Control){
                
                el_state = audio_element_get_state(i2s_stream_writer);
                ESP_LOGI(TAG,"message: Audio_Control state:%d",el_state);
                switch (el_state) {
                    case AEL_STATE_INIT:
                        audio_element_set_uri(fatfs_stream_reader, url);
                        audio_pipeline_run(pipeline);
                        break;
                    case AEL_STATE_RUNNING:
                        audio_pipeline_pause(pipeline);
                        break;
                    case AEL_STATE_PAUSED:
                        audio_pipeline_resume(pipeline);
                        break;
                    case AEL_STATE_FINISHED:
                        audio_element_set_uri(fatfs_stream_reader, url);
                        audio_pipeline_reset_ringbuffer(pipeline);
                        audio_pipeline_reset_elements(pipeline);
                        audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
                        audio_pipeline_run(pipeline);
                        break;
                    default:
                        break;
                }
            }
            else if(message[0] == Audio_Resource){
                ESP_LOGI(TAG,"Audio_Resource resource:%s",url);
                sprintf(url,"%s",message+1);
                audio_pipeline_pause(pipeline);
                audio_pipeline_reset_ringbuffer(pipeline);
                audio_pipeline_reset_elements(pipeline);
                audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
                audio_element_set_uri(fatfs_stream_reader, url);
                audio_pipeline_run(pipeline);
            }
        }
        // vTaskDelay(pdMS_TO_TICKS(50));
    }
    vQueueDelete(audio_queue);
    vTaskDelete(NULL);
}

void audio_task(void *param){
    // esp_log_level_set("*", ESP_LOG_WARN);
    // esp_log_level_set(TAG, ESP_LOG_INFO);
    // esp_log_level_set("main", ESP_LOG_INFO);

    // ESP_LOGI(TAG, "[1.0] Initialize peripherals management");
    // esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    // esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    // ESP_LOGI(TAG, "[1.1] Initialize and start peripherals");
    // audio_board_key_init(set);//初始化ADC_BTN
    // sd_init();
    // audio_board_sdcard_init(set, SD_MODE_1_LINE);//初始化SD卡

    // ESP_LOGI(TAG, "[1.2] Set up a sdcard playlist and scan sdcard music save to it");
    // //创建一个播放列表
    // sdcard_list_create(&sdcard_list_handle);
    // //保存回调,路径,格式,
    // //扫描到的文件保存->默认路径
    // sdcard_scan(sdcard_url_save_cb, LV_FS_PATH, 1, (const char *[]) {"mp3"}, 1, sdcard_list_handle);//扫描文件
    // ESP_LOGI(TAG, "[1.3] Show up playlist");
    // //启用日志,否则下面的语句不输出
    // esp_log_level_set("PLAYLIST_SDCARD",ESP_LOG_INFO);
    // sdcard_list_show(sdcard_list_handle);
    
    // ESP_LOGI(TAG, "[ 2 ] Start codec chip");
    // audio_board_handle_t board_handle = audio_board_init();
    // audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    // ESP_LOGI(TAG, "[ 3 ] Create and start input key service");
    // input_key_service_info_t input_key_info[] = INPUT_KEY_DEFAULT_INFO();
    // input_key_service_cfg_t input_cfg = INPUT_KEY_SERVICE_DEFAULT_CONFIG();
    // input_cfg.handle = set;
    // periph_service_handle_t input_ser = input_key_service_create(&input_cfg);
    // input_key_service_add_key(input_ser, input_key_info, INPUT_KEY_NUM);
    // periph_service_set_callback(input_ser, input_key_service_cb, (void *)board_handle);

    ESP_LOGI(TAG, "[4.0] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[4.1] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_cfg.i2s_port = I2S_NUM;

    i2s_cfg.i2s_config.use_apll = false;//关闭apll时钟
    i2s_cfg.i2s_config.dma_buf_count = 3;
    i2s_cfg.i2s_config.dma_buf_len = 300;
    i2s_cfg.use_alc = true;//使用音量控制
    i2s_cfg.volume = -30;//range(-64,64)
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);
    // i2s_stream_set_clk(i2s_stream_writer, 48000, 16, 2);

    i2s_setPin();

    ESP_LOGI(TAG, "[4.2] Create mp3 decoder to decode mp3 file");
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_decoder = mp3_decoder_init(&mp3_cfg);

    /* ZL38063 audio chip on board of ESP32-LyraTD-MSC does not support 44.1 kHz sampling frequency,
       so resample filter has been added to convert audio data to other rates accepted by the chip.
       You can resample the data to 16 kHz or 48 kHz.
    */
    // ESP_LOGI(TAG, "[4.3] Create resample filter");
    // //将音源为44.1KHZ的重采样到48KHZ
    // rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    // rsp_handle = rsp_filter_init(&rsp_cfg);

    ESP_LOGI(TAG, "[4.4] Create fatfs stream to read data from sdcard");
    // char *url = NULL;
    // //从播放列表中获取文件的URL
    // sdcard_list_current(sdcard_list_handle, &url);
    // ESP_LOGI(TAG, "list url :%s",url);
    fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_cfg.type = AUDIO_STREAM_READER;
    fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);
    // audio_element_set_uri(fatfs_stream_reader, url);
    // audio_element_set_uri(fatfs_stream_reader, "/sdcard/系统/游戏平台/yxmb/10/mp3/1024.mp3");

    ESP_LOGI(TAG, "[4.5] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, fatfs_stream_reader, "file");
    audio_pipeline_register(pipeline, mp3_decoder, "mp3");
    // audio_pipeline_register(pipeline, rsp_handle, "filter");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

    ESP_LOGI(TAG, "[4.6] Link it together [sdcard]-->fatfs_stream-->mp3_decoder-->resample-->i2s_stream-->[codec_chip]");
    // const char *link_tag[4] = {"file", "mp3", "filter", "i2s"};
    const char *link_tag[3] = {"file", "mp3","i2s"};
    audio_pipeline_link(pipeline, &link_tag[0], 3);
    // const char *link_tag[2] = {"file", "mp3"};
    // audio_pipeline_link(pipeline, &link_tag[0], 2);

    ESP_LOGI(TAG, "[5.0] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[5.1] Listen for all pipeline events");
    audio_pipeline_set_listener(pipeline, evt);

    // audio_pipeline_run(pipeline);
    //用于UI和AUDIO组件间通讯
    xTaskCreate(message_task,"message",8*1024,NULL,5,NULL);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        /* Handle event interface messages from pipeline
           to set music info and to advance to the next song
        */
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) {
            // Set music info for a new song to be played
            if (msg.source == (void *) mp3_decoder
                && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
                audio_element_info_t music_info = {0};
                audio_element_getinfo(mp3_decoder, &music_info);
                ESP_LOGI(TAG, "[ * ] Received music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                         music_info.sample_rates, music_info.bits, music_info.channels);
                audio_element_setinfo(i2s_stream_writer, &music_info);
                // rsp_filter_set_src_info(rsp_handle, music_info.sample_rates, music_info.channels);
                //根据音源文件设置解码器参数
                i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
                continue;
            }
            // Advance to the next song when previous finishes
            if (msg.source == (void *) i2s_stream_writer
                && msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {
                audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
                if (el_state == AEL_STATE_FINISHED) {
                    ESP_LOGI(TAG, "[ * ] Finished, advancing to the next song");
                    // sdcard_list_next(sdcard_list_handle, 1, &url);
                    // ESP_LOGW(TAG, "URL: %s", url);
                    /* In previous versions, audio_pipeline_terminal() was called here. It will close all the element task and when we use
                     * the pipeline next time, all the tasks should be restarted again. It wastes too much time when we switch to another music.
                     * So we use another method to achieve this as below.
                     */
                    // audio_element_set_uri(fatfs_stream_reader, url);
                    // audio_pipeline_reset_ringbuffer(pipeline);
                    // audio_pipeline_reset_elements(pipeline);
                    audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
                    // audio_pipeline_run(pipeline);
                    change_btn_icon(false);
                }
                else if (el_state == AEL_STATE_STOPPED)
                {
                    audio_pipeline_reset_ringbuffer(pipeline);
                    audio_pipeline_reset_elements(pipeline);
                    audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
                    audio_pipeline_run(pipeline);
                }
                else if (el_state == AEL_STATE_PAUSED)
                {
                    change_btn_icon(false);
                }
                else if (el_state == AEL_STATE_RUNNING)
                {
                    change_btn_icon(true);
                }
                continue;
            }
        }
    }

    ESP_LOGI(TAG, "[ 7 ] Stop audio_pipeline");
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);

    audio_pipeline_unregister(pipeline, mp3_decoder);
    audio_pipeline_unregister(pipeline, i2s_stream_writer);
    audio_pipeline_unregister(pipeline, rsp_handle);

    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);

    /* Stop all peripherals before removing the listener */
    // esp_periph_set_stop_all(set);
    // audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), evt);

    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);

    /* Release all resources */
    sdcard_list_destroy(sdcard_list_handle);
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(i2s_stream_writer);
    audio_element_deinit(mp3_decoder);
    audio_element_deinit(rsp_handle);
    // periph_service_destroy(input_ser);
    // esp_periph_set_destroy(set);
    vTaskDelete(NULL);
}
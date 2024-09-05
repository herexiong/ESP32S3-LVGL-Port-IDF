#include "audio_dlna.h"
#include "esp_log.h"
#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "esp_wifi.h"
#include "esp_ssdp.h"
#include "esp_dlna.h"
#include "media_lib_adapter.h"

#include "esp_audio.h"

#include "audio_player.h"

#define DLNA_UNIQUE_DEVICE_NAME "ESP32_DMR_8db0797a"
#define DLNA_DEVICE_UUID "8db0797a-f01a-4949-8f59-51188b181809"

#define DLNA_ROOT_PATH "/rootDesc.xml"

static const char *TAG = "DLNA_AUDIO_PLAYER";
char *trans_state = "STOPPED";

static int vol, mute;
static char *track_uri = NULL; //音频轨道源

static esp_dlna_handle_t  dlna_handle = NULL;


extern esp_audio_handle_t audio_player;



void dlna_notify(void){
	esp_dlna_notify_avt_by_action(dlna_handle, "TransportState");
}


//dlan事件处理？
int renderer_request(esp_dlna_handle_t dlna, const upnp_attr_t *attr, int attr_num, char *buffer, int max_buffer_len)
{
    int req_type;
    int tmp_data = 0, buffer_len = 0;
    int hour = 0, min = 0, sec = 0;

    if (attr_num != 1) {
        return 0;
    }

    req_type = attr->type & 0xFF;
    switch (req_type) {
        case RCS_GET_MUTE:
            ESP_LOGD(TAG, "get mute = %d", mute);
            return snprintf(buffer, max_buffer_len, "%d", mute);
        case RCS_SET_MUTE:
            mute = atoi(buffer);
            ESP_LOGD(TAG, "set mute = %d", mute);
            if (mute) {
                esp_audio_vol_set(audio_player, 0);
            } else {
                esp_audio_vol_set(audio_player, vol);
            }
            esp_dlna_notify(dlna, "RenderingControl");
            return 0;
        case RCS_GET_VOL:
            esp_audio_vol_get(audio_player, &vol);
            ESP_LOGI(TAG, "get vol = %d", vol);
            return snprintf(buffer, max_buffer_len, "%d", vol);
        case RCS_SET_VOL:
            vol = atoi(buffer);
            ESP_LOGI(TAG, "set vol = %d", vol);
            esp_audio_vol_set(audio_player, vol);
            esp_dlna_notify(dlna, "RenderingControl");
            return 0;
        case AVT_PLAY:
            ESP_LOGI(TAG, "Play with speed=%s trans_state %s", buffer, trans_state);
            esp_audio_state_t state = { 0 };
            esp_audio_state_get(audio_player, &state);
            if (state.status == AUDIO_STATUS_PAUSED) {
                esp_audio_resume(audio_player);
                esp_dlna_notify(dlna, "AVTransport");
                break;
            } else if (track_uri != NULL) {
                esp_audio_play(audio_player, AUDIO_CODEC_TYPE_DECODER, track_uri, 0);
                esp_dlna_notify(dlna, "AVTransport");
            }
            return 0;
        case AVT_STOP:
            ESP_LOGI(TAG, "Stop instance=%s", buffer);
            esp_audio_stop(audio_player, TERMINATION_TYPE_NOW);
            esp_dlna_notify_avt_by_action(dlna_handle, "TransportState");
            return 0;
        case AVT_PAUSE:
            ESP_LOGI(TAG, "Pause instance=%s", buffer);
            esp_audio_pause(audio_player);
            esp_dlna_notify_avt_by_action(dlna_handle, "TransportState");
            return 0;
        case AVT_NEXT:
            esp_audio_stop(audio_player, TERMINATION_TYPE_NOW);
            return 0;
        case AVT_PREV:
            esp_audio_stop(audio_player, TERMINATION_TYPE_NOW);
            return 0;
        case AVT_SEEK:
            sscanf(buffer, "%d:%d:%d", &hour, &min, &sec);
            tmp_data = hour * 3600 + min * 60 + sec;
            ESP_LOGI(TAG, "Seekto %d s", tmp_data);
            esp_audio_seek(audio_player, tmp_data);//调整进度
            return 0;
        case AVT_SET_TRACK_URI:
            ESP_LOGI(TAG, "SetAVTransportURI=%s", buffer);
            esp_audio_state_t state_t = { 0 };
            esp_audio_state_get(audio_player, &state_t);
            if ((track_uri != NULL) && (state_t.status == AUDIO_STATUS_RUNNING) && strcasecmp(track_uri, buffer)) {
                esp_audio_stop(audio_player, TERMINATION_TYPE_NOW);
                esp_dlna_notify(dlna, "AVTransport");
            }
            free(track_uri);
            track_uri = NULL;
            if (track_uri == NULL) {
                asprintf(&track_uri, "%s", buffer);
            }
            return 0;
        case AVT_SET_TRACK_METADATA:
            ESP_LOGD(TAG, "CurrentURIMetaData=%s", buffer);
            ESP_LOGI(TAG, "CurrentURIMetaData=%s", buffer);
            return 0;
        case AVT_GET_TRACK_URI:
            if (track_uri != NULL) {
                return snprintf(buffer, max_buffer_len, "%s", track_uri);
            } else {
                return 0;
            }
        case AVT_GET_PLAY_SPEED:    /* ["1"] */
            return snprintf(buffer, max_buffer_len, "%d", 1);
        case AVT_GET_PLAY_MODE:
            return snprintf(buffer, max_buffer_len, "NORMAL");
        case AVT_GET_TRANS_STATUS:  /* ["ERROR_OCCURRED", "OK"] */
            return snprintf(buffer, max_buffer_len, "OK");
        case AVT_GET_TRANS_STATE:   /* ["STOPPED", "PAUSED_PLAYBACK", "TRANSITIONING", "NO_MEDIA_PRESENT"] */
            ESP_LOGI(TAG, "_avt_get_trans_state %s", trans_state);
            return snprintf(buffer, max_buffer_len, trans_state);
        case AVT_GET_TRACK_DURATION:
        case AVT_GET_MEDIA_DURATION:
            esp_audio_duration_get(audio_player, &tmp_data);
            tmp_data /= 1000;
            buffer_len = snprintf(buffer, max_buffer_len, "%02d:%02d:%02d", tmp_data / 3600, tmp_data / 60, tmp_data % 60);
            ESP_LOGD(TAG, "_avt_get_duration %s", buffer);
            return buffer_len;
        case AVT_GET_TRACK_NO:
            return snprintf(buffer, max_buffer_len, "%d", 1);
        case AVT_GET_TRACK_METADATA:
            return 0;
        // case AVT_GET_POS_ABSTIME:
        case AVT_GET_POS_RELTIME:
            esp_audio_time_get(audio_player, &tmp_data);
            tmp_data /= 1000;
            buffer_len = snprintf(buffer, max_buffer_len, "%02d:%02d:%02d", tmp_data / 3600, tmp_data / 60, tmp_data % 60);
            ESP_LOGD(TAG, "_avt_get_time %s", buffer);
            return buffer_len;
        // case AVT_GET_POS_ABSCOUNT:
        case AVT_GET_POS_RELCOUNT:
            esp_audio_pos_get(audio_player, &tmp_data);
            buffer_len = snprintf(buffer, max_buffer_len, "%d", tmp_data);
            ESP_LOGD(TAG, "_avt_get_pos %s", buffer);
            return buffer_len;
    }
    return 0;
}

static httpd_handle_t httpd;

static void start_dlna()
{
    const ssdp_service_t ssdp_service[] = {
        { DLNA_DEVICE_UUID, "upnp:rootdevice",                                  NULL },//支持 UPnP 协议
        { DLNA_DEVICE_UUID, "urn:schemas-upnp-org:device:MediaRenderer:1",      NULL },//媒体渲染：支持 UPnP 协议，可以作为媒体渲染器使用。
        { DLNA_DEVICE_UUID, "urn:schemas-upnp-org:service:ConnectionManager:1", NULL },//连接管理：提供 ConnectionManager 服务，用于管理设备连接。
        { DLNA_DEVICE_UUID, "urn:schemas-upnp-org:service:RenderingControl:1",  NULL },//渲染控制：提供 RenderingControl 服务，用于控制渲染设置，如音量和图像质量。
        { DLNA_DEVICE_UUID, "urn:schemas-upnp-org:service:AVTransport:1",       NULL },//音视频传输：支持 AVTransport 服务，用于音视频内容的传输和控制。
        { NULL, NULL, NULL },
    };

    ssdp_config_t ssdp_config = SSDP_DEFAULT_CONFIG();
    ssdp_config.udn = DLNA_UNIQUE_DEVICE_NAME;
    ssdp_config.location = "http://${ip}"DLNA_ROOT_PATH;//和根地址以及UUID
    esp_ssdp_start(&ssdp_config, ssdp_service);

    httpd = NULL;
    httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();
    httpd_config.max_uri_handlers = 25;
    httpd_config.stack_size = 6 * 1024;
    if (httpd_start(&httpd, &httpd_config) != ESP_OK) {
        ESP_LOGI(TAG, "Error starting httpd");
    }

    extern const uint8_t logo_png_start[] asm("_binary_logo_png_start");
    extern const uint8_t logo_png_end[] asm("_binary_logo_png_end");

    dlna_config_t dlna_config = {
        .friendly_name = "ESP32 MD (ESP32 Renderer)",
        .uuid = (const char *)DLNA_DEVICE_UUID,
        .logo               = {
            .mime_type  = "image/png",
            .data       = (const char *)logo_png_start,
            .size       = logo_png_end - logo_png_start,
        },
        .httpd          = httpd,
        .httpd_port     = httpd_config.server_port,
        .renderer_req   = renderer_request,
        .root_path      = DLNA_ROOT_PATH,
        .device_list    = false
    };

    dlna_handle = esp_dlna_start(&dlna_config);

    ESP_LOGI(TAG, "DLNA Started...");
}

esp_periph_handle_t wifi_handle;
esp_periph_set_handle_t set;

static bool netif_initialized = false;

void dlna_init_task(void *param){

    if (netif_initialized == false) {
        media_lib_add_default_adapter();
        ESP_ERROR_CHECK(esp_netif_init());

        //创建外设集
        esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
        set = esp_periph_set_init(&periph_cfg);
        //初始化wifi外设句柄
        periph_wifi_cfg_t wifi_cfg = {
            .wifi_config.sta.ssid = WiFi_SSID,
            .wifi_config.sta.password = WiFi_PWD,
        };
        wifi_handle = periph_wifi_init(&wifi_cfg);
        netif_initialized = true;
    }
    //将外设添加到外设集中（开启外设）
    esp_periph_start(set, wifi_handle);
    periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);

    player_create(DLNA_MODE);

    start_dlna();

	vTaskDelete(NULL);
}

void dlna_deinit_task(void *param){
    esp_ssdp_send_byebye();

    // 停止DLNA
    if (dlna_handle) {
        esp_dlna_destroy(dlna_handle);
        dlna_handle = NULL;
    }

    // 停止SSDP
    esp_ssdp_stop();

    // 停止HTTP服务器
    if (httpd) {
        httpd_stop(httpd);
        httpd = NULL;
    }

    esp_periph_stop(wifi_handle);

    player_create(PLAYER_MODE);

    vTaskDelete(NULL);
}

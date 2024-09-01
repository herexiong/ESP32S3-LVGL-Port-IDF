#ifndef AUDIO_PLAYER_H_
#define AUDIO_PLAYER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define I2S_NUM I2S_NUM_1

void message_task(void *param);

void esp_audio_task(void *param);
void player_set_time(int value);
void player_set_volume(int value);

void player_create(uint8_t mode);

QueueHandle_t audio_queue;
#define Audio_Control '0'
#define Audio_Resource '1'

typedef enum {
	DLNA_MODE = 0,
	PLAYER_MODE = 1,
}audio_player_mode_t;

#endif
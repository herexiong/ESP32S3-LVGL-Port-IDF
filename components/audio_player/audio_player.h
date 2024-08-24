#ifndef AUDIO_PLAYER_H_
#define AUDIO_PLAYER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define I2S_NUM I2S_NUM_1

void audio_task(void *param);
void message_task(void *param);

void esp_audio_task(void *param);
void player_set_time(int value);
void player_set_volume(int value);

QueueHandle_t audio_queue;
#define Audio_Control '0'
#define Audio_Resource '1'

#endif
#ifndef AUDIO_DLNA_H_
#define AUDIO_DLNA_H_

#define WiFi_SSID "ssid"
#define WiFi_PWD "password"

char *trans_state;
void dlna_notify(void);

void dlna_init_task(void *param);
void dlna_deinit(void);

#endif
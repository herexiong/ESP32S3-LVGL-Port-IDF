#ifndef AUDIO_METADATA_H_
#define AUDIO_METADATA_H_

typedef struct {
    char tag[3];
    char title[30];
    char artist[30];
    char album[30];
    char Year[4];
    char Comment[28];
	unsigned char Reserved;
	unsigned char TrackNum;
    unsigned char Genre; /*类型*/
} ID3v1_t;

ID3v1_t id3v1_data;

typedef struct {
    char *title;
    char *artist;
    char *album;
    char *trck;
    char *year;
    char *comment;
} ID3v2_t;

ID3v2_t id3v2_data;

unsigned char *img_src;
unsigned int img_size;

void audio_ReadMetaData_task(void *param);

#endif
#ifndef PLAYLIST_H_
#define PLAYLIST_H_

#define MAX_SONG_NUM 1000
#define MAX_SONG_NAME_LEN 100
#define SCAN_PATH "/sdcard"
#define PLAYLIST_PATH "/sdcard/mp3_player/playlist/playlist.txt"

typedef struct 
{
	char* playlist[MAX_SONG_NUM];//读取txt的playlist
	int playlist_count;//用于指示实际存储的歌曲数量-1
	char *scan_playlist[MAX_SONG_NUM];
	int scan_playlist_count;//扫描到的歌曲个数-1
	char *show_playlist[MAX_SONG_NUM];//UI显示的歌名
}sd_playlist_t;


int read_playlist(void);
void playlist_start_task(void *param);

#endif
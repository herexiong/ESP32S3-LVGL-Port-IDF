#include "sd_playlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"

#include "sd.h"

#include <sys/stat.h>
#include <sys/unistd.h>//unlink头文件

#define TAG "SD_PLAYLIST"

static SemaphoreHandle_t scan_semphr;
SemaphoreHandle_t read_semphr;

sd_playlist_t list = {
	.playlist = {0},
	.playlist_count = 0,
	.scan_playlist = {0},
	.scan_playlist_count = 0,
	.show_playlist = {0}
};

//从SD卡中读取音频列表文件
static bool read_playlist(char *sd_playlist_path){
	char buffer[MAX_SONG_NAME_LEN];
	size_t length;
	FILE *f_playlist = fopen(sd_playlist_path, "r");
	if (f_playlist != NULL)
	{
		ESP_LOGI(TAG,"playlist is existed,open now");
		while (fgets(buffer, MAX_SONG_NAME_LEN, f_playlist) != NULL) {
			length = strlen(buffer);
			// 去掉换行符
			if (length > 0 && buffer[length - 1] == '\n') {
				if (length >1 && buffer[length-2] == '\r')//window换行符是\r\n
				{
					buffer[length - 2] = '\0';
					length--;
				}
				buffer[length - 1] = '\0';
			}else{
				length++;
				buffer[length - 1] = '\0';
			}
			//将歌曲的URL拷贝到playlist
			list.playlist[list.playlist_count] = (char *)malloc(sizeof(char)*(length));
			strcpy(list.playlist[list.playlist_count],buffer);
			//将歌曲的名字拷贝到show_playlist
			char *separator_pos = strrchr(list.playlist[list.playlist_count],'/');//寻找最后一个文件分隔符的位置
			int len = length-(separator_pos - list.playlist[list.playlist_count])+1;//若len计算错误会导致下个循环的malloc失败
			list.show_playlist[list.playlist_count] = (char *)malloc(sizeof(char)*len);
			strcpy(list.show_playlist[list.playlist_count],separator_pos+1);
			// ESP_LOGI(TAG,"%s %d.",list.playlist[list.playlist_count],length);
			// ESP_LOGI(TAG,"%s %d.",list.show_playlist[list.playlist_count],list.playlist_count);
			// vTaskDelay(10);
			list.playlist_count++;
		}
		fclose(f_playlist);
		ESP_LOGI(TAG,"plsylist read successfully num:%d.",list.playlist_count);
		list.playlist_count--;
		return true;
	}
	else{
		ESP_LOGI(TAG,"playlist is not existed,scan now");
		return false;
	}
}

//从SD卡中递归扫描所有文件
//新esp_audio中已添加了aac,wav,m4a,ts格式的音频，若需要可在这添加后缀判断加入playlist
//更多如flac等音频格式可修改esp_audio_task内的auto_decode参数
static int read_content(const char *path, uint8_t *filecount)
{
    int ret = 0;
    char nextpath[300];
    struct dirent *de;
    DIR *dir = opendir(path);
    //递归读取整个目录
    while (1) {
        de = readdir(dir);
        if (!de) {
            break;
        } else if (de->d_type == DT_REG) {//如果是文件
            if (strstr(de->d_name, ".mp3") || strstr(de->d_name, ".MP3")) {
				list.scan_playlist[*filecount] = (char *)malloc(sizeof(char)*(strlen(path)+strlen(de->d_name)+2));
				
                sprintf(list.scan_playlist[*filecount], "%s/%s", path, de->d_name);
                // printf("filecount = %d :%s\n", (*filecount) , list.scan_playlist[*filecount]);
                if (++(*filecount) >= MAX_SONG_NUM) {
                    ESP_LOGE(TAG,"break %d",(*filecount));
                    ret = -1;
                    break;
                }
            }
        } else if (de->d_type == DT_DIR) {//目录则进入递归
            sprintf(nextpath, "%s/%s", path, de->d_name);
            ret = read_content(nextpath, filecount);
            if(ret == -1){
                break;
            }
        }
    }
    closedir(dir);
    return ret;
}

static void scan_task(void *param){
	char path[100];
	strcpy(path,(char *)param); 
	read_content(path, &list.scan_playlist_count);
	list.scan_playlist_count--;//统一为0开始,故减一
	xSemaphoreGive(scan_semphr);
	vTaskDelete(NULL);
}

void playlist_start_task(void *param){
	char sd_playlist_path[100];
	strcpy(sd_playlist_path,(char *)param); 
	scan_semphr = xSemaphoreCreateBinary();
	read_semphr = xSemaphoreCreateBinary();
	//标记txt中的播放列表和实际扫描到的列表是否有变化
	bool ischanged = false;

	xTaskCreate(scan_task,"scan_task",8*1024,SCAN_PATH,5,NULL);
	if (read_playlist(sd_playlist_path) == true)
	{
		//载入到UI
		xSemaphoreGive(read_semphr);
	}
    //设置等待位，等待扫描完成，只有此位被置位时才继续执行后续代码，否则阻塞
	xSemaphoreTake(scan_semphr,portMAX_DELAY);
	//对比txt和扫描结果
	int i = 0;
	int length,len;
	char *separator_pos;
	for (; i <= list.scan_playlist_count; i++)
	{
		if (i <= list.playlist_count && list.playlist[i] != NULL)
		{
			if(strcmp(list.scan_playlist[i],list.playlist[i]) != 0){//此条不一致
				//更新playlist
				free(list.playlist[i]);
				length = strlen(list.scan_playlist[i]);
				list.playlist[i] = (char *)malloc(sizeof(char)*(length+1));
				// printf("%s strlen:%d\n",list.scan_playlist[i],strlen(list.scan_playlist[i]));
				strcpy(list.playlist[i],list.scan_playlist[i]);
				//更新show_playlist
			separator_pos = strrchr(list.playlist[i],'/');//寻找最后一个文件分隔符的位置
			len = length-(separator_pos - list.playlist[i])+1;//若len计算错误会导致下个循环的malloc失败
			list.show_playlist[i] = (char *)malloc(sizeof(char)*len);
			strcpy(list.show_playlist[i],separator_pos+1);
				ischanged = true;
				ESP_LOGI(TAG,"syn a song from scan result");
			}
		}else if (i > list.playlist_count && list.scan_playlist[i] != NULL)//有新增歌曲
		{
			//更新playlist
			length = strlen(list.scan_playlist[i]);
			list.playlist[i] = (char *)malloc(sizeof(char)*(length+1));
			strcpy(list.playlist[i],list.scan_playlist[i]);
			printf("%s strlen:%d\n",list.scan_playlist[i],strlen(list.scan_playlist[i]));
				//更新show_playlist
			separator_pos = strrchr(list.playlist[i],'/');//寻找最后一个文件分隔符的位置
			len = length-(separator_pos - list.playlist[i])+1;//若len计算错误会导致下个循环的malloc失败
			list.show_playlist[i] = (char *)malloc(sizeof(char)*len);
			strcpy(list.show_playlist[i],separator_pos+1);
			ischanged = true;
			ESP_LOGI(TAG,"syn a song from scan result");
		}
		
	}
	//若playlist内有多余部分
	while (i < list.playlist_count)
	{
		ischanged = true;
		if(list.playlist[i] != NULL) free(list.playlist[i]);
		if(list.show_playlist[i] != NULL) free(list.show_playlist[i]);
		i++;
	}
	list.playlist_count = list.scan_playlist_count;
	ESP_LOGI(TAG,"ready to syn the playlist to SDCard");
	//扫描结果和txt文本不一致
	if (ischanged)
	{
		//重载入到UI
		xSemaphoreGive(read_semphr);
		//将playlist写入sdcard
		ESP_LOGE(TAG,"playlist changed ready rewrite playlist to SD");
		i = 0;
		//删掉旧的playlist
		struct stat st;
		if (stat(sd_playlist_path, &st) == 0) {
			unlink(sd_playlist_path);
		}
		ESP_LOGI(TAG,"delete old playlist successfully");
		FILE *f_playlist = fopen(sd_playlist_path, "w");
		ESP_LOGI(TAG,"create new playlist successfully");
		while (i<=list.playlist_count)
		{
			fprintf(f_playlist, "%s\n", list.playlist[i]);
			i++;
		}
		ESP_LOGI(TAG,"write to new playlist done");
		fclose(f_playlist);
		ESP_LOGI(TAG,"close new playlist successfully");
	}
	//释放scan_list
	i = 0;
	while (i<=list.scan_playlist_count)
	{
		free(list.scan_playlist[i]);
		i++;
	}
	ESP_LOGI(TAG,"playlist_start_task dead");
	vTaskDelete(NULL);
}


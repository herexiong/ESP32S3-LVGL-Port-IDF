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

#define TAG "PLAYLIST"

static SemaphoreHandle_t scan_semphr;
SemaphoreHandle_t read_semphr;

// char *playlist[MAX_SONG_NUM] = {0};//读取txt的playlist
// char *scan_playlist[MAX_SONG_NUM] = {0};
// char *show_playlist[MAX_SONG_NUM] = {0};
// int playlist_count = 0;//用于指示实际存储的歌曲数量-1
// int scan_playlist_count = 0;//扫描到的歌曲个数-1

sd_playlist_t list = {
	.playlist = {0},
	.playlist_count = 0,
	.scan_playlist = {0},
	.scan_playlist_count = 0,
	.show_playlist = {0}
};

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
				// printf("path: %s len:%d name:%s len:%d\n", path , strlen(path),de->d_name,strlen(de->d_name));
                printf("filecount = %d :%s\n", (*filecount) , list.scan_playlist[*filecount]);
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
	read_content(SCAN_PATH, &list.scan_playlist_count);
	list.scan_playlist_count--;//统一为0开始,故减一
	xSemaphoreGive(scan_semphr);
	vTaskDelete(NULL);
}

void playlist_start_task(void *param){
	// sd_init();
	scan_semphr = xSemaphoreCreateBinary();
	read_semphr = xSemaphoreCreateBinary();
	//标记txt中的播放列表和实际扫描到的列表是否有变化
	bool ischanged = false;

	xTaskCreate(scan_task,"scan_task",8*1024,NULL,5,NULL);
	if (read_playlist() == 1)
	{
		//载入到UI
		ESP_LOGI(TAG,"read_playlist succesfully");
		xSemaphoreGive(read_semphr);

	}
    //设置等待位，只有此位被置位时才继续执行后续代码，否则阻塞
	xSemaphoreTake(scan_semphr,portMAX_DELAY);
	//对比txt和扫描结果
	int i = 0;
	for (; i <= list.scan_playlist_count; i++)
	{
		int ret = 1;
		if (i<=list.playlist_count && list.playlist[i] != NULL)
		{
			ret = strcmp(list.scan_playlist[i],list.playlist[i]);
		}
		if(ret != 0){
			// printf("list.playlist[%d]:",i);
			// if (i<list.playlist_count)
			// {
			// 	// ESP_LOGI(TAG,"ready to free list.playlist[%d]",i);
			// 	free(list.playlist[i]);
			// }
			if (list.playlist[i] != NULL)
			{
				free(list.playlist[i]);
			}
			list.playlist[i] = (char *)malloc(sizeof(char)*(strlen(list.scan_playlist[i])+1));
			// printf("%s strlen:%d\n",list.scan_playlist[i],strlen(list.scan_playlist[i]));
			strcpy(list.playlist[i],list.scan_playlist[i]);
			ischanged = true;
			ESP_LOGI(TAG,"strcpy done");
		}
	}
	ESP_LOGI(TAG,"ready to syn the playlist");
	//若playlist内有多余部分
	while (i<=list.playlist_count)
	{
		ischanged = true;
		free(list.playlist[i]);
		i++;
	}
	//扫描结果和txt文本不一致
	if (ischanged)
	{
		//重载入到UI
		xSemaphoreGive(read_semphr);
		list.playlist_count = list.scan_playlist_count;
		//reload form scan_playlist
		ESP_LOGE(TAG,"reload form scan_playlist");
		// load_playlist_ui();
		//将playlist写入sdcard
		i = 0;
		//删掉txt
		struct stat st;
		if (stat(PLAYLIST_PATH, &st) == 0) {
			// Delete it if it exists
			unlink(PLAYLIST_PATH);
		}
		ESP_LOGI(TAG,"delete txt successfully");
		FILE *f_playlist = fopen(PLAYLIST_PATH, "w");
		ESP_LOGI(TAG,"open txt successfully");
		while (i<=list.playlist_count)
		{
			// ESP_LOGI(TAG,"written successfully times:%d",i);
			fprintf(f_playlist, "%s\n", list.playlist[i]);
			i++;
		}
		ESP_LOGI(TAG,"write to playlist done");
		fclose(f_playlist);
		ESP_LOGI(TAG,"close txt successfully");
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

// void create_ui_playlist(char **playlist,int playlist_count){
//     //ToDo->更新需要重新释放
//     //ToDo->封装playlist
//     ESP_LOGI(TAG,"create_ui_playlist playlist_count:%d",playlist_count);
//     for (int i = 0; i <= playlist_count && playlist[i] != NULL; i++)
//     {
//         int len = 0;
//         len = (strrchr(playlist[i],'/')-playlist[i])+1+1;
//         show_playlist[i] = (char *)malloc(sizeof(char)*len);
//         strcpy(show_playlist[i],strrchr(playlist[i],'/')+1);
//         // ESP_LOGI(TAG,"ui_playlist:%d %s",i,show_playlist[i]);
//     }
// }

int read_playlist(void){
	char buffer[MAX_SONG_NAME_LEN];
	size_t length;
	FILE *f_playlist = fopen(PLAYLIST_PATH, "r");
	if (f_playlist != NULL)
	{
		ESP_LOGI(TAG,"playlist is existed,open now");
		while (fgets(buffer, MAX_SONG_NAME_LEN, f_playlist) != NULL) {
			int len = 0;
			length = strlen(buffer);
			// 去掉换行符
			if (length > 0 && buffer[length - 1] == '\n') {
				if (buffer[length-2] == '\r')//window换行符是\r\n
				{
					buffer[length - 2] = '\0';
					length--;
				}
				buffer[length - 1] = '\0';
			}else{
				length++;
				buffer[length - 1] = '\0';
				
			}
			list.playlist[list.playlist_count] = (char *)malloc(sizeof(char)*(length));
			strcpy(list.playlist[list.playlist_count],buffer);
			//len计算错误导致下个循环的malloc失败
			len = (length-( strrchr(list.playlist[list.playlist_count],'/')-list.playlist[list.playlist_count] ))+1;
			list.show_playlist[list.playlist_count] = (char *)malloc(sizeof(char)*len);
			strcpy(list.show_playlist[list.playlist_count],strrchr(list.playlist[list.playlist_count],'/')+1);
			// ESP_LOGI(TAG,"%s %d.",list.playlist[list.playlist_count],length);
			// ESP_LOGI(TAG,"%s %d.",list.show_playlist[list.playlist_count],list.playlist_count);
			// vTaskDelay(10);
			list.playlist_count++;
		}
		fclose(f_playlist);
		ESP_LOGI(TAG,"plsylist read successfully num:%d.",list.playlist_count);
		list.playlist_count--;
		return 1;
	}
	else{
		ESP_LOGI(TAG,"playlist is not existed,scan now");
		return 0;
	}
}
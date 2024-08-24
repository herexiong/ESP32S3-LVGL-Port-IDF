#include "audio_metadata.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define TAG "AUDIO_METADATA"
#define ID3V1_TAG_SIZE 128 //ID3V1的大小是128B

extern SemaphoreHandle_t album_semphr;

//图片的内存
unsigned char *album_img;

//计算ID3V2帧大小
unsigned int get_frame_size(const unsigned char *size_bytes) {
    return (size_bytes[0] << 24) | 
           (size_bytes[1] << 16) | 
           (size_bytes[2] << 8)  | 
            size_bytes[3];
}

//读取ID3V1元数据
bool read_mp3_ID3V1(const char *mp3_file) {
    FILE *file = fopen(mp3_file, "rb");
    if (!file) {
		ESP_LOGE(TAG,"Failed to open file");
        return false;
    }
    ESP_LOGE(TAG,"open file done");
    fseek(file, -ID3V1_TAG_SIZE, SEEK_END);
    char buffer[ID3V1_TAG_SIZE];
    fread(buffer, ID3V1_TAG_SIZE, 1, file);
    ESP_LOGE(TAG,"read file done");
    if (strncmp(buffer, "TAG", 3) == 0) {
        strncpy(id3v1_data.tag, buffer+0, 3);
        strncpy(id3v1_data.title, buffer + 3, 30);
        strncpy(id3v1_data.artist, buffer + 33, 30);
        strncpy(id3v1_data.album, buffer + 63, 30);
        printf("tag:%s\n",id3v1_data.tag);
        printf("title:%s\n",id3v1_data.title);
        printf("artist:%s\n",id3v1_data.artist);
        printf("album:%s\n",id3v1_data.album);
        return true;
    } else {
        printf("No ID3v1 tag found.\n");
        return false;
    }
}

//将UTF16转码成UTF8
int utf16_to_utf8(unsigned char* utf16_str, char** utf8_str) {
    int utf8_index = 0;
    int utf8_size = 1; // 初始化UTF8字符串大小为1，用于存储字符串结束'\0'的空间

    int i = 0;
    while (utf16_str[i] != '\0' || utf16_str[i+1] != '\0') {
        //中文'：'0xFF1A有显示问题，替换为英文':'0x003A
        if (utf16_str[i] == 0x1A && utf16_str[i+1] == 0xFF)
        {
            printf("here\n");
            utf16_str[i] = 0x3A;
            utf16_str[i+1] = 0x00;
        }
        unsigned int unicode_code = (unsigned char)utf16_str[i] | ((unsigned char)utf16_str[i+1] << 8);
        i += 2;

        if (unicode_code >= 0xD800 && unicode_code <= 0xDBFF && utf16_str[i] != '\0' && utf16_str[i+1] != '\0') {
            unsigned int surrogate_pair_code = (unsigned char)utf16_str[i] | ((unsigned char)utf16_str[i+1] << 8);
            if (surrogate_pair_code >= 0xDC00 && surrogate_pair_code <= 0xDFFF) {
                unicode_code = ((unicode_code - 0xD800) << 10) + (surrogate_pair_code - 0xDC00) + 0x10000;
                i += 2;
            }
        }

        if (unicode_code < 0x80) {
            utf8_size += 1;
        } else if (unicode_code < 0x800) {
            utf8_size += 2;
        } else if (unicode_code < 0x10000) {
            utf8_size += 3;
        } else {
            utf8_size += 4;
        }
    }

    *utf8_str = (char*) malloc(utf8_size * sizeof(char));
    if (*utf8_str == NULL) {
        return 0; // 内存分配失败，返回错误码
    }

    i = 0;
    utf8_index = 0;
    while (utf16_str[i] != '\0' || utf16_str[i+1] != '\0') {
        unsigned int unicode_code = (unsigned char)utf16_str[i] | ((unsigned char)utf16_str[i+1] << 8);
        i += 2;

        if (unicode_code >= 0xD800 && unicode_code <= 0xDBFF && utf16_str[i] != '\0' && utf16_str[i+1] != '\0') {
            unsigned int surrogate_pair_code = (unsigned char)utf16_str[i] | ((unsigned char)utf16_str[i+1] << 8);
            if (surrogate_pair_code >= 0xDC00 && surrogate_pair_code <= 0xDFFF) {
                unicode_code = ((unicode_code - 0xD800) << 10) + (surrogate_pair_code - 0xDC00) + 0x10000;
                i += 2;
            }
        }

        if (unicode_code < 0x80) {
            (*utf8_str)[utf8_index++] = unicode_code;
        } else if (unicode_code < 0x800) {
            (*utf8_str)[utf8_index++] = ((unicode_code >> 6) & 0x1F) | 0xC0;
            (*utf8_str)[utf8_index++] = (unicode_code & 0x3F) | 0x80;
        } else if (unicode_code < 0x10000) {
            (*utf8_str)[utf8_index++] = ((unicode_code >> 12) & 0x0F) | 0xE0;
            (*utf8_str)[utf8_index++] = ((unicode_code >> 6) & 0x3F) | 0x80;
            (*utf8_str)[utf8_index++] = (unicode_code & 0x3F) | 0x80;
        } else {
            (*utf8_str)[utf8_index++] = ((unicode_code >> 18) & 0x07) | 0xF0;
            (*utf8_str)[utf8_index++] = ((unicode_code >> 12) & 0x3F) | 0x80;
            (*utf8_str)[utf8_index++] = ((unicode_code >> 6) & 0x3F) | 0x80;
            (*utf8_str)[utf8_index++] = (unicode_code & 0x3F) | 0x80;
        }
    }

    (*utf8_str)[utf8_index] = '\0';
    return 1; // 转换成功，返回成功码
}

//读取ID3V2中帧数据
void read_ID3V2_frame(FILE *file,int size,char **output){
    unsigned char char_type;
    fread(&char_type,1, 1, file);//读取编码方式
    printf("char type:%d\n",char_type);

    if (char_type == 1 || char_type ==2)//1代表字符使用UTF-16编码方式,2代表字符使用 UTF-16BE编码方式
    {
        unsigned char *utf16_buffer = (unsigned char *)malloc((size+1)*sizeof(char));
        if (!utf16_buffer) {
            perror("Failed to allocate memory");
            fclose(file);
            return;
        }
        fread(utf16_buffer, 1, size-1, file);
        //添加结尾符
        utf16_buffer[size-1] = '\0';
        utf16_buffer[size] = '\0';
        utf16_to_utf8(utf16_buffer+2,output);//前两个字节 0xff 和 0xfe 是UTF-16LE的字节顺序标记
        free(utf16_buffer);
    }else{
        fread(*output, 1, size-1, file);
    }
}

//读取ID3V2中的数据,保存至结构体中
bool read_mp3_ID3V2(const char *mp3_file) {
    unsigned int size;
    unsigned char buffer[10];
    size_t bytes_read;

    if (id3v2_data.title != NULL) {
        free(id3v2_data.title);
        id3v2_data.title = NULL;
    }
    if (id3v2_data.artist != NULL) {
        free(id3v2_data.artist);
        id3v2_data.artist = NULL;
    }
    if (id3v2_data.album != NULL) {
        free(id3v2_data.album);
        id3v2_data.album = NULL;
    }
    if (id3v2_data.trck != NULL) {
        free(id3v2_data.trck);
        id3v2_data.trck = NULL;
    }
    if (id3v2_data.year != NULL) {
        free(id3v2_data.year);
        id3v2_data.year = NULL;
    }
    if (id3v2_data.comment != NULL) {
        free(id3v2_data.comment);
        id3v2_data.comment = NULL;
    }
    
    FILE *file = fopen(mp3_file, "rb");
    if (!file) {
        perror("Failed to open file");
        return false;
    }

    bytes_read = fread(buffer, 1, 10, file);//读取10个字节写入到buffer中,前十个是描述ID3V2的头结构
    if (bytes_read < 10 || strncmp((char *)buffer, "ID3", 3) != 0) {//比较前n个字符是否为ID3
        printf("No ID3v2 tag found.\n");
        fclose(file);
        return false;
    }
    //获取标签的大小
    unsigned int tag_size = (buffer[6] & 0x7F) << 21 | 
                            (buffer[7] & 0x7F) << 14 | 
                            (buffer[8] & 0x7F) << 7  | 
                            (buffer[9] & 0x7F);
    //文件大小<标签大小+标签头大小
    while (ftell(file) < tag_size + 10) {
        bytes_read = fread(buffer, 1, 10, file);//读取10个字节写入到buffer中,帧头
        if (bytes_read < 10){//帧头不到10字节，非法帧
            fclose(file);
            return false;
        }

        if(strncmp((char *)buffer, "TIT2", 4) == 0){//标题
            size = get_frame_size(buffer + 4);//获取标签帧中的大小
            
            if ((int)size < 0) {
                printf("Invalid frame size: %d bytes\n", size);
                fclose(file);
                return false;
            }

            printf("TIT2 frame found, size: %u bytes\n", size);

            read_ID3V2_frame(file,size,&id3v2_data.title);

        } else if(strncmp((char *)buffer, "TPE1", 4) == 0){//作者
            size = get_frame_size(buffer + 4);//获取标签帧中的大小

            if ((int)size < 0) {
                printf("Invalid frame size: %d bytes\n", size);
                fclose(file);
                return false;
            }

            printf("TPE1 frame found, size: %u bytes\n", size);

            read_ID3V2_frame(file,size,&id3v2_data.artist);

        } else if(strncmp((char *)buffer, "TALB", 4) == 0){//专集
            size = get_frame_size(buffer + 4);//获取标签帧中的大小

            if ((int)size < 0) {
                printf("Invalid frame size: %d bytes\n", size);
                fclose(file);
                return false;
            }

            printf("TALB frame found, size: %u bytes\n", size);

            read_ID3V2_frame(file,size,&id3v2_data.album);
        } else if(strncmp((char *)buffer, "TRCK", 4) == 0){//音轨 格式:N/M 其中 N 为专集中的第 N 首,M 为专集中共 M 首,N 和 M 为 ASCII 码表示的数字
            size = get_frame_size(buffer + 4);//获取标签帧中的大小

            if ((int)size < 0) {
                printf("Invalid frame size: %d bytes\n", size);
                fclose(file);
                return false;
            }

            printf("TRCK frame found, size: %u bytes\n", size);

            read_ID3V2_frame(file,size,&id3v2_data.trck);
        } else if(strncmp((char *)buffer, "TYER", 4) == 0){//年代
            size = get_frame_size(buffer + 4);//获取标签帧中的大小

            if ((int)size < 0) {
                printf("Invalid frame size: %d bytes\n", size);
                fclose(file);
                return false;
            }

            printf("TYER frame found, size: %u bytes\n", size);

            read_ID3V2_frame(file,size,&id3v2_data.year);
        } else if(strncmp((char *)buffer, "TCON", 4) == 0){//类型
            size = get_frame_size(buffer + 4);//获取标签帧中的大小

            if ((int)size < 0) {
                printf("Invalid frame size: %d bytes\n", size);
                fclose(file);
                return false;
            }

            printf("TCON frame found, size: %u bytes\n", size);

            read_ID3V2_frame(file,size,&id3v2_data.comment);
        } else {
            unsigned int frame_size = get_frame_size(buffer + 4);
            fseek(file, frame_size, SEEK_CUR);
        }
    }

    fclose(file);
    return true;
}

//提取ID3V2中的专辑图并保存至堆中的内存
int extract_album_art(const char *mp3_file) {
    FILE *file = fopen(mp3_file, "rb");
    if (!file) {
		ESP_LOGE(TAG,"Failed to open file");
        return -1;
    }
	//释放上一次的内存
	if (album_img != NULL)
	{
		heap_caps_free(album_img);
        album_img = NULL;
	}
	
    unsigned char frame_head[10];
    size_t bytes_read;
    
    bytes_read = fread(frame_head, 1, 10, file);//读取10个字节写入到buffer中
    if (bytes_read < 10 || strncmp((char *)frame_head, "ID3", 3) != 0) {//比较前n个字符是否为ID3
		ESP_LOGE(TAG,"No ID3v2 tag found.");
        fclose(file);
        return -1;
    }
    //获取标签的大小
    unsigned int tag_size = (frame_head[6] & 0x7F) << 21 | 
                            (frame_head[7] & 0x7F) << 14 | 
                            (frame_head[8] & 0x7F) << 7  | 
                            (frame_head[9] & 0x7F);
    //文件大小<标签大小+标签头大小
    while (ftell(file) < tag_size + 10) {
        bytes_read = fread(frame_head, 1, 10, file);//读取10个字节写入到frame_head中
        if (bytes_read < 10) break;//标签头不到10字节，非法标签

        if (strncmp((char *)frame_head, "APIC", 4) == 0) {//标签ID为APIC，图片
            unsigned int frame_size = get_frame_size(frame_head + 4);//获取标签帧中的大小

            if ((int)frame_size < 0) {
				ESP_LOGI(TAG,"Invalid frame size: %d bytes", frame_size);
                fclose(file);
                return -1;
            }

			ESP_LOGI(TAG,"APIC frame found, size: %u bytes", frame_size);

            //在SPIRAM中的堆中分配内存
            album_img = (unsigned char *)heap_caps_malloc(frame_size, MALLOC_CAP_SPIRAM);
            if (!album_img) {
				ESP_LOGE(TAG,"Failed to allocate memory in SPIRAM");
                fclose(file);
                return -1;
            }

            fread(album_img, 1, frame_size, file);//将标签帧中的内容读到album_img中

            int offset = 1;
            while (album_img[offset] != '\0') offset++;//计算frame_content中非'\0'字符的数量
            offset++;
            offset++;

            // Locate start of JPEG data: looking for JPEG SOI marker (FF D8)
            while (offset < frame_size - 1 && !(album_img[offset] == 0xFF && album_img[offset + 1] == 0xD8)) {
                offset++;
            }

            if (offset >= frame_size - 1) {
                printf("Failed to find JPEG SOI marker.\n");
                free(album_img);
                fclose(file);
                return -1;
            }

            //开头offset不写.其余内容写到文件里
            // fwrite(album_img + offset, 1, frame_size - offset, img_file);
            //存在内存泄漏问题,但整体无法实现,故此不解决
			img_src = album_img + offset;
			img_size = frame_size - offset;
            // free(album_img);

//调试,将图片保存至内存卡中用于检查读取数据是否有问题
// FILE *album_file = fopen("/sdcard/image/album.jpg", "wb");
// if (album_file) {
//     fwrite(img_src, 1, img_size, album_file);
//     fclose(album_file);
// }
			ESP_LOGI(TAG,"Album art extracted");
            fclose(file);
            return 0;
        } else {
            unsigned int frame_size = get_frame_size(frame_head + 4);
            fseek(file, frame_size, SEEK_CUR);
        }
    }

    printf("No album art found.\n");
    fclose(file);
    return -1;
}

//读取元数据任务
void audio_ReadMetaData_task(void *param){
    //提取ID3V2中的专辑图
	// if (extract_album_art((char *)param) == 0)
	// {
	// 	ESP_LOGI(TAG,"Album art extracted successfully");
	// 	xSemaphoreGive(album_semphr);
	// }else{
	// 	ESP_LOGI(TAG,"Album art extracted faild");
	// }
    //提取ID3V2中的字符信息
	if (read_mp3_ID3V2((char *)param) == true)
	{
		ESP_LOGI(TAG,"read ID3V1 successfully");
		xSemaphoreGive(album_semphr);
	}else{
		ESP_LOGI(TAG,"Album art extracted faild");
	}
	vTaskDelete(NULL);
}
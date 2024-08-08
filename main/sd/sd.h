#ifndef SD_H_

//SD驱动
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_common.h"
#include "esp_err.h"

// #define LV_FS_PATH "/storage" //文件系统挂载路径
#define LV_FS_PATH "/sdcard"

#define SDMMC 1 			  //使用SDIO
#define SDSPI 0

sdmmc_card_t *card;

void sd_init(void);

#endif
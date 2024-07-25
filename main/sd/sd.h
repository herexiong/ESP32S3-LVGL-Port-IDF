#ifndef SD_H_

//SD驱动
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_common.h"
#include "esp_err.h"

#define TAG "SD"

#define LV_FS_PATH "/storage" //文件系统挂载路径

#define SDMMC 1 			  //使用SDIO
#define SDSPI 0
#if SDMMC
	#define CLK_IO 12
	#define CMD_IO 11
	#define D0_IO  13
#else if SDSPI
	#define CLK_IO  12
	#define MOSI_IO 11
	#define MISO_IO 13
#endif

sdmmc_card_t *card;

void sd_init(void);

#endif
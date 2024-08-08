#ifndef DEV_BOARD_H_
#define DEV_BOARD_H_
//sereen pin setting
#define SCREEN_PIN_NUM_DATA0          GPIO_NUM_1
#define SCREEN_PIN_NUM_DATA1          GPIO_NUM_2
#define SCREEN_PIN_NUM_DATA2          GPIO_NUM_7
#define SCREEN_PIN_NUM_DATA3          GPIO_NUM_8
#define SCREEN_PIN_NUM_DATA4          GPIO_NUM_3
#define SCREEN_PIN_NUM_DATA5          GPIO_NUM_18
#define SCREEN_PIN_NUM_DATA6          GPIO_NUM_17
#define SCREEN_PIN_NUM_DATA7          GPIO_NUM_16
#define SCREEN_PIN_NUM_PCLK           GPIO_NUM_15
#define SCREEN_PIN_NUM_CS             GPIO_NUM_10
#define SCREEN_PIN_NUM_DC             GPIO_NUM_9
#define SCREEN_PIN_NUM_RST            GPIO_NUM_14
#define SCREEN_PIN_NUM_BK_LIGHT       GPIO_NUM_4
//touch pin setting
#define CONFIG_I2C_MANAGER_0_SDA 	  GPIO_NUM_42
#define CONFIG_I2C_MANAGER_0_SCL 	  GPIO_NUM_41
#define CONFIG_I2C_MANAGER_1_SDA 	  GPIO_NUM_42
#define CONFIG_I2C_MANAGER_1_SCL 	  GPIO_NUM_41
#define CONFIG_GT911_RST_PIN 	  	  GPIO_NUM_39
#define CONFIG_GT911_INT_PIN 	  	  GPIO_NUM_40
//sdcard pin setting
#if SDMMC
	#define SD_PIN_NUM_CLK 			  GPIO_NUM_12
	#define SD_PIN_NUM_CMD 			  GPIO_NUM_11
	#define SD_PIN_NUM_D0  			  GPIO_NUM_13
#else if SDSPI
	#define SD_PIN_NUM_CLK 			  GPIO_NUM_12
	#define SD_PIN_NUM_MOSI 		  GPIO_NUM_11
	#define SD_PIN_NUM_MISO 		  GPIO_NUM_13
#endif
//audio i2s pin setting
#define AUDIO_I2S_PIN_BCK         	  GPIO_NUM_45
#define AUDIO_I2S_PIN_WS       	      GPIO_NUM_47// I2S word select io number    I2S_LRC
#define AUDIO_I2S_PIN_DATA            GPIO_NUM_46// I2S data out io number    I2S_DOUT

#endif

#ifndef LV_PORT_INDEV_H_
#define LV_PORT_INDEV_H_

#include <esp_log.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <driver/i2c.h>
#include "lvgl.h"

////////////////////////////////////////////
#define CONFIG_GT911_I2C_TOUCH_PORT 0 //使用到的I2C PORT
#define I2C_ZERO I2C_NUM_0
#define CONFIG_I2C_MANAGER_0_ENABLED 1

// #define I2C_ONE I2C_NUM_1
// #define CONFIG_I2C_MANAGER_1_ENABLED 0

//触摸颠倒 ToDo->和屏幕定义一起自动处理
// #define CONFIG_LV_GT911_SWAPXY 1
#define CONFIG_LV_GT911_INVERT_X 1
#define CONFIG_LV_GT911_INVERT_Y 1

#define CONFIG_I2C_MANAGER_0_FREQ_HZ 400000
#define CONFIG_I2C_MANAGER_0_TIMEOUT 20
#define CONFIG_I2C_MANAGER_0_LOCK_TIMEOUT 50
#define CONFIG_I2C_MANAGER_0_PULLUPS y

#define CONFIG_I2C_MANAGER_1_FREQ_HZ 400000
#define CONFIG_I2C_MANAGER_1_TIMEOUT 20
#define CONFIG_I2C_MANAGER_1_LOCK_TIMEOUT 50
#define CONFIG_I2C_MANAGER_1_PULLUPS y

#if defined (I2C_NUM_0) && defined (CONFIG_I2C_MANAGER_0_ENABLED)
	#define I2C_ZERO 					I2C_NUM_0
	#if defined (CONFIG_I2C_MANAGER_0_PULLUPS)
		#define I2C_MANAGER_0_PULLUPS 	true
	#else
		#define I2C_MANAGER_0_PULLUPS 	false
	#endif

	#define I2C_MANAGER_0_TIMEOUT 		( CONFIG_I2C_MANAGER_0_TIMEOUT / portTICK_RATE_MS )
	#define I2C_MANAGER_0_LOCK_TIMEOUT	( CONFIG_I2C_MANAGER_0_LOCK_TIMEOUT / portTICK_RATE_MS )
#endif

#if defined (I2C_NUM_1) && defined (CONFIG_I2C_MANAGER_1_ENABLED)
	#define I2C_ONE 					I2C_NUM_1
	#if defined (CONFIG_I2C_MANAGER_1_PULLUPS)
		#define I2C_MANAGER_1_PULLUPS 	true
	#else
		#define I2C_MANAGER_1_PULLUPS 	false
	#endif

	#define I2C_MANAGER_1_TIMEOUT 		( CONFIG_I2C_MANAGER_1_TIMEOUT / portTICK_RATE_MS )
	#define I2C_MANAGER_1_LOCK_TIMEOUT	( CONFIG_I2C_MANAGER_0_LOCK_TIMEOUT / portTICK_RATE_MS )
#endif

#define ERROR_PORT(port, fail) { \
	ESP_LOGE(TAG, "Invalid port or not configured for I2C Manager: %d", (int)port); \
	return fail; \
}

#if defined(I2C_ZERO) && defined (I2C_ONE)
	#define I2C_PORT_CHECK(port, fail) \
		if (port != I2C_NUM_0 && port != I2C_NUM_1) ERROR_PORT(port, fail);
#else
	#if defined(I2C_ZERO)
		#define I2C_PORT_CHECK(port, fail) \
			if (port != I2C_NUM_0) ERROR_PORT(port, fail);
	#elif defined(I2C_ONE)
		#define I2C_PORT_CHECK(port, fail) \
			if (port != I2C_NUM_1) ERROR_PORT(port, fail);
	#else
		#define I2C_PORT_CHECK(port, fail) \
			ERROR_PORT(port, fail);
	#endif
#endif

#define I2C_ADDR_10 ( 1 << 15 )
#define I2C_REG_16  ( 1 << 31 )
#define I2C_NO_REG  ( 1 << 30 )
////////////////////////////////////////////

#define GT911_I2C_SLAVE_ADDR   0x5D
#define GT911_PRODUCT_ID_LEN   4

/* Register Map of GT911 */
#define GT911_PRODUCT_ID1             0x8140
#define GT911_PRODUCT_ID2             0x8141
#define GT911_PRODUCT_ID3             0x8142
#define GT911_PRODUCT_ID4             0x8143
#define GT911_FIRMWARE_VER_L          0x8144
#define GT911_FIRMWARE_VER_H          0x8145
#define GT911_X_COORD_RES_L           0x8146
#define GT911_X_COORD_RES_H           0x8147
#define GT911_Y_COORD_RES_L           0x8148
#define GT911_Y_COORD_RES_H           0x8149
#define GT911_VENDOR_ID               0x814A

#define GT911_STATUS_REG              0x814E
#define   GT911_STATUS_REG_BUF        0x80
#define   GT911_STATUS_REG_LARGE      0x40
#define   GT911_STATUS_REG_PROX_VALID 0x20
#define   GT911_STATUS_REG_HAVEKEY    0x10
#define   GT911_STATUS_REG_PT_MASK    0x0F

#define GT911_TRACK_ID1               0x814F
#define GT911_PT1_X_COORD_L           0x8150
#define GT911_PT1_X_COORD_H           0x8151
#define GT911_PT1_Y_COORD_L           0x8152
#define GT911_PT1_Y_COORD_H           0x8153
#define GT911_PT1_X_SIZE_L            0x8154
#define GT911_PT1_X_SIZE_H            0x8155

typedef struct {
    bool inited;
    char product_id[GT911_PRODUCT_ID_LEN];
    uint16_t max_x_coord;
    uint16_t max_y_coord;
    uint8_t i2c_dev_addr;
} gt911_status_t;

void gt911_init(uint8_t dev_addr);

void lv_port_indev_init(void);

#endif
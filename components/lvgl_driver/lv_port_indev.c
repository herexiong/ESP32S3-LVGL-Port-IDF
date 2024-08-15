#include "lv_port_indev.h"
#include "dev_board.h"

#define TAG "GT911"

gt911_status_t gt911_status;
static const uint8_t ACK_CHECK_EN = 1;

static QueueHandle_t lvgl_i2c_local_mutex[2] = { NULL, NULL };
static QueueHandle_t* lvgl_i2c_mutex = &lvgl_i2c_local_mutex[0];

static void i2c_send_address(i2c_cmd_handle_t cmd, uint16_t addr, i2c_rw_t rw) {
	if (addr & I2C_ADDR_10) {
		i2c_master_write_byte(cmd, 0xF0 | ((addr & 0x3FF) >> 7) | rw, ACK_CHECK_EN);
		i2c_master_write_byte(cmd, addr & 0xFF, ACK_CHECK_EN);
	} else {
		i2c_master_write_byte(cmd, (addr << 1) | rw, ACK_CHECK_EN);
	}
}

static void i2c_send_register(i2c_cmd_handle_t cmd, uint32_t reg) {
	if (reg & I2C_REG_16) {
	    i2c_master_write_byte(cmd, (reg & 0xFF00) >> 8, ACK_CHECK_EN);
	}
    i2c_master_write_byte(cmd, reg & 0xFF, ACK_CHECK_EN);
}

esp_err_t lvgl_i2c_force_unlock(i2c_port_t port) {
	I2C_PORT_CHECK(port, ESP_FAIL);
	if (lvgl_i2c_mutex[port]) {
		vSemaphoreDelete(lvgl_i2c_mutex[port]);
	}
	lvgl_i2c_mutex[port] = xSemaphoreCreateMutex();
	return ESP_OK;
}

esp_err_t lvgl_i2c_lock(i2c_port_t port) {
	I2C_PORT_CHECK(port, ESP_FAIL);
	ESP_LOGV(TAG, "Mutex lock set for %d.", (int)port);

	TickType_t timeout;
	#if defined (I2C_ZERO)
		if (port == I2C_NUM_0) {
			timeout = (CONFIG_I2C_MANAGER_0_LOCK_TIMEOUT) / portTICK_RATE_MS;
		}
	#endif
	#if defined (I2C_ONE)
		if (port == I2C_NUM_1) {
			timeout = (CONFIG_I2C_MANAGER_1_LOCK_TIMEOUT) / portTICK_RATE_MS;
		}
	#endif

	if (xSemaphoreTake(lvgl_i2c_mutex[port], timeout) == pdTRUE) {
		return ESP_OK;
	} else {
		ESP_LOGE(TAG, "Removing stale mutex lock from port %d.", (int)port);
		lvgl_i2c_force_unlock(port);
		return (xSemaphoreTake(lvgl_i2c_mutex[port], timeout) == pdTRUE ? ESP_OK : ESP_FAIL);
	}
}

esp_err_t lvgl_i2c_unlock(i2c_port_t port) {
	I2C_PORT_CHECK(port, ESP_FAIL);
	ESP_LOGV(TAG, "Mutex lock removed for %d.", (int)port);
	return (xSemaphoreGive(lvgl_i2c_mutex[port]) == pdTRUE) ? ESP_OK : ESP_FAIL;
}

void gt911_set_addr(uint8_t dev_addr){
//fix show error the first time
		gpio_config_t io_conf;
		io_conf.intr_type = GPIO_INTR_DISABLE;
		io_conf.mode = GPIO_MODE_OUTPUT;
		io_conf.pin_bit_mask = (1ULL << CONFIG_GT911_INT_PIN)|(1ULL << CONFIG_GT911_RST_PIN);
		io_conf.pull_down_en = 0;
		io_conf.pull_up_en = 0;
		gpio_config(&io_conf);
		gpio_pad_select_gpio(CONFIG_GT911_INT_PIN);
		gpio_set_direction(CONFIG_GT911_INT_PIN, GPIO_MODE_OUTPUT);
		gpio_pad_select_gpio(CONFIG_GT911_RST_PIN);
		gpio_set_direction(CONFIG_GT911_RST_PIN, GPIO_MODE_OUTPUT);

		// 设置引脚电平
		gpio_set_level(CONFIG_GT911_INT_PIN, 0);
		gpio_set_level(CONFIG_GT911_RST_PIN, 0);
		vTaskDelay(pdMS_TO_TICKS(10));
		gpio_set_level(CONFIG_GT911_INT_PIN, GT911_I2C_SLAVE_ADDR==0x29);
		vTaskDelay(pdMS_TO_TICKS(1));
		gpio_set_level(CONFIG_GT911_RST_PIN, 1);
		vTaskDelay(pdMS_TO_TICKS(5));
		gpio_set_level(CONFIG_GT911_INT_PIN, 0);
		vTaskDelay(pdMS_TO_TICKS(50));
		vTaskDelay(pdMS_TO_TICKS(50));
}

esp_err_t lvgl_i2c_init(int port) {

	I2C_PORT_CHECK(port, ESP_FAIL);

	esp_err_t ret = ESP_OK;

	if (lvgl_i2c_mutex[port] == 0) {

		ESP_LOGI(TAG, "Starting I2C master at port %d.", (int)port);

		lvgl_i2c_mutex[port] = xSemaphoreCreateMutex();

		i2c_config_t conf = {0};
		
		#ifdef HAS_CLK_FLAGS
			conf.clk_flags = 0;
		#endif

		#if defined (I2C_ZERO)
			if (port == I2C_NUM_0) {
				conf.sda_io_num = CONFIG_I2C_MANAGER_0_SDA;
				conf.scl_io_num = CONFIG_I2C_MANAGER_0_SCL;
				conf.sda_pullup_en = I2C_MANAGER_0_PULLUPS ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
				conf.scl_pullup_en = conf.sda_pullup_en;
				conf.master.clk_speed = CONFIG_I2C_MANAGER_0_FREQ_HZ;
			}
		#endif

		#if defined (I2C_ONE)
			if (port == I2C_NUM_1) {
				conf.sda_io_num = CONFIG_I2C_MANAGER_1_SDA;
				conf.scl_io_num = CONFIG_I2C_MANAGER_1_SCL;
				conf.sda_pullup_en = I2C_MANAGER_1_PULLUPS ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
				conf.scl_pullup_en = conf.sda_pullup_en;
				conf.master.clk_speed = CONFIG_I2C_MANAGER_1_FREQ_HZ;
			}
		#endif

		conf.mode = I2C_MODE_MASTER;

		ret = i2c_param_config(port, &conf);
		ret |= i2c_driver_install(port, conf.mode, 0, 0, 0);

		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "Failed to initialise I2C port %d.", (int)port);
			ESP_LOGW(TAG, "If it was already open, we'll use it with whatever settings were used "
			              "to open it. See I2C Manager README for details.");
		} else {
			ESP_LOGI(TAG, "Initialised port %d (SDA: %d, SCL: %d, speed: %d Hz.)",
					 port, conf.sda_io_num, conf.scl_io_num, conf.master.clk_speed);
		}

	}

    return ret;
}

static esp_err_t lvgl_i2c_read(int port, uint16_t addr, uint32_t reg, uint8_t *buffer, uint16_t size) {

	I2C_PORT_CHECK(port, ESP_FAIL);

    esp_err_t result;

    // May seem weird, but init starts with a check if it's needed, no need for that check twice.
	lvgl_i2c_init(port);

   	ESP_LOGV(TAG, "Reading port %d, addr 0x%03x, reg 0x%04x", port, addr, reg);

	TickType_t timeout = 0;
	#if defined (I2C_ZERO)
		if (port == I2C_NUM_0) {
			timeout = I2C_MANAGER_0_TIMEOUT;
		}
	#endif
	#if defined (I2C_ONE)
		if (port == I2C_NUM_1) {
			timeout = I2C_MANAGER_1_TIMEOUT;
		}
	#endif

	if (lvgl_i2c_lock((int)port) == ESP_OK) {
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		if (!(reg & I2C_NO_REG)) {
			/* When reading specific register set the addr pointer first. */
			i2c_master_start(cmd);
			i2c_send_address(cmd, addr, I2C_MASTER_WRITE);
			i2c_send_register(cmd, reg);
		}
		/* Read size bytes from the current pointer. */
		i2c_master_start(cmd);
		i2c_send_address(cmd, addr, I2C_MASTER_READ);
		i2c_master_read(cmd, buffer, size, I2C_MASTER_LAST_NACK);
		i2c_master_stop(cmd);
		result = i2c_master_cmd_begin(port, cmd, timeout);
		i2c_cmd_link_delete(cmd);
		lvgl_i2c_unlock((int)port);
	} else {
		ESP_LOGE(TAG, "Lock could not be obtained for port %d.", (int)port);
		return ESP_ERR_TIMEOUT;
	}

    if (result != ESP_OK) {
    	ESP_LOGW(TAG, "Error: %d", result);
    }

	ESP_LOG_BUFFER_HEX_LEVEL(TAG, buffer, size, ESP_LOG_VERBOSE);

    return result;
}

esp_err_t lvgl_i2c_write(i2c_port_t port, uint16_t addr, uint32_t reg, const uint8_t *buffer, uint16_t size) {

	I2C_PORT_CHECK(port, ESP_FAIL);

    esp_err_t result;

    // May seem weird, but init starts with a check if it's needed, no need for that check twice.
	lvgl_i2c_init(port);

    ESP_LOGV(TAG, "Writing port %d, addr 0x%03x, reg 0x%04x", port, addr, reg);

	TickType_t timeout = 0;
	#if defined (I2C_ZERO)
		if (port == I2C_NUM_0) {
			timeout = (CONFIG_I2C_MANAGER_0_TIMEOUT) / portTICK_RATE_MS;
		}
	#endif
	#if defined (I2C_ONE)
		if (port == I2C_NUM_1) {
			timeout = (CONFIG_I2C_MANAGER_1_TIMEOUT) / portTICK_RATE_MS;
		}
	#endif

	if (lvgl_i2c_lock((int)port) == ESP_OK) {
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_send_address(cmd, addr, I2C_MASTER_WRITE);
		if (!(reg & I2C_NO_REG)) {
			i2c_send_register(cmd, reg);
		}
		i2c_master_write(cmd, (uint8_t *)buffer, size, ACK_CHECK_EN);
		i2c_master_stop(cmd);
		result = i2c_master_cmd_begin( port, cmd, timeout);
		i2c_cmd_link_delete(cmd);
		lvgl_i2c_unlock((int)port);
	} else {
		ESP_LOGE(TAG, "Lock could not be obtained for port %d.", (int)port);
		return ESP_ERR_TIMEOUT;
	}

    if (result != ESP_OK) {
    	ESP_LOGW(TAG, "Error: %d", result);
    }

	ESP_LOG_BUFFER_HEX_LEVEL(TAG, buffer, size, ESP_LOG_VERBOSE);

    return result;
}


//TODO: handle multibyte read and refactor to just one read transaction
esp_err_t gt911_i2c_read(uint8_t slave_addr, uint16_t register_addr, uint8_t *data_buf, uint8_t len) {
    return lvgl_i2c_read(CONFIG_GT911_I2C_TOUCH_PORT, slave_addr, register_addr | I2C_REG_16, data_buf, len);
}

esp_err_t gt911_i2c_write8(uint8_t slave_addr, uint16_t register_addr, uint8_t data) {
    uint8_t buffer = data;
    return lvgl_i2c_write(CONFIG_GT911_I2C_TOUCH_PORT, slave_addr, register_addr | I2C_REG_16, &buffer, 1);
}

bool gt911_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

void touch_driver_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
	gt911_read(drv, data);
}
//定义在外面防止内存被释放导致的无限重启错误
static lv_indev_drv_t indev_drv;

/**
  * @brief  Initialize for GT911 communication via I2C
  * @param  dev_addr: Device address on communication Bus (I2C slave address of GT911).
  * @retval None
  */
void gt911_init(uint8_t dev_addr) {
    if (!gt911_status.inited) {
        gt911_status.i2c_dev_addr = dev_addr;
        uint8_t data_buf;
        esp_err_t ret;
		gt911_set_addr(dev_addr);//fix show error the first time
        ESP_LOGI(TAG, "Checking for GT911 Touch Controller");
        if ((ret = gt911_i2c_read(dev_addr, GT911_PRODUCT_ID1, &data_buf, 1) != ESP_OK)) {
            ESP_LOGE(TAG, "Error reading from device: %s",
                        esp_err_to_name(ret));    // Only show error the first time
            return;
        }

        // Read 4 bytes for Product ID in ASCII
        for (int i = 0; i < GT911_PRODUCT_ID_LEN; i++) {
            gt911_i2c_read(dev_addr, (GT911_PRODUCT_ID1 + i), (uint8_t *)&(gt911_status.product_id[i]), 1);
        }
        ESP_LOGI(TAG, "\tProduct ID: %s", gt911_status.product_id);

        gt911_i2c_read(dev_addr, GT911_VENDOR_ID, &data_buf, 1);
        ESP_LOGI(TAG, "\tVendor ID: 0x%02x", data_buf);

        gt911_i2c_read(dev_addr, GT911_X_COORD_RES_L, &data_buf, 1);
        gt911_status.max_x_coord = data_buf;
        gt911_i2c_read(dev_addr, GT911_X_COORD_RES_H, &data_buf, 1);
        gt911_status.max_x_coord |= ((uint16_t)data_buf << 8);
        ESP_LOGI(TAG, "\tX Resolution: %d", gt911_status.max_x_coord);

        gt911_i2c_read(dev_addr, GT911_Y_COORD_RES_L, &data_buf, 1);
        gt911_status.max_y_coord = data_buf;
        gt911_i2c_read(dev_addr, GT911_Y_COORD_RES_H, &data_buf, 1);
        gt911_status.max_y_coord |= ((uint16_t)data_buf << 8);
        ESP_LOGI(TAG, "\tY Resolution: %d", gt911_status.max_y_coord);
        gt911_status.inited = true;
    }
}

//先调用gt911_init()初始化触摸再注册
void lv_port_indev_init(void){
    lv_indev_drv_init(&indev_drv);
    indev_drv.read_cb = &touch_driver_read;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev_drv);
}

/**
  * @brief  Get the touch screen X and Y positions values. Ignores multi touch
  * @param  drv:
  * @param  data: Store data here
  * @retval Always false
  */
bool gt911_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    uint8_t touch_pnt_cnt;        // Number of detected touch points
    static int16_t last_x = 0;  // 12bit pixel value
    static int16_t last_y = 0;  // 12bit pixel value
    uint8_t data_buf;
    uint8_t status_reg;

    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_STATUS_REG, &status_reg, 1);
//    ESP_LOGI(TAG, "\tstatus: 0x%02x", status_reg);
    touch_pnt_cnt = status_reg & 0x0F;
    if ((status_reg & 0x80) || (touch_pnt_cnt < 6)) {
        //Reset Status Reg Value
        gt911_i2c_write8(gt911_status.i2c_dev_addr, GT911_STATUS_REG, 0x00);
    }
    if (touch_pnt_cnt != 1) {    // ignore no touch & multi touch
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_REL;
        return false;
    }

//    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_TRACK_ID1, &data_buf, 1);
//    ESP_LOGI(TAG, "\ttrack_id: %d", data_buf);

    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_PT1_X_COORD_L, &data_buf, 1);
    last_x = data_buf;
    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_PT1_X_COORD_H, &data_buf, 1);
    last_x |= ((uint16_t)data_buf << 8);

    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_PT1_Y_COORD_L, &data_buf, 1);
    last_y = data_buf;
    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_PT1_Y_COORD_H, &data_buf, 1);
    last_y |= ((uint16_t)data_buf << 8);

#if CONFIG_LV_GT911_INVERT_X
    last_x = gt911_status.max_x_coord - last_x;
#endif
#if CONFIG_LV_GT911_INVERT_Y
    last_y = gt911_status.max_y_coord - last_y;
#endif
#if CONFIG_LV_GT911_SWAPXY
    int16_t swap_buf = last_x;
    last_x = last_y;
    last_y = swap_buf;
#endif

    data->point.x = last_x;
    data->point.y = last_y;
    data->state = LV_INDEV_STATE_PR;
    ESP_LOGI(TAG, "X=%u Y=%u", data->point.x, data->point.y);
    ESP_LOGV(TAG, "X=%u Y=%u", data->point.x, data->point.y);
    return false;
}

#include "sd.h"
#include "dev_board.h"

#define TAG "SD"

void sd_init(void){
    static int sd_inited = 0;
    if (!sd_inited)
    {
        /*E.g. for FatFS initialize the SD card and FatFS itself*/
        esp_err_t ret;

        esp_vfs_fat_mount_config_t mount_config = {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024
        };
        ESP_LOGI(TAG, "Initializing SD card");

    #if SDSPI
        ESP_LOGI(TAG, "Using SDSPI peripheral");
        sdmmc_host_t host = SDSPI_HOST_DEFAULT();
        host.slot = SPI3_HOST;    
        spi_bus_config_t bus_cfg = {
            .mosi_io_num = SD_PIN_NUM_MOSI,
            .miso_io_num = SD_PIN_NUM_MISO,
            .sclk_io_num = SD_PIN_NUM_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 4*1024*sizeof(uint8_t),
        };
        ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);

        spi_device_interface_config_t dev_config;
        spi_device_handle_t handle;
        spi_bus_add_device(SPI2_HOST,&dev_config, &handle);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize bus.");
            return;
        }    
        sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
        slot_config.gpio_cs = 38;
        slot_config.host_id = host.slot;

        ESP_LOGI(TAG, "Mounting filesystem");
        //挂载点，SD控制器，SD硬件插槽，VFS挂载配置，句柄
        ret = esp_vfs_fat_sdspi_mount(LV_FS_PATH , &host , &slot_config , &mount_config , &card);
    #elif SDMMC
        ESP_LOGI(TAG, "Using SDMMC peripheral");
        sdmmc_host_t host = SDMMC_HOST_DEFAULT();

        // This initializes the slot without card detect (CD) and write protect (WP) signals.
        // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
        sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
        // Set bus width to use:
        slot_config.width = 1;
        slot_config.clk = SD_PIN_NUM_CLK;
        slot_config.cmd = SD_PIN_NUM_CMD;
        slot_config.d0 = SD_PIN_NUM_D0;

        ESP_LOGI(TAG, "Mounting filesystem");
        ret = esp_vfs_fat_sdmmc_mount(LV_FS_PATH, &host, &slot_config, &mount_config, &card);    

    #endif    

        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                ESP_LOGE(TAG, "Failed to mount filesystem. "
                        "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
            } else {
                ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                        "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
            }
            return;
        }
        ESP_LOGI(TAG, "Filesystem mounted");
        // Card has been initialized, print its properties
        sdmmc_card_print_info(stdout, card);
        sd_inited = 1;
    }else{
        return;
    }
}
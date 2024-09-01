# LVGL移植
***开始前请将本工程resource目录下的所有内容拷贝到sd卡中***  
## 项目介绍
本仓库是用于记录LVGL移植到ESP32S3上的过程，使用的开发环境是ESP-IDF 4.4.6以及LVGL V8.3，理论上4.4的版本应该都可以使用。
从移植开始，本仓库已经实现了
+ LVGL移植
+ LVGL的帧率优化，以及去除开机花屏
+ SPI，8080驱动屏幕
+ I2C驱动触摸
+ 将SquareLine设计的UI界面移植到单片机中
+ SPI，SDIO驱动SD卡，并对接至LVGL中的文件系统
+ LVGL解码JPG，PNG，GIF多媒体文件
+ 修复[lvgl_esp32_drivers](https://github.com/lvgl/lvgl_esp32_drivers/pull/238)项目中的GT911返回-1的bug
+ 使用ESP-ADF组件播放SD卡中的MP3音频文件(MAX98357模块)
+ LVGL使用中文大字库显示中文
+ LVGL UI界面的MP3播放器

本项目使用的使用维可思的3.5寸320*480 TFT电容开发板  

## 分支介绍
此分支基于8080withSD分支,是一个用于练手的工程。  
基于[乐鑫的示例](https://github.com/espressif/esp-iot-solution/tree/release/v1.0/examples/hmi/mp3_example)修改而来,在原有的基础上实现了：  
+ 使用MAX98357模块播放音频 
+ 同时增加了异步扫描SD卡并缓存到SD卡的功能,极大加速了初始速度
+ 扫描SD卡中的音频文件并形成播放列表
+ 对于去掉了原播放列表中显示的路径
+ 使用队列做组件间通讯
+ 增加中文大字库,中文歌曲也可以正常显示  
+ I2S调整音量
+ 播放进度条显示及拖动改变播放进度
+ 解析MP3文件中的元数据并显示

## 下一步开发计划(画饼)
+ ~~整理代码~~(V1.3.1实现)
+ ~~增加音量控制~~(V1.2实现)
+ 自动播放设置
+ ~~增加音频时间~~(V1.1实现)
+ ~~增加音频信息显示~~(V1.3实现)
+ ~~增加进度条~~(V1.1实现)
+ ~~增加专辑图~~(已放弃，LVGL不支持缩放，解析黑屏，V1.3)
+ 显示歌词
---
+ 将播放列表缓存到NVS
+ 实现去掉SD卡也能正常开机
+ DLAN (V1.4实现,有bug待完善)

## [版本更新日志](./更新日志.md)


## 环境要求
+ ESP-IDF 4.4.6  
+ ESP32S3 N16R8
+ ST7796 显示器,分辨率为320*480
+ GT911电容触摸
+ LVGL V8.3
+ MAX98357模块(音频部分需要)

## 引脚设置
引脚的相关设置在main文件夹下的board.h中定义  
未来尝试和外设封装在一起做成版级驱动（画饼）
__引脚号__ | __用途__|__备注__
----------|---------|-------
11|SD_CMD_IO<br>SD_MOSI_IO|上拉
12|SD_CLK_IO<br>SD_CLK_IO|上拉
13|SD_D0_IO<br>SD_MISO_IO|上拉
38|SD_CS_IO|上拉
35|PSRAM和FLASH占用
36|PSRAM和FLASH占用
37|PSRAM和FLASH占用
41|TOUCH_I2C_SCL
42|TOUCH_I2C_SDA
39|CONFIG_GT911_RST_PIN
40|CONFIG_GT911_INT_PIN
45|AUDIO_I2S_PIN_BCK
1|SCEEEN_DATA0_IO
2|SCEEEN_DATA1_IO
7|SCEEEN_DATA2_IO
8|SCEEEN_DATA3_IO
3|SCEEEN_DATA4_IO
18|SCEEEN_DATA5_IO
17|SCEEEN_DATA6_IO
16|SCEEEN_DATA7_IO
15|SCEEEN_PCLK_IO
10|SCEEEN_CS_IO
9|SCEEEN_DC_IO
14|SCEEEN_RST_IO
4|SCEEEN_BK_LIGHT_IO
19|USB_D-_IO|未使用
20|USB_D+_IO|未使用
0|RESET|按下复位
5|BAT_ADC_IO|未使用
6|未使用
21|未使用
46|AUDIO_I2S_PIN_DATA
47|AUDIO_I2S_PIN_WS
48|未使用


---  

# 博客
随工程有移植以及优化教程文章,在doc目录下  
[1. lvgl移植](./doc/lvgl移植/lvgl.md)  
[2. LVGL帧率优化](./doc/lvgl帧率优化/lvgl帧率优化.md)  
[3. 使用SquareLine并移植到ESP32](./doc/使用Squareline并移植到ESP32/使用Squareline并移植到ESP32.md)  
[4. LVGL使用8080驱动屏幕，驱动SD卡对接至LVGL](./doc/LVGL使用8080串口驱动屏幕，并使用SD卡/LVGL使用8080驱动屏幕，驱动SD卡对接至LVGL%20%20.md)  
[5. 使用LVGL解码显示媒体文件](./doc/使用LVGL解码显示媒体文件/使用LVGL解码显示媒体文件.md)  
[6. 使用ESP-ADF播放MP3音频](./doc/使用ESP-ADF播放MP3音频/音频处理.md)  
[7. MP3文件中的元数据](./doc/MP3文件中的元数据/MP3文件中的元数据.md)
# LVGL移植
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

本项目使用的使用维可思的3.5寸320*480 TFT电容开发板  

## 分支介绍
此分支基于main分支，由于原main分支使用的为SPI屏幕，在驱动SD卡时发生了冲突，因此另起了本分支。对于初学者可按照博客顺序学习移植

## 环境要求
+ ESP-IDF 4.4.6  
+ ESP32S3 N16R8
+ ST7796 显示器,分辨率为320*480
+ GT911电容触摸
+ LVGL V8.3

---  

# 博客
随工程有移植以及优化教程文章,在doc目录下  
[1. lvgl移植](./doc/lvgl移植/lvgl.md)  
[2. LVGL帧率优化](./doc/lvgl帧率优化/lvgl帧率优化.md)  
[3. 使用SquareLine并移植到ESP32](./doc/使用Squareline并移植到ESP32/使用Squareline并移植到ESP32.md)  
[4. LVGL使用8080驱动屏幕，驱动SD卡对接至LVGL](./doc/LVGL使用8080串口驱动屏幕，并使用SD卡/LVGL使用8080驱动屏幕，驱动SD卡对接至LVGL%20%20.md)  
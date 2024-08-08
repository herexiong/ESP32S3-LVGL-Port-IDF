# LVGL移植

## 项目介绍
本仓库是用于记录LVGL移植到ESP32S3上的过程，使用的开发环境是ESP-IDF 4.4.6以及LVGL V8.3，理论上4.4的版本应该都可以使用。
由于冲突原因，后续使用8080驱动屏幕以及文件系统，媒体文件等在[8080withSD分支](https://github.com/herexiong/ESP32S3-LVGL-Port-IDF/tree/8080withSD)。后续博客基于新分支也不会在此分支进行更新，此分支现用于初入门移植LVGL的学习。
## 环境要求
+ ESP-IDF 4.4.6  
+ ESP32S3 N16R8(只要是S3都可以)
+ ST7796 SPI显示器,分辨率为320*480
+ GT911电容触摸芯片
+ LVGL V8.3
+ lvgl_esp32_drivers  
---  
本仓库是将LVGL到ESP32S3上，使用的开发环境是ESP-IDF 4.4.6，理论上4.4的版本应该都可以使用  
使用维可思的3.5寸TFT电容开发板，若使用其他硬件，请检查是否要自行更新引脚或驱动。  

# 博客
随工程有移植以及优化教程文章,在doc目录下  
[1. lvgl移植](./doc/lvgl移植/lvgl.md)  
[2. LVGL帧率优化](./doc/lvgl帧率优化/lvgl帧率优化.md)  
[3. 使用SquareLine并移植到ESP32](./doc/使用Squareline并移植到ESP32/使用Squareline并移植到ESP32.md)  
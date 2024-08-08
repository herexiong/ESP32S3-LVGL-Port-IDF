file(GLOB SOURCES *.c)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES lvgl driver esp_lcd main)
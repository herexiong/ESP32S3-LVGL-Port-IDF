file(GLOB SOURCES *.c)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES 
                        driver audio_pipeline 
                        audio_stream 
                        esp_peripherals 
                        input_key_service
                        playlist
                        main)
idf_component_register(SRCS "app_main.c"  "ntp_srv.c" "mqtt_app.c" "http_srv.c" "http_app.c" "config_handler.c" 
                    INCLUDE_DIRS "." "include")

spiffs_create_partition_image(storage ../spiffs-image FLASH_IN_PROJECT)
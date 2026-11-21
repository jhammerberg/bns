idf_component_register(SRCS "main.c" "bno055.c" "shape.c"
                       INCLUDE_DIRS "." "../../../common"
                       PRIV_REQUIRES esp_wifi esp_event nvs_flash esp_driver_i2c esp_driver_gpio esp_timer)

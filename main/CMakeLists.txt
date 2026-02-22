idf_component_register(SRCS "sigfoxmanager.c" "sensors.c" "loramanager.c" "espnow.c" "main.c"
                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS "."
                        REQUIRES bme280 esp_adc)

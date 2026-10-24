idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES display buttons wifi calendar menu audio esp_wifi esp_lcd esp_driver_spi esp_driver_i2s esp_driver_gpio nvs_flash esp_http_server esp_netif esp_event
)

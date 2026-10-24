idf_component_register(
    SRCS "display.c"
    INCLUDE_DIRS "."
    REQUIRES driver board esp_driver_gpio esp_driver_spi esp_lcd
)
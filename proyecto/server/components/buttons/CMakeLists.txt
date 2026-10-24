idf_component_register(
    SRCS "buttons.c"
    INCLUDE_DIRS "."
    REQUIRES driver board esp_driver_gpio
)
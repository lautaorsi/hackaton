#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void) {
    ESP_LOGI("MAIN", "Hello from my custom code folder!");
    
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}   
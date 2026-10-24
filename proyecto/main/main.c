#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "audio.h"
#include "buttons.h" 
#include "display.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "wifi.h"

#define MAX_SCREEN_CHARS 20
#define MAX_SCREEN_ROWS 8

struct DateTime {
    int weekDay;
    int hour;
    int minute;
};

struct CalendarEntry {
    char descripcion[MAX_SCREEN_CHARS];
    struct DateTime dateTime;
};



void app_main(void) {
    ESP_LOGI("MAIN", "Hello from my custom code folder!");
    
    start_server();

    bool display_ok = display_init() == ESP_OK;
    bool buttons_ok = buttons_init() == ESP_OK; 

    char strs[MAX_SCREEN_ROWS][MAX_SCREEN_CHARS];

    strcpy(strs[0], "Option 1"); 
    strcpy(strs[1], "Option 2");
    strcpy(strs[2], "Option 3");
    strcpy(strs[3], "Option 4");
    strcpy(strs[4], "Option 5");
    strcpy(strs[5], "Option 6");
    strcpy(strs[6], "Option 7");
    strcpy(strs[7], "Option 8");

    int selected_row = 0;
    int last_selected_row = -1; 

    if (display_ok) {

        // Main UI Loop
        while (true) {
            QueueHandle_t btn_q = buttons_event_queue();


            if (selected_row != last_selected_row) {
                display_clear();
                
                for (int i = 0; i < MAX_SCREEN_ROWS; i++) {
                    bool is_highlighted = (i == selected_row);
                    
                    char display_buffer[MAX_SCREEN_CHARS + 3];
                    // Prepend '>' to visually show which row is hovered
                    if (is_highlighted) {
                        snprintf(display_buffer, sizeof(display_buffer), ">%s", strs[i]);
                    } else {
                        snprintf(display_buffer, sizeof(display_buffer), " %s", strs[i]);
                    }

                    display_text(5, i * 8, display_buffer, is_highlighted);
                }
                display_flush();
                last_selected_row = selected_row;
            }



            button_event_t ev;
            if (xQueueReceive(btn_q, &ev, pdMS_TO_TICKS(50)) == pdPASS) {
                
                switch (ev.id) {
                case BTN_UP:
                    if (selected_row > 0) selected_row--;
                    break;
                case BTN_DOWN:
                    if (selected_row < MAX_SCREEN_ROWS - 1) selected_row++;
                    break;
                case BTN_KEY3:
                    ESP_LOGI("MAIN", "Button pressed on row: %d", selected_row);
                    break;
                case BTN_KEY4:
                    ESP_LOGI("MAIN", "EMERGENCY");
                    break;
                default:
                    break;  
                }

            
                vTaskDelay(pdMS_TO_TICKS(50)); 
            }
        }
    }
}

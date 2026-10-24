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

#include "calendar.h"

#define MAX_SCREEN_ROWS 8



void app_main(void) {
    ESP_LOGI("MAIN", "Hello from my custom code folder!");
    
    calendar_init();

    start_server();
    calendar_set_timezone(CALENDAR_TZ_ARGENTINA);
    calendar_set_fake_time(2026, 10, 23, 18, 59, 50); 
    calendar_load_dummy();
    calendar_start();

    bool display_ok = display_init() == ESP_OK;
    bool buttons_ok = buttons_init() == ESP_OK; 

    char strs[MAX_SCREEN_ROWS][MAX_SCREEN_CHARS];

    int selected_row = 0;
    int last_selected_row = -1; 

    if (display_ok) {

        char strs[MAX_SCREEN_ROWS][MAX_SCREEN_CHARS];
        for (int i = 0; i < MAX_SCREEN_ROWS; i++) {
            snprintf(strs[i], MAX_SCREEN_CHARS, "Option %d", i + 1);
        }

        // Main UI Loop
        while (true) {
            QueueHandle_t btn_q = buttons_event_queue();

            const struct CalendarEntry *alertEntry;
            if (xQueueReceive(calendar_alert_queue(), &alertEntry, 0) == pdPASS) {
                ESP_LOGI("MAIN","MANDAR LA NOTIFICACION %s AL CLIENTE", alertEntry->descripcion);


                //CODIGO QUE CORRE EL CLIENTE 
                //display_clear();
                // display_text(5, 0,  "** ALERT **", true);
                // display_text(5, 16, alertEntry->descripcion, false);
                // display_flush();

                //for (int i = 0; i < 3; i++) {
                //     audio_play_tone(880, 250);   // La5
                //     audio_play_tone(659, 250);   // Mi5
                // }

                // button_event_t ev;
                // xQueueReceive(btn_q, &ev, pdMS_TO_TICKS(10000));  // wait for a button
                // last_selected_row = -1;                           // redraw the menu

            }

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

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

#include "menu.h"  
#include "calendar.h"   

void app_main(void) {
    calendar_init();
    start_server();

    calendar_set_timezone(CALENDAR_TZ_ARGENTINA);
    calendar_set_fake_time(2026, 10, 23, 18, 59, 50);
    calendar_load_dummy();
    calendar_start();

    bool display_ok = display_init() == ESP_OK;
    bool buttons_ok = buttons_init() == ESP_OK;
    (void)buttons_ok;

    if (!display_ok) return;

    menu_t menu;
    menu_init(&menu);
    for (int i = 0; i < MENU_MAX_ROWS; i++) {
        char text[MAX_SCREEN_CHARS];
        snprintf(text, sizeof(text), "Option %d", i + 1);   // placeholder
        menu_add(&menu, text);
    }

    QueueHandle_t btn_q = buttons_event_queue();

    while (true) {
        const struct CalendarEntry *alertEntry;
        if (xQueueReceive(calendar_alert_queue(), &alertEntry, 0) == pdPASS) {
            ESP_LOGI("MAIN", "MANDAR LA NOTIFICACION %s AL CLIENTE", alertEntry->descripcion);
            // menu_invalidate(&menu);   // if you ever draw an alert on this screen
        }

        menu_draw(&menu);

        button_event_t ev;
        if (xQueueReceive(btn_q, &ev, pdMS_TO_TICKS(50)) == pdPASS && ev.pressed) {
            switch (ev.id) {
            case BTN_UP:   menu_move_up(&menu);   break;
            case BTN_DOWN: menu_move_down(&menu); break;
            case BTN_KEY3: {
                char removed[MAX_SCREEN_CHARS];
                if (menu_remove_selected(&menu, removed, sizeof(removed))) {
                    ESP_LOGI("MAIN", "Selected: %s", removed);
                }
                break;
            }
            case BTN_KEY4:
                ESP_LOGI("MAIN", "EMERGENCY");
                break;
            default:
                break;
            }
        }
    }
}
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "audio.h"
#include "buttons.h"
#include "display.h"
#include "server_link.h"
#include "wifi_manager.h"

static const char *TAG = "demo";

#define UI_PERIOD_MS   50
#define NOTICE_MAX_LEN 21     // 20 caracteres entran en la pantalla + '\0'
#define NOTICE_SHOW_MS 5000

static QueueHandle_t s_notice_queue;

/*
static const int s_button_freq[BTN_COUNT] = {
    [BTN_UP] = 659,
    [BTN_DOWN] = 440,
    [BTN_LEFT] = 494,
    [BTN_RIGHT] = 587,
    [BTN_CENTER] = 784,
    [BTN_KEY1] = 262,
    [BTN_KEY2] = 330,
    [BTN_KEY3] = 392,
    [BTN_KEY4] = 523,
};
*/

// Runs on the websocket task: only queues, never draws or plays.
static void on_server_message(const char *message)
{
    ESP_LOGI(TAG, "server dice: %s", message);

    if (strncmp(message, "notify ", 7) == 0) {
        char notice[NOTICE_MAX_LEN];
        snprintf(notice, sizeof(notice), "%.20s", message + 7);
        xQueueSend(s_notice_queue, notice, 0);
    }
}

static void draw_key(int x, int y, int size, bool pressed)
{
    if (pressed) {
        display_fill_rect(x, y, size, size, true);
    } else {
        display_fill_rect(x, y, size, size, false);
        display_draw_rect(x, y, size, size);
    }
}

static void draw_ui(uint32_t pressed, int mic_level, bool audio_ok, const char *notice)
{
    char line[24];

    display_clear();
    display_text(2, 1, notice[0] ? notice : "HACKATHON BOARD", true);

    display_text(0, 14, "MIC", false);
    display_draw_rect(22, 13, 82, 10);
    display_fill_rect(24, 15, mic_level * 78 / 100, 6, true);
    if (audio_ok) {
        snprintf(line, sizeof(line), "%3d", mic_level);
        display_text(107, 14, line, false);
    } else {
        display_text(107, 14, "ERR", false);
    }

    draw_key(15, 28, 10, pressed & (1u << BTN_UP));
    draw_key(4, 39, 10, pressed & (1u << BTN_LEFT));
    draw_key(15, 39, 10, pressed & (1u << BTN_CENTER));
    draw_key(26, 39, 10, pressed & (1u << BTN_RIGHT));
    draw_key(15, 50, 10, pressed & (1u << BTN_DOWN));

    display_text(50, 29, "KEYS", false);
    for (int i = 0; i < 4; i++) {
        bool on = pressed & (1u << (BTN_KEY1 + i));
        int x = 50 + i * 15;
        draw_key(x, 40, 13, on);
        char num[2] = {'1' + i, 0};
        display_text(x + 4, 43, num, on);
    }
}

// Sirena de dos tonos, unos 3 segundos.
static void play_alarm(void)
{
    for (int i = 0; i < 6; i++) {
        audio_play_tone(880, 250);
        audio_play_tone(659, 250);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Hackathon board demo starting");

    bool display_ok = display_init() == ESP_OK;
    if (!display_ok) {
        ESP_LOGE(TAG, "display init failed - continuing without screen");
    }
    ESP_ERROR_CHECK(buttons_init());

    s_notice_queue = xQueueCreate(4, NOTICE_MAX_LEN);
    ESP_ERROR_CHECK(wifi_manager_init());
    ESP_ERROR_CHECK(server_link_start(on_server_message));

    if (display_ok) {
        display_clear();
        display_text(10, 8, "Hello, Hackathon!", false);
        display_text(10, 24, "ESP32-S3 dev board", false);
        display_text(10, 40, "Self test...", false);
        display_flush();
    }

    bool audio_ok = audio_init() == ESP_OK;
    if (audio_ok) {
        const int notes[] = {523, 659, 784};
        for (int i = 0; i < 3; i++) {
            audio_play_tone(notes[i], 120);
        }
    } else {
        ESP_LOGE(TAG, "audio init failed - continuing without sound");
    }
    vTaskDelay(pdMS_TO_TICKS(500));

    QueueHandle_t events = buttons_event_queue();
    int log_div = 0;
    char notice[NOTICE_MAX_LEN] = "";
    int notice_ms_left = 0;

    while (1) {
        button_event_t ev;
        if (xQueueReceive(events, &ev, pdMS_TO_TICKS(UI_PERIOD_MS)) == pdTRUE) {
            ESP_LOGI(TAG, "button %s %s", buttons_name(ev.id), ev.pressed ? "pressed" : "released");
            if (ev.pressed) {
                char msg[40];
                snprintf(msg, sizeof(msg), "event button %s", buttons_name(ev.id));

                if (server_link_send(msg) != ESP_OK) {
                    ESP_LOGW(TAG, "no se pudo enviar: sin conexion al server");
                }
            }
        }

        char incoming[NOTICE_MAX_LEN];
        if (xQueueReceive(s_notice_queue, incoming, 0) == pdTRUE) {
            strcpy(notice, incoming);
            notice_ms_left = NOTICE_SHOW_MS;

            if (strcmp(incoming, "hola") == 0) {
                if (audio_ok) {
                    play_alarm();
                }
            } else if (audio_ok) {
                audio_play_tone(1000, 150);   // aviso normal
                audio_play_tone(1500, 300);
            }
        } else if (notice_ms_left > 0) {
            notice_ms_left -= UI_PERIOD_MS;
            if (notice_ms_left <= 0) {
                notice[0] = '\0';
            }
        }

        int mic_level = audio_ok ? audio_mic_level() : 0;
        if (display_ok) {
            draw_ui(buttons_read(), mic_level, audio_ok, notice);
            display_flush();
        }

        if (audio_ok && ++log_div >= 1000 / UI_PERIOD_MS) {
            log_div = 0;
            ESP_LOGI(TAG, "mic level: %d", mic_level);
        }
    }
}
#include "server_link.h"

#include <string.h>

#include "esp_log.h"
#include "esp_websocket_client.h"
#include "freertos/FreeRTOS.h"

#define SERVER_URI          "ws://192.168.4.1/device"
#define RECONNECT_TIMEOUT_MS 2000
#define SEND_TIMEOUT_MS      1000
#define MESSAGE_MAX_LEN      64
#define WS_OPCODE_TEXT       0x01

static const char *TAG = "server_link";

static esp_websocket_client_handle_t client;
static server_link_message_cb_t message_cb;

static void websocket_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
) {
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Conectado al server");
        break;

    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Desconectado del server; reintentando");
        break;

    case WEBSOCKET_EVENT_DATA:
        // Only handle whole, small text messages.
        if (data->op_code == WS_OPCODE_TEXT &&
            data->payload_offset == 0 &&
            data->data_len == data->payload_len &&
            data->data_len < MESSAGE_MAX_LEN &&
            message_cb != NULL) {

            char message[MESSAGE_MAX_LEN];
            memcpy(message, data->data_ptr, data->data_len);
            message[data->data_len] = '\0';
            message_cb(message);
        }
        break;

    default:
        break;
    }
}

esp_err_t server_link_start(server_link_message_cb_t on_message)
{
    message_cb = on_message;

    esp_websocket_client_config_t config = {
        .uri = SERVER_URI,
        .reconnect_timeout_ms = RECONNECT_TIMEOUT_MS,
    };

    client = esp_websocket_client_init(&config);
    if (client == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(esp_websocket_register_events(
        client, WEBSOCKET_EVENT_ANY, websocket_event_handler, NULL));

    return esp_websocket_client_start(client);
}

bool server_link_is_connected(void)
{
    return client != NULL && esp_websocket_client_is_connected(client);
}

esp_err_t server_link_send(const char *text)
{
    if (!server_link_is_connected()) {
        return ESP_ERR_INVALID_STATE;
    }

    int sent = esp_websocket_client_send_text(
        client, text, strlen(text), pdMS_TO_TICKS(SEND_TIMEOUT_MS));

    return sent < 0 ? ESP_FAIL : ESP_OK;
}
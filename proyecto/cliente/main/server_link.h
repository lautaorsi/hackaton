#pragma once

#include <stdbool.h>

#include "esp_err.h"

// Called for every text message received from the server (runs on the websocket task).
typedef void (*server_link_message_cb_t)(const char *message);

esp_err_t server_link_start(server_link_message_cb_t on_message);
bool server_link_is_connected(void);
esp_err_t server_link_send(const char *text);
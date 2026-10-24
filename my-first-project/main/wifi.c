
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#define WIFI_SSID   "eciExploit"
#define WIFI_PASS   "ioannis1"

static const char *TAG = "kiwi";

static httpd_handle_t s_server;
static volatile bool s_tune_requested;

static const char s_page[] =
    "<!doctype html><html><head><meta charset=utf-8>"
    "<meta name=viewport content='width=device-width,initial-scale=1'><title>Kiwi</title></head><body>"
    "<h1>Add event</h1>"
    "<select id=d><option value=0>Monday<option value=1>Tuesday<option value=2>Wednesday"
    "<option value=3>Thursday<option value=4>Friday<option value=5>Saturday<option value=6>Sunday</select> "
    "<input id=t type=time value=09:00> "
    "<input id=x maxlength=24 placeholder=event> "
    "<button id=a>Add</button>"
    "<p id=st>connecting...</p><script>"
    "let ws;"
    "function c(){ws=new WebSocket('ws://'+location.host+'/ws');"
    "ws.onopen=()=>{st.textContent='connected';"
    "ws.send('now '+Math.floor(Date.now()/1000)+' '+(-new Date().getTimezoneOffset()))};"
    "ws.onclose=()=>{st.textContent='disconnected, retrying...';setTimeout(c,1000)};"
    "ws.onmessage=e=>{st.textContent=e.data}}"
    "a.onclick=()=>{if(ws.readyState==1&&x.value)ws.send('add '+d.value+' '+t.value+' '+x.value)};"
    "c();</script></body></html>";

static esp_err_t page_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, s_page, sizeof(s_page) - 1);
}   

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "phone connected");
        return ESP_OK;
    }
    uint8_t buf[16] = {0};
    httpd_ws_frame_t frame = {.payload = buf};
    if (httpd_ws_recv_frame(req, &frame, sizeof(buf) - 1) != ESP_OK) {
        return ESP_FAIL;
    }
    if (frame.type == HTTPD_WS_TYPE_TEXT && strcmp((char *)buf, "tune") == 0) {
        s_tune_requested = true;
    }
    return ESP_OK;
}

// Runs on the http server task, the only one that should touch its sockets.
static void broadcast(void *arg)
{
    char text[24];
    int len = snprintf(text, sizeof(text), "%d %d", (int)((uintptr_t)arg >> 16), (int)((uintptr_t)arg & 0xffff));
    httpd_ws_frame_t frame = {.type = HTTPD_WS_TYPE_TEXT, .payload = (uint8_t *)text, .len = len};

    int fds[CONFIG_LWIP_MAX_SOCKETS];
    size_t n = CONFIG_LWIP_MAX_SOCKETS;
    if (httpd_get_client_list(s_server, &n, fds) != ESP_OK) {
        return;
    }
    for (size_t i = 0; i < n; i++) {
        if (httpd_ws_get_fd_info(s_server, fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET) {
            httpd_ws_send_frame_async(s_server, fds[i], &frame);
        }
    }
}

// Plays a note on every connected phone and waits for it to finish.
static void play(int freq_hz, int ms)
{
    httpd_queue_work(s_server, broadcast, (void *)(uintptr_t)(freq_hz << 16 | ms));
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static void play_tune(void)
{
    // Ode to Joy.
    static const int notes[][2] = {
        {330, 300}, {330, 300}, {349, 300}, {392, 300}, {392, 300}, {349, 300}, {330, 300}, {294, 300},
        {262, 300}, {262, 300}, {294, 300}, {330, 300}, {330, 450}, {294, 150}, {294, 600},
    };
    for (int i = 0; i < sizeof(notes) / sizeof(notes[0]); i++) {
        play(notes[i][0], notes[i][1] - 30);
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

static void wifi_ap_start(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
    wifi_config_t ap_cfg = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    // No power save: it adds latency to every note.
    esp_wifi_set_ps(WIFI_PS_NONE);
}

void start_server(void)
{
    wifi_ap_start();

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(httpd_start(&s_server, &cfg));
    const httpd_uri_t page = {.uri = "/", .method = HTTP_GET, .handler = page_handler};
    const httpd_uri_t ws = {.uri = "/ws", .method = HTTP_GET, .handler = ws_handler, .is_websocket = true};
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &page));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &ws));
    ESP_LOGI(TAG, "join Wi-Fi '%s' (password '%s') and open http://192.168.4.1", WIFI_SSID, WIFI_PASS);


}
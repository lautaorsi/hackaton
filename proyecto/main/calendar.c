#include "calendar.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define SLOTS (7 * 24 * 60)

static const char *TAG = "CAL";

static const struct CalendarEntry *slotTable[SLOTS];
static struct CalendarEntry entryStore[CALENDAR_MAX_ENTRIES]; 
static size_t entryCount = 0;

static QueueHandle_t alertQueue = NULL;
static SemaphoreHandle_t calMutex = NULL;


static inline int slot_index(const struct DateTime *d)
{
    return (d->weekDay * 24 + d->hour) * 60 + d->minute;
}

static bool datetime_valid(int weekDay, int hour, int minute)
{
    return weekDay >= 0 && weekDay <= 6 &&
           hour >= 0 && hour <= 23 &&
           minute >= 0 && minute <= 59;
}

void calendar_init(void)
{
    if (alertQueue == NULL) {
        alertQueue = xQueueCreate(4, sizeof(const struct CalendarEntry *));
    }
    if (calMutex == NULL) {
        calMutex = xSemaphoreCreateMutex();
    }
}

QueueHandle_t calendar_alert_queue(void)
{
    return alertQueue;
}

bool calendar_add(const char *description, int weekDay, int hour, int minute)
{
    if (!description || !datetime_valid(weekDay, hour, minute)) {
        ESP_LOGW(TAG, "Invalid entry arguments");
        return false;
    }

    xSemaphoreTake(calMutex, portMAX_DELAY);

    struct DateTime dt = { weekDay, hour, minute };
    int idx = slot_index(&dt);

    // Reuse the existing storage if the slot is already taken
    struct CalendarEntry *e = (struct CalendarEntry *)slotTable[idx];
    if (e) {
        ESP_LOGW(TAG, "Slot %d already used by '%s', overwriting",
                 idx, e->descripcion);
    } else {
        if (entryCount >= CALENDAR_MAX_ENTRIES) {
            xSemaphoreGive(calMutex);
            ESP_LOGW(TAG, "Calendar full");
            return false;
        }
        e = &entryStore[entryCount++];
    }

    strncpy(e->descripcion, description, MAX_SCREEN_CHARS - 1);
    e->descripcion[MAX_SCREEN_CHARS - 1] = '\0';
    e->dateTime = dt;
    slotTable[idx] = e;

    xSemaphoreGive(calMutex);
    return true;
}

bool calendar_add_relative(const char *description, int offsetMinutes)
{
    time_t when = time(NULL) + (time_t)offsetMinutes * 60;
    struct tm t;
    localtime_r(&when, &t);
    return calendar_add(description, t.tm_wday, t.tm_hour, t.tm_min);
}

const struct CalendarEntry *calendar_get(const struct DateTime *dt)
{
    if (!dt || !datetime_valid(dt->weekDay, dt->hour, dt->minute)) {
        return NULL;
    }
    return slotTable[slot_index(dt)];
}

size_t calendar_count(void)
{
    return entryCount;
}

void calendar_clear(void)
{
    xSemaphoreTake(calMutex, portMAX_DELAY);
    memset(slotTable, 0, sizeof(slotTable));
    entryCount = 0;
    xSemaphoreGive(calMutex);
}

void calendar_load_dummy(void)
{

    calendar_add("Gym",           5, 19, 0);

    ESP_LOGI(TAG, "Loaded %d dummy entries", (int)entryCount);
}

void calendar_set_timezone(const char *posixTz)
{
    setenv("TZ", posixTz, 1);
    tzset();
    ESP_LOGI(TAG, "Timezone set to %s", posixTz);
}

void calendar_set_fake_time(int year, int month, int day,
                            int hour, int minute, int second)
{
    // Values are interpreted in the current timezone (local time)
    struct tm t = {
        .tm_year  = year - 1900,
        .tm_mon   = month - 1,   // month is 1-12 here
        .tm_mday  = day,
        .tm_hour  = hour,
        .tm_min   = minute,
        .tm_sec   = second,
        .tm_isdst = -1,
    };
    struct timeval tv = { .tv_sec = mktime(&t), .tv_usec = 0 };
    settimeofday(&tv, NULL);

    ESP_LOGW(TAG, "Fake time set to %04d-%02d-%02d %02d:%02d:%02d (local)",
             year, month, day, hour, minute, second);
}

static void calendar_task(void *arg)
{
    int lastMinuteId = -1;

    while (true) {
        time_t now = time(NULL);
        struct tm t;
        localtime_r(&now, &t);

        // Ignore until the clock has been set (SNTP or fake time)
        if (t.tm_year + 1900 >= 2024) {
            int minuteId = t.tm_wday * 1440 + t.tm_hour * 60 + t.tm_min;

            // Check once per minute so an event doesn't fire repeatedly
            if (minuteId != lastMinuteId) {
                lastMinuteId = minuteId;

                struct DateTime nowDt = {
                    .weekDay = t.tm_wday,
                    .hour    = t.tm_hour,
                    .minute  = t.tm_min,
                };

                xSemaphoreTake(calMutex, portMAX_DELAY);
                const struct CalendarEntry *e = slotTable[slot_index(&nowDt)];
                xSemaphoreGive(calMutex);

                if (e) {
                    ESP_LOGW(TAG, "Event: %s", e->descripcion);
                    xQueueSend(alertQueue, &e, 0);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void calendar_start(void)
{
    xTaskCreate(calendar_task, "calendar", 4096, NULL, 5, NULL);
}
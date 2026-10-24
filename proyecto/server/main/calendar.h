#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifndef MAX_SCREEN_CHARS
#define MAX_SCREEN_CHARS 20
#endif

#define CALENDAR_MAX_ENTRIES 32

struct DateTime {
    int weekDay;   // 0 = Sunday ... 6 = Saturday (same as struct tm)
    int hour;      // 0-23
    int minute;    // 0-59
};

struct CalendarEntry {
    char descripcion[MAX_SCREEN_CHARS];
    struct DateTime dateTime;
};

/**
 * Creates the alert queue and the mutex. Call once, before anything else.
 */
void calendar_init(void);

/**
 * Queue that receives a `const struct CalendarEntry *` each time an event
 * fires. Read it from the UI task:
 *
 *   const struct CalendarEntry *e;
 *   if (xQueueReceive(calendar_alert_queue(), &e, 0) == pdPASS) { ... }
 */
QueueHandle_t calendar_alert_queue(void);

/**
 * Adds a weekly-recurring entry. Returns false if the arguments are invalid
 * or the store is full. If the slot is already used, it is overwritten.
 */
bool calendar_add(const char *description, int weekDay, int hour, int minute);

/**
 * Adds an entry `offsetMinutes` from the current time (handy for testing).
 */
bool calendar_add_relative(const char *description, int offsetMinutes);

/**
 * Returns the entry at that slot, or NULL if there is none.
 */
const struct CalendarEntry *calendar_get(const struct DateTime *dt);

/**
 * Number of entries currently stored.
 */
size_t calendar_count(void);

/**
 * Removes all entries.
 */
void calendar_clear(void);

/**
 * Fills the calendar with dummy entries for testing. Call after the time has
 * been set (real or fake), since some entries are relative to "now".
 */
void calendar_load_dummy(void);

// POSIX timezone strings. Argentina is UTC-3 all year (no DST).
#define CALENDAR_TZ_ARGENTINA "<-03>3"

/**
 * Sets the timezone used to interpret calendar entries (POSIX TZ string).
 * Call this first, before setting any time or adding entries.
 */
void calendar_set_timezone(const char *posixTz);

/**
 * Sets the system clock to a fixed LOCAL time (in the timezone set with
 * calendar_set_timezone). For testing without SNTP.
 */
void calendar_set_fake_time(int year, int month, int day,
                            int hour, int minute, int second);

/**
 * Starts the task that checks the time and pushes matching entries to the
 * alert queue.
 */
void calendar_start(void);
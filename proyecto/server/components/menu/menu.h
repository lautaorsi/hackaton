#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifndef MAX_SCREEN_CHARS
#define MAX_SCREEN_CHARS 20
#endif

#define MENU_MAX_ROWS 8

typedef struct {
    char items[MENU_MAX_ROWS][MAX_SCREEN_CHARS];
    int count;       // options currently in the list
    int selected;    // highlighted row
    bool dirty;      // needs a redraw
} menu_t;

void menu_init(menu_t *m);

/** Appends an option. Returns false if the menu is full. */
bool menu_add(menu_t *m, const char *text);

void menu_move_up(menu_t *m);
void menu_move_down(menu_t *m);

/** Text of the highlighted option, or NULL if the menu is empty. */
const char *menu_selected(const menu_t *m);

/**
 * Removes the highlighted option; the ones below shift up.
 * Copies the removed text into `out` (may be NULL). Returns false if empty.
 */
bool menu_remove_selected(menu_t *m, char *out, size_t out_len);

/** Forces a redraw on the next menu_draw() (e.g. after an alert screen). */
void menu_invalidate(menu_t *m);

/** Redraws the display only if something changed. */
void menu_draw(menu_t *m);

/**
 * Draws a full-screen alert: title banner, the message centered in a solid
 * block, and a footer. Call it repeatedly with `inverted` toggling to make
 * the block flash. It overwrites the whole screen, so call
 * menu_invalidate() afterwards to bring the menu back.
 */
void menu_draw_alert(const char *message, bool inverted);
#include "menu.h"

#include <stdio.h>
#include <string.h>

#include "display.h"
#include "sunicon.h"

static void draw_row_centered(int row, const char *text, bool inverted, int offset);

// Draws a 1-bit bitmap (row-major, MSB first) using the framebuffer's
// display_set_pixel(). Set bits light the pixel; clear bits are left alone.
static void draw_bitmap(int x, int y, int w, int h, const uint8_t *bits)
{
    const int stride = (w + 7) / 8;

    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            if (bits[j * stride + i / 8] & (0x80 >> (i % 8))) {
                display_set_pixel(x + i, y + j, true);
            }
        }
    }
}

// When there are no events, hardcode a sun and message
static void draw_empty_placeholder(void)
{
    
    const int x = 5 + (MAX_SCREEN_CHARS * DISPLAY_FONT_W - SUN_W) / 2;

    draw_bitmap(x, 0, SUN_W, SUN_H, sun_icon);

    draw_row_centered(6, "Buen dia", true, 0);
    draw_row_centered(7, "No hay eventos", false, 3);
}

//Reservers space
void menu_init(menu_t *m)
{
    memset(m, 0, sizeof(*m));
    m->dirty = true;
}

//Adds an item to the list and returns true if succeeded
bool menu_add(menu_t *m, const char *text)
{
    if (!text || m->count >= MENU_MAX_ROWS) return false;

    strncpy(m->items[m->count], text, MAX_SCREEN_CHARS - 1);
    m->items[m->count][MAX_SCREEN_CHARS - 1] = '\0';
    m->count++;
    m->dirty = true;
    return true;
}

void menu_move_up(menu_t *m)
{
    if (m->selected > 0) {
        m->selected--;
        m->dirty = true;
    }
}

void menu_move_down(menu_t *m)
{
    if (m->selected < m->count - 1) {
        m->selected++;
        m->dirty = true;
    }
}

const char *menu_selected(const menu_t *m)
{
    return m->count > 0 ? m->items[m->selected] : NULL;
}

bool menu_remove_selected(menu_t *m, char *out, size_t out_len)
{
    if (m->count == 0) return false;

    if (out && out_len > 0) {
        strncpy(out, m->items[m->selected], out_len - 1);
        out[out_len - 1] = '\0';
    }

    // Shift everything below the selected row up by one
    memmove(m->items[m->selected], m->items[m->selected + 1],
            (size_t)(m->count - m->selected - 1) * MAX_SCREEN_CHARS);
    m->count--;

    // If the last row was removed, move the cursor up with it
    if (m->selected >= m->count && m->selected > 0) {
        m->selected = m->count - 1;
    }
    m->dirty = true;
    return true;
}

void menu_invalidate(menu_t *m)
{
    m->dirty = true;
}

void menu_draw(menu_t *m)
{
    if (!m->dirty) return;

    display_clear();

    if (m->count == 0) {
        draw_empty_placeholder();
    }

    for (int i = 0; i < m->count; i++) {
        bool highlighted = (i == m->selected);

        char line[MAX_SCREEN_CHARS + 3];
        snprintf(line, sizeof(line), "%c%s", highlighted ? '>' : ' ', m->items[i]);

        display_text(5, i * 8, line, highlighted);
    }

    display_flush();
    m->dirty = false;
}



#define ALERT_FOOTER "Apreta un boton"

//Useful only for the alert
static void draw_row_centered(int row, const char *text, bool inverted, int offset)
{
    char line[MAX_SCREEN_CHARS + 1];
    int len = (int)strlen(text);
    if (len > MAX_SCREEN_CHARS) len = MAX_SCREEN_CHARS;
    int left = (MAX_SCREEN_CHARS - len - offset) / 2;

    memset(line, ' ', MAX_SCREEN_CHARS);
    memcpy(line + left, text, (size_t)len);
    line[MAX_SCREEN_CHARS] = '\0';

    display_text(5, row * 8, line, inverted);
}

void menu_draw_alert(const char *message)
{
    const int msg_row = MENU_MAX_ROWS / 2 - 1;

    display_clear();

    // Solid inverted block with the message in the middle
    for (int row = 1; row < MENU_MAX_ROWS - 1; row++) {
        draw_row_centered(row, row == msg_row && message ? message : "", true, 0);
    }

    draw_row_centered(MENU_MAX_ROWS - 1, ALERT_FOOTER, false, 5);

    display_flush();
}